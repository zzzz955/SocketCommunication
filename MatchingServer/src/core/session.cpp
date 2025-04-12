// core/session.cpp
// 세션 관리 클래스 구현
// 클라이언트와의 통신 세션을 처리하는 핵심 파일
#include "session.h"
#include "server.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>

namespace game_server {

    using json = nlohmann::json;

    Session::Session(boost::asio::ip::tcp::socket socket,
        std::map<std::string, std::shared_ptr<Controller>>& controllers,
        Server* server)
        : socket_(std::move(socket)),
        controllers_(controllers),
        user_id_(0),
        server_(server),
        last_activity_time_(std::chrono::steady_clock::now()),
        remote_ip_(socket_.remote_endpoint().address().to_string())
    {
        spdlog::info("새 세션이 생성되었습니다. 주소: {}:{}",
            socket_.remote_endpoint().address().to_string(),
            socket_.remote_endpoint().port());

    }

    Session::~Session() {
        if (server_) {
            if (is_mirror_) {
                server_->removeMirrorSession(mirror_port_);
            }
            if (!token_.empty()) {
                server_->removeSession(token_, user_id_);
            }
            if (!remote_ip_.empty()) {
                server_->removeConnection(remote_ip_);
            }
        }
    }

    void Session::initialize() {
        // 서버에 세션 등록 및 토큰 받기
        token_ = server_->registerSession(shared_from_this());
        spdlog::info("세션이 초기화되었습니다. 토큰: {}", token_);
    }

    void Session::handlePing() {
        last_activity_time_ = std::chrono::steady_clock::now();
        spdlog::debug("유저 ID : {}로 부터 핑을 받음", user_id_);

        json response = {
            {"action", "refreshSession"},
            {"status", "success"},
            {"message", "pong"},
            {"sessionToken", token_}
        };

        write_response(response.dump());
        spdlog::debug("핑 수신, 세션 {} 갱신됨", token_);
    }

    bool Session::isActive(std::chrono::seconds timeout) const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_time_);
        return elapsed < timeout;
    }

    const std::string& Session::getToken() const {
        return token_;
    }

    // 세션 시작 - 핸드셰이크부터 시작
    void Session::start() {
        read_handshake();  // 먼저 핸드셰이크 처리
    }

    // 핸드셰이크 메시지 처리
    void Session::read_handshake() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](boost::system::error_code ec, std::size_t length) {                
                if (!ec) {
                    try {
                        std::string data(buffer_.data(), length);
                        json handshake = json::parse(data);

                        // 미러 서버 구분 로직
                        if (handshake.contains("connectionType") &&
                            handshake["connectionType"] == "mirror" &&
                            handshake.contains("port")) {

                            // 미러 서버 세션으로 설정
                            is_mirror_ = true;
                            mirror_port_ = handshake["port"];
                            spdlog::info("미러 서버 연결이 수립되었습니다. 포트: {}", mirror_port_);

                            // 미러 서버 전용 초기화
                            server_->registerMirrorSession(shared_from_this(), handshake["port"]);
                            user_id_ = handshake["port"];

                            // 확인 응답 전송
                            json response = {
                                {"status", "success"},
                                {"message", "미러 서버가 연결되었습니다"}
                            };
                            write_handshake_response(response.dump());
                        }
                        else {
                            if (!server_->allowConnection(remote_ip_)) {
                                json response = {
                                    {"status", "error"},
                                    {"message", "이미 접속 중인 IP입니다."}
                                };
                                write_handshake_response(response.dump());
                                handle_error("다중 클라이언트 접속 감지 IP : " + remote_ip_);
                            }

                            // 일반 클라이언트 세션 초기화
                            std::string serverVersion = server_->getServerVersion();
                            if (!handshake.contains("version") || serverVersion != handshake["version"].get<std::string>()) {
                                handle_error("최신 버전이 아닌 클라이언트 접속 반려");
                                return;
                            }
                            initialize();

                            // 핸드셰이크가 실제 요청인 경우 처리
                            if (handshake.contains("action")) {
                                process_request(handshake);
                            }
                            else {
                                // 일반 클라이언트에게 연결 확인 메시지 전송
                                json response = {
                                    {"status", "success"},
                                    {"message", "서버에 연결되었습니다"}
                                };
                                write_handshake_response(response.dump());
                            }
                        }
                    }
                    catch (const std::exception& e) {
                        // 핸드셰이크 실패 처리
                        spdlog::error("핸드셰이크 오류: {}", e.what());
                        handle_error("잘못된 핸드셰이크 형식");
                    }
                }
                else {
                    handle_error("핸드셰이크 읽기 오류: " + ec.message());
                }
            });
    }

    // 핸드셰이크 응답 전송 (응답 후 일반 메시지 처리로 전환)
    void Session::write_handshake_response(const std::string& response) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(response),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 핸드셰이크 완료 후 일반 메시지 처리 시작
                    read_message();
                }
                else {
                    handle_error("핸드셰이크 응답 쓰기 오류: " + ec.message());
                }
            });
    }

    void Session::read_message() {
        auto self(shared_from_this());

        // 비동기적으로 데이터 읽기
        socket_.async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    try {
                        // 수신된 데이터를 문자열로 변환
                        std::string data(buffer_.data(), length);

                        // JSON 파싱
                        json request = json::parse(data);

                        // 요청 처리
                        process_request(request);
                    }
                    catch (const std::exception& e) {
                        // JSON 파싱 오류 등 예외 처리
                        spdlog::error("요청 데이터 처리 중 오류: {}", e.what());
                        json error_response = {
                            {"status", "error"},
                            {"message", "잘못된 요청 형식"}
                        };
                        write_response(error_response.dump());
                    }
                }
                else {
                    handle_error("메시지 읽기 오류: " + ec.message());
                }
            });
    }

    void Session::process_request(json& request) {
        try {
            spdlog::debug("요청 처리 중...");
            // action 필드로 요청 유형 확인
            std::string action = request["action"];
            spdlog::debug("액션: {}", action);
            std::string controller_type;

            // 컨트롤러 유형 결정
            if (action == "register" || action == "login" || action == "SSAFYlogin" || action == "updateNickName") {
                if (user_id_) request["userId"] = user_id_;
                controller_type = "auth";
            }
            else if (action == "createRoom" || action == "joinRoom" || action == "exitRoom" || action == "listRooms") {
                if (user_id_ == 0) {
                    json error_response = {
                        {"status", "error"},
                        {"message", "인증이 필요합니다"}
                    };
                    write_response(error_response.dump());
                    return;
                }

                request["userId"] = user_id_;
                controller_type = "room";
            }
            else if (action == "gameStart" || action == "gameEnd") {
                if (user_id_ == 0 || !is_mirror_) {
                    json error_response = {
                        {"status", "error"},
                        {"message", "권한이 없습니다."}
                    };
                    write_response(error_response.dump());
                    return;
                }

                request["userId"] = user_id_;
                controller_type = "game";
            }
            else if (action == "alivePing") {
                handlePing();
                return;
            }
            else if (action == "logout") {
                std::string logMessage = user_name_ + " 님이 로그아웃하였습니다";
                handle_error(logMessage);
                return;
            }
            else if (action == "roomCapacity") {
                json response;
                response["action"] = "roomCapacity";
                response["status"] = "success";
                response["roomCapacity"] = server_->getRoomCapacity();
                write_response(response.dump());
                return;
            }
            else if (action == "CCU") {
                json response;
                response["action"] = "CCU";
                response["status"] = "success";
                response["roomCapacity"] = server_->getCCU();
                write_response(response.dump());
                return;
            }
            else {
                // 알수 없는 액션 처리
                spdlog::warn("알 수 없는 액션: {}", action);
                json error_response = {
                    {"status", "error"},
                    {"message", "알 수 없는 액션"}
                };
                write_response(error_response.dump());
                return;
            }

            auto controller_it = controllers_.find(controller_type);
            if (controller_it != controllers_.end()) {
                spdlog::debug("컨트롤러 찾음: {}", controller_type);
                json response = controller_it->second->handleRequest(request);
                spdlog::debug("컨트롤러 응답 수신됨");

                if ((action == "login" || action == "SSAFYlogin") && response["status"] == "success") {
                    spdlog::debug("로그인 응답 처리 중");
                    if (server_->checkAlreadyLogin(response["userId"].get<int>())) {
                        spdlog::error("사용자 ID: {}는 이미 로그인되어 있습니다", response["userId"].get<int>());
                        json error_response = {
                            {"status", "error"},
                            {"message", "이미 로그인된 사용자입니다"}
                        };
                        write_response(error_response.dump());
                        return;
                    }

                    init_current_user(response);
                    std::string token = server_->registerSession(shared_from_this());
                    token_ = token;
                    response["sessionToken"] = token;
                }
                else if (action == "createRoom" && response["status"] == "success") {
                    spdlog::debug("방 생성 응답 처리 중");

                    // response 객체 디버깅 로그
                    spdlog::debug("응답 내용: {}", response.dump());

                    try {
                        json broad_response;
                        broad_response["action"] = "setRoom";
                        broad_response["roomId"] = response["roomId"];
                        broad_response["roomName"] = response["roomName"];
                        broad_response["maxPlayers"] = response["maxPlayers"];

                        auto mirror = server_->getMirrorSession(response["port"]);
                        if (!mirror) {
                            json error_response = {
                                {"status", "error"},
                                {"message", "미러 서버가 없습니다"}
                            };
                            spdlog::error("방 ID {}에 미러 서버가 없습니다", response["roomId"].get<int>());
                            write_response(error_response.dump());
                            return;
                        }
                        spdlog::debug("미러 서버 찾음, 메시지 브로드캐스팅");
                        status_ = std::to_string(broad_response["roomId"].get<int>()) + "번 방";
                        write_mirror(broad_response.dump(), mirror);
                    }
                    catch (const std::exception& e) {
                        spdlog::error("방 생성 응답 처리 중 오류: {}", e.what());
                        // 예외가 발생해도 원래 응답은 전송
                    }
                }
                else if (action == "joinRoom" && response["status"] == "success") {
                    status_ = std::to_string(response["roomId"].get<int>()) + "번 방";
                }
                else if (action == "exitRoom" && response["status"] == "success") {
                    status_ = "대기중";
                }
                else if (action == "gameStart" && response["status"] == "success") {
                    server_->setSessionStatus(response, true);
                }
                else if (action == "gameEnd" && response["status"] == "success") {
                    server_->setSessionStatus(response, false);
                }
                else if (action == "updateNickName" && response["status"] == "success") {
                    nick_name_ = response["nickName"];
                }

                spdlog::debug("클라이언트에 응답 전송 중");
                write_response(response.dump());
            }
            else {
                spdlog::error("컨트롤러를 찾지 못함: {}", controller_type);
                json error_response = {
                    {"status", "error"},
                    {"message", "내부 서버 오류"}
                };
                write_response(error_response.dump());
            }
        }
        catch (const std::exception& e) {
            spdlog::error("process_request 중 오류: {}", e.what());
            if (request.is_object() && request.contains("action")) {
                spdlog::error("실패한 액션: {}", request["action"].get<std::string>());
            }
            json error_response = {
                {"status", "error"},
                {"message", "잘못된 요청 형식"}
            };
            write_response(error_response.dump());
        }
    }

    void Session::write_mirror(const std::string& response, std::shared_ptr<Session> mirror) {
        boost::asio::async_write(
            mirror->socket_,
            boost::asio::buffer(response),
            [mirror](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 다음 요청 대기
                    mirror->read_message();
                }
                else {
                    mirror->handle_error("응답 쓰기 오류: " + ec.message());
                }
            });
    }

    void Session::write_broadcast(const std::string& response) {
        auto self = shared_from_this();
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(response),
            [self](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    self->handle_error("동접자 수 받기 에러: " + ec.message());
                }
            });
    }

    void Session::write_response(const std::string& response) {
        auto self(shared_from_this());

        // 클라이언트로 응답 데이터 전송
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(response),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // 다음 요청 대기
                    read_message();
                }
                else {
                    handle_error("응답 쓰기 오류: " + ec.message());
                }
            });
    }

    void Session::handle_error(const std::string& error_message) {
        // 오류 로깅
        spdlog::info(error_message);

        // 사용자가 방에 참여 중이라면 퇴장 처리
        try {
            auto controller_it = controllers_.find("room");
            if (controller_it != controllers_.end() && user_id_ > 0) {
                spdlog::debug("사용자 {}의 자동 방 퇴장 시도 중", user_id_);

                json temp = {
                    {"action", "exitRoom"},
                    {"userId", user_id_}
                };

                json response = controller_it->second->handleRequest(temp);

                if (response.contains("status") && response["status"] == "success") {
                    spdlog::info("사용자 {}가 세션 종료 시 자동으로 방에서 퇴장하였습니다", user_id_);
                }
            }
        }
        catch (const std::exception& e) {
            spdlog::error("방 퇴장 중 에러가 발생하였습니다. : {}", e.what());
        }

        if (socket_.is_open()) {
            boost::system::error_code ec;
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            if (ec) {
                spdlog::error("소켓 종료 중 에러가 발생하였습니다. : {}", ec.message());
            }

            socket_.close(ec);
            if (ec) {
                spdlog::error("소켓 종료 중 에러가 발생하였습니다. : {}", ec.message());
            }
            else {
                spdlog::info("소켓을 정상적으로 종료하였습니다.");
            }
        }
    }

    int Session::getUserId() {
        return user_id_;
    }

    std::string Session::getUserNickName() {
        return nick_name_;
    }

    void Session::setStatus(const std::string& status) {
        status_ = status;
    }

    std::string Session::getStatus() {
        return status_;
    }

    void Session::setToken(const std::string& token) {
        token_ = token;
    }

    void Session::init_current_user(const json& response) {
        if (response.contains("userId")) user_id_ = response["userId"];
        if (response.contains("userName")) user_name_ = response["userName"];
        if (response.contains("nickName")) nick_name_ = response["nickName"];
        status_ = "대기중";
        spdlog::info("{}유저가 로그인 하였습니다. (ID: {}) 닉네임 : {}", user_name_, user_id_, nick_name_);
    }

} // namespace game_server
