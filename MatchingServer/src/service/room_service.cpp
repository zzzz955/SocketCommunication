#include "room_service.h"
#include "../repository/room_repository.h"
#include <spdlog/spdlog.h>
#include <random>
#include <string>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    namespace {
        // 방 이름 유효성 검증 함수
        bool isValidRoomName(const std::string& name) {
            // 빈 이름은 유효하지 않음
            if (name.empty()) {
                return false;
            }

            // 40바이트(UTF-8 표준) 이내인지 확인
            if (name.size() > 40) {
                return false;
            }

            // 영어, 한글, 숫자만 포함하는지 확인
            int len = name.size();
            for (int i = 0; i < len; ++i) {
                // ASCII 영어와 숫자 확인
                const char& c = name[i];
                if (i == len - 1 && c == '$') continue;
                if ((c >= 'A' && c <= 'Z') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') ||
                    (c == ' ')) {
                    continue;
                }

                // UTF-8 한글 범위 확인 (첫 바이트가 0xEA~0xED 범위)
                if ((c & 0xF0) == 0xE0) {
                    // 한글 문자의 첫 바이트 가능성, 좀 더 정확한 확인 필요
                    continue;
                }

                // 한글 문자의 연속 바이트 (0x80~0xBF 범위)
                if ((c & 0xC0) == 0x80) {
                    continue;
                }

                // 허용되지 않는 문자
                return false;
            }

            return true;
        }
    }

    // 서비스 구현체
    class RoomServiceImpl : public RoomService {
    public:
        explicit RoomServiceImpl(std::shared_ptr<RoomRepository> roomRepo)
            : roomRepo_(roomRepo) {
        }

        json createRoom(json& request) override {
            json response;

            try {
                // 요청 유효성 검증
                if (!request.contains("roomName") || !request.contains("userId") || !request.contains("maxPlayers")) {
                    response["status"] = "error";
                    response["message"] = "방 생성 요청에 필수 필드가 누락되었습니다";
                    return response;
                }

                if (!isValidRoomName(request["roomName"])) {
                    response["status"] = "error";
                    response["message"] = "방 이름은 1-40바이트 길이여야 하며 영어, 한글, 숫자만 포함해야 합니다";
                    return response;
                }

                if (request["maxPlayers"] < 2 || request["maxPlayers"] > 8) {
                    response["status"] = "error";
                    response["message"] = "최대 플레이어 수는 2~8 사이여야 합니다";
                    return response;
                }

                // 단일 트랜잭션으로 방 생성 및 호스트 추가
                json result = roomRepo_->createRoomWithHost(
                    request["userId"], request["roomName"], request["maxPlayers"]);
                if (result["roomId"] == -1) {
                    response["status"] = "error";
                    response["message"] = "방 생성에 실패했습니다";
                    return response;
                }

                // 성공 응답 생성
                response["action"] = "createRoom";
                response["status"] = "success";
                response["message"] = "방이 성공적으로 생성되었습니다";
                response["roomId"] = result["roomId"];
                response["roomName"] = result["roomName"];
                response["maxPlayers"] = result["maxPlayers"];
                response["ipAddress"] = result["ipAddress"];
                response["port"] = result["port"];

                spdlog::info("사용자 {}가 새 방을 생성했습니다: {} (ID: {})",
                    request["userId"].get<int>(), request["roomName"].get<std::string>(), result["roomId"].get<int>());
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("방 생성 오류: ") + e.what();
                spdlog::error("createRoom 오류: {}", e.what());
            }

            return response;
        }

        json joinRoom(json& request) override {
            json response;

            try {
                // 요청 유효성 검증
                if (!request.contains("roomId") || !request.contains("userId")) {
                    response["status"] = "error";
                    response["message"] = "방 참가 요청에 필수 필드가 누락되었습니다";
                    return response;
                }

                int roomId = request["roomId"];
                int userId = request["userId"];

                // 방에 참가자 추가
                if (!roomRepo_->addPlayer(roomId, userId)) {
                    response["status"] = "error";
                    response["message"] = "방 참가에 실패했습니다 - 방이 가득 찼거나 WAITING 상태가 아닙니다";
                    return response;
                }

                // 성공 응답 생성
                response["action"] = "joinRoom";
                response["roomId"] = roomId;
                response["status"] = "success";
                response["message"] = "방에 성공적으로 참가했습니다";

                spdlog::info("사용자 {}가 방 {}에 참가했습니다", userId, roomId);
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("방 참가 오류: ") + e.what();
                spdlog::error("joinRoom 오류: {}", e.what());
            }

            return response;
        }

        json exitRoom(json& request) override {
            json response;

            try {
                // 요청 유효성 검증
                if (!request.contains("userId")) {
                    response["status"] = "error";
                    response["message"] = "방 퇴장 요청에 userId가 누락되었습니다";
                    return response;
                }

                int userId = request["userId"];

                // 플레이어를 방에서 제거 
                if (!roomRepo_->removePlayer(userId)) {
                    response["status"] = "error";
                    response["message"] = "사용자가 어떤 방에도 없습니다";
                    return response;
                }

                // 성공 응답 생성
                response["action"] = "exitRoom";
                response["status"] = "success";
                response["message"] = "방에서 성공적으로 퇴장했습니다";

                spdlog::info("사용자 {}가 방에서 퇴장했습니다", userId);
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("방 퇴장 오류: ") + e.what();
                spdlog::error("exitRoom 오류: {}", e.what());
            }

            return response;
        }

        json listRooms() override {
            json response;

            try {
                // 열린 방 목록 가져오기
                auto rooms = roomRepo_->findAllOpen();

                // 응답 생성
                response["action"] = "listRooms";
                response["status"] = "success";
                response["message"] = "방 목록을 성공적으로 가져왔습니다";
                response["rooms"] = json::array();

                for (auto& room : rooms) {
                    room["currentPlayers"] = roomRepo_->getPlayerCount(room["roomId"]);
                    response["rooms"].push_back(room);
                }

                spdlog::debug("{}개의 열린 방을 조회했습니다", response["rooms"].size());
            }
            catch (const std::exception& e) {
                response["status"] = "error";
                response["message"] = std::string("방 목록 조회 오류: ") + e.what();
                spdlog::error("listRooms 오류: {}", e.what());
            }

            return response;
        }

    private:
        std::shared_ptr<RoomRepository> roomRepo_;
    };

    // 팩토리 메서드 구현
    std::unique_ptr<RoomService> RoomService::create(std::shared_ptr<RoomRepository> roomRepo) {
        return std::make_unique<RoomServiceImpl>(roomRepo);
    }

} // namespace game_server