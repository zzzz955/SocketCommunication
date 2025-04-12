// core/server.cpp
#include "server.h"
#include "session.h"
#include "../controller/auth_controller.h"
#include "../controller/room_controller.h"
#include "../controller/game_controller.h"
#include "../service/auth_service.h"
#include "../service/room_service.h"
#include "../service/game_service.h"
#include "../repository/user_repository.h"
#include "../repository/room_repository.h"
#include "../repository/game_repository.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

namespace game_server {

    using json = nlohmann::json;

    Server::Server(boost::asio::io_context& io_context,
        short port,
        const std::string& db_connection_string,
        const std::string& version)
        : io_context_(io_context),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
        running_(false),
        uuid_generator_(),
        session_check_timer_(io_context),
        broadcast_timer_(io_context),
        version_(version)
    {
        // DB풀 생성
        db_pool_ = std::make_unique<DbPool>(db_connection_string, 20);

        // 컨트롤러 초기화
        init_controllers();

        spdlog::info("서버 초기화 완료! 포트 번호 : {}", port);
    }

    Server::~Server()
    {
        if (running_) {
            stop();
        }
    }

    void Server::setSessionStatus(const json& users, bool flag) {
        std::vector<std::string> tokens;
        {
            std::lock_guard<std::mutex> tokens_lock(tokens_mutex_);
            for (const auto& user : users["users"]) {
                int u = user.get<int>();
                if (!tokens_.count(u)) continue;
                tokens.push_back(tokens_[u]);
            }
        }
        {
            std::lock_guard<std::mutex> sesions_lock(sessions_mutex_);
            for (const std::string& token : tokens) {
                if (!sessions_.count(token)) continue;
                auto session = sessions_[token].lock();
                if (flag) session->setStatus("게임중");
                else session->setStatus("대기중");
            }
        }
    }

    std::string Server::getServerVersion() {
        return version_;
    }

    bool Server::checkAlreadyLogin(int userId) {
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        return tokens_.count(userId) > 0;
    }

    void Server::setSessionTimeout(std::chrono::seconds timeout) {
        session_timeout_ = timeout;
        spdlog::info("세션 타임아웃 발생 {} 초", timeout.count());
    }

    void Server::startSessionTimeoutCheck() {
        if (timeout_check_running_) return;
        timeout_check_running_ = true;
        check_inactive_sessions();
    }

    void Server::check_inactive_sessions() {
        if (!running_ || !timeout_check_running_) return;
        spdlog::debug("유효하지 않은 세션 체크 중...");

        std::vector<std::string> sessionsToRemove;
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            for (const auto& [token, wsession] : sessions_) {
                auto session = wsession.lock();
                if (!session) {
                    // 세션이 이미 소멸됨
                    spdlog::info("세션 {}가 이미 소멸되었습니다.", token);
                    sessionsToRemove.push_back(token);
                }
                else if (!session->isActive(session_timeout_)) {
                    // 세션이 존재하지만 타임아웃됨
                    spdlog::info("세션 {}가 {}초간 연결이 없어 타임아웃 되었습니다.",
                        token, session_timeout_.count());
                    sessionsToRemove.push_back(token);
                }
            }
        }

        for (const auto& token : sessionsToRemove) {
            std::shared_ptr<Session> session;
            {
                std::lock_guard<std::mutex> lock(sessions_mutex_);
                auto it = sessions_.find(token);
                if (it != sessions_.end()) {
                    session = it->second.lock();
                    sessions_.erase(it);  // 컬렉션에서 세션 제거
                    spdlog::info("세션 {}가 서버로 부터 삭제되었습니다.", token);
                }
            }

            if (session) {
                session->handle_error("세션 타임 아웃 발생");
            }
        }

        session_check_timer_.expires_after(std::chrono::seconds(10));
        session_check_timer_.async_wait([this](const boost::system::error_code& ec) {
            if (!ec) {
                check_inactive_sessions();
            }
            });
    }


    void Server::startBroadcastTimer() {
        if (broadcast_running_) return;
        broadcast_running_ = true;
        scheduleBroadcast();
    }

    void Server::scheduleBroadcast() {
        if (!running_ || !broadcast_running_) return;
        broadcast_timer_.expires_after(broadcast_interval_);
        broadcast_timer_.async_wait([this](const boost::system::error_code & ec) {
            if (!ec) {
                broadcastCCU();
                scheduleBroadcast();
            }
            else {
                spdlog::error("동시 접속자 브로드캐스트 타이머 오류: {}", ec.message());
            }
            });
    }

    std::string Server::generateSessionToken() {
        boost::uuids::uuid uuid = uuid_generator_();
        return boost::uuids::to_string(uuid);
    }

    std::string Server::registerSession(std::shared_ptr<Session> session) {
        std::lock_guard<std::mutex> lock(sessions_mutex_);

        // 기존 세션이 존재하면 제거
        for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
            if (it->second.lock() == session) {
                spdlog::info("이전에 할당된 토큰 확인, 삭제 후 새로운 토큰 할당: {}", it->first);
                sessions_.erase(it);
                break;  // 한 개만 삭제하면 되므로 루프 종료
            }
        }

        std::string token = generateSessionToken();
        sessions_[token] = session;
        int userId = session->getUserId();
        if (userId) {
            tokens_[userId] = token;
            spdlog::info("유저ID : {}에게 토큰ID : {} 할당 완료", userId, token);
        }
        return token;
    }

    void Server::registerMirrorSession(std::shared_ptr<Session> session, int port) {
        std::lock_guard<std::mutex> lock(mirrors_mutex_);

        // 기존 세션이 존재하면 제거
        for (auto it = mirrors_.begin(); it != mirrors_.end(); ++it) {
            if (it->second.lock() == session) {
                spdlog::info("이미 존재하는 미러 서버 세션이 확인되어 삭제 후 재할당 하였습니다, 포트 번호 : {}", it->first);
                mirrors_.erase(it);
                break;  // 한 개만 삭제하면 되므로 루프 종료
            }
        }

        mirrors_[port] = session;
        return;
    }

    void Server::removeSession(const std::string& token, int userId) {
        std::lock_guard<std::mutex> session_lock(sessions_mutex_);
        std::lock_guard<std::mutex> token_lock(tokens_mutex_);

        bool found = false;
        auto it_session = sessions_.find(token);
        if (it_session != sessions_.end()) {
            sessions_.erase(it_session);
            found = true;
        }

        auto it_token = tokens_.find(userId);
        if (it_token != tokens_.end()) {
            tokens_.erase(it_token);
            found = true;
        }
        if (found) {
            spdlog::info("유저 ID : {}의 토큰 삭제 완료, 토큰 ID : {}", userId, token);
        }
    }

    void Server::removeMirrorSession(int port) {
        std::lock_guard<std::mutex> lock(mirrors_mutex_);
        auto it = mirrors_.find(port);
        if (it != mirrors_.end()) {
            mirrors_.erase(it);
            spdlog::info("미러 서버 세션 삭제 완료, 포트 번호 : {}", port);
        }
    }

    std::shared_ptr<Session> Server::getSession(const std::string& token) {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = sessions_.find(token);
        if (it != sessions_.end()) {
            auto session = it->second.lock();
            if (session) {
                return session;
            }
            else {
                // 세션이 이미 소멸된 경우 맵에서 제거
                sessions_.erase(it);
                spdlog::info("토큰 ID : {}가 이미 삭제된 상태입니다, 세션 정보를 서버에서 제거하였습니다.", token);
            }
        }
        return nullptr;
    }

    std::shared_ptr<Session> Server::getMirrorSession(int port) {
        std::lock_guard<std::mutex> lock(mirrors_mutex_);
        auto it = mirrors_.find(port);
        if (it != mirrors_.end()) {
            auto session = it->second.lock();
            if (session) {
                return session;
            }
            else {
                // 세션이 이미 소멸된 경우 맵에서 제거
                mirrors_.erase(it);
                spdlog::info("포트 번호 : {}가 이미 삭제된 상태입니다, 미리 서버 세션 정보를 서버에서 제거하였습니다.", port);
            }
        }
        return nullptr;
    }

    int Server::getCCU() {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        return sessions_.size();
    }

    int Server::getRoomCapacity() {
        std::lock_guard<std::mutex> lock(mirrors_mutex_);
        return mirrors_.size();
    }

    std::vector<std::shared_ptr<Session>> Server::getWaitingSessions() {
        std::vector<std::shared_ptr<Session>> waitingSessions;
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (const auto& obj : sessions_) {
            auto session = obj.second.lock();
            if (!session || !session->getUserId()) continue;
            if (session->getStatus() == "대기중") {
                waitingSessions.push_back(session);
            }
        }
        return waitingSessions;
    }

    void Server::broadcastCCU() {
        json broadcast = {
            {"action", "CCUList"},
            {"users", json::array()}
        };
        std::vector<std::shared_ptr<Session>> waitingSessions;
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            for (const auto& obj : sessions_) {
                auto session = obj.second.lock();
                if (!session || !session->getUserId()) continue;

                json userInfo;
                userInfo["nickName"] = session->getUserNickName();
                userInfo["status"] = session->getStatus();
                broadcast["users"].push_back(userInfo);

                if (session->getStatus() == "대기중") {
                    waitingSessions.push_back(session);
                }
            }
        }
        broadcastActiveUser(broadcast.dump(), waitingSessions);
    }

    void Server::broadcastLogin(const std::string& nickName) {
        json broadcast = {
            {"action", "newLogin"},
            {"nickName", nickName}
        };
        auto waitingSessions = getWaitingSessions();
        broadcastActiveUser(broadcast.dump(), waitingSessions);
    }

    void Server::broadcastChat(const std::string& nickName, const std::string& message) {
        json broadcast = {
            {"action", "chat"},
            {"nickName", nickName},
            {"message", message}
        };
        auto waitingSessions = getWaitingSessions();
        broadcastActiveUser(broadcast.dump(), waitingSessions);
    }

    void Server::broadcastActiveUser(const std::string& message, const std::vector<std::shared_ptr<Session>>& sessions) {
        for (const auto& session : sessions) {
            session->write_broadcast(message);
        }
    }

    void Server::init_controllers() {
        // 레포지토리 생성
        auto userRepo = UserRepository::create(db_pool_.get());
        auto roomRepo = RoomRepository::create(db_pool_.get());
        auto gameRepo = GameRepository::create(db_pool_.get());

        std::shared_ptr<UserRepository> sharedUserRepo = std::move(userRepo);
        std::shared_ptr<RoomRepository> sharedRoomRepo = std::move(roomRepo);
        std::shared_ptr<GameRepository> sharedGameRepo = std::move(gameRepo);
        spdlog::info("레포지토리 객체 생성 및 포인터화 완료");

        // 서비스 생성
        auto authService = AuthService::create(sharedUserRepo);
        auto roomService = RoomService::create(sharedRoomRepo);
        auto gameService = GameService::create(sharedGameRepo);
        spdlog::info("레포지토리와 서비스 연동 및 서비스 객체 생성 완료");

        // 컨트롤러 생성 및 등록
        controllers_["auth"] = std::make_shared<AuthController>(std::move(authService));
        controllers_["room"] = std::make_shared<RoomController>(std::move(roomService));
        controllers_["game"] = std::make_shared<GameController>(std::move(gameService));
        spdlog::info("서비스와 컨트롤러 연동 및 컨트롤러 객체 생성, 핸들러 할당 완료");
    }

    void Server::run()
    {
        running_ = true;
        do_accept();
        startSessionTimeoutCheck();
        startBroadcastTimer();
        spdlog::info("서버 실행 완료, 클라이언트 연결 요청을 기다리는 중...");
    }

    void Server::stop() {
        if (!running_) return;  // 이미 중지된 경우 중복 실행 방지

        running_ = false;
        timeout_check_running_ = false;
        broadcast_running_ = false;

        // 타이머 취소 및 대기
        session_check_timer_.cancel();
        broadcast_timer_.cancel();

        // 모든 세션에 종료 알림
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            for (auto& [token, wsession] : sessions_) {
                auto session = wsession.lock();
                try {
                    if (session) {
                        session->handle_error("서버 중단으로 인한 연결 종료");
                    }
                }
                catch (const std::exception& e) {
                    spdlog::error("세션을 정리하던 중 에러가 발생하였습니다. : {}", e.what());
                }
            }
            sessions_.clear();
        }

        // acceptor 닫기
        try {
            if (acceptor_.is_open()) {
                acceptor_.close();
            }
        }
        catch (const std::exception& e) {
            spdlog::error("서버 종료 중 에러가 발생하였습니다. : {}", e.what());
        }

        spdlog::info("서버 중단");
    }

    bool Server::allowConnection(const std::string& ipAddress) {
        std::lock_guard<std::mutex> lock(connected_ips_mutex_);
        if (connected_ips_.count(ipAddress)) {
            return false;
        }
        connected_ips_.insert(ipAddress);
        return true;
    }

    void Server::removeConnection(const std::string& ipAddress) {
        std::lock_guard<std::mutex> lock(connected_ips_mutex_);
        connected_ips_.erase(ipAddress);
    }

    void Server::do_accept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    // 세션 생성 및 시작
                    auto session = std::make_shared<Session>(std::move(socket), controllers_, this);
                    session->start();
                }
                else {
                    spdlog::error("클라이언트 연결을 받아 들이던 중 에러가 발생하였습니다. : {}", ec.message());
                }

                // 계속해서 연결 수락 (서버가 여전히 실행 중인 경우)
                if (running_) {
                    do_accept();
                }
            }
        );
    }

} // namespace game_server