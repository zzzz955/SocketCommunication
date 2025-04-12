// controller/room_controller.cpp
// 방 컨트롤러 구현 파일
// 방 생성, 참가, 목록 조회 등의 요청을 처리하는 컨트롤러
#include "room_controller.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    RoomController::RoomController(std::shared_ptr<RoomService> roomService)
        : roomService_(roomService) {
    }

    nlohmann::json RoomController::handleRequest(json& request) {
        // 요청의 action 필드에 따라 적절한 핸들러 호출
        std::string action = request["action"];

        if (action == "createRoom") {
            return handleCreateRoom(request);
        }
        else if (action == "joinRoom") {
            return handleJoinRoom(request);
        }
        else if (action == "exitRoom") {
            return handleExitRoom(request);
        }
        else if (action == "listRooms") {
            return handleListRooms(request);
        }
        else {
            json error_response = {
                {"status", "error"},
                {"message", "알 수 없는 방 액션"}
            };
            return error_response;
        }
    }

    nlohmann::json RoomController::handleCreateRoom(json& request) {
        json response = roomService_->createRoom(request);
        return response;
    }

    nlohmann::json RoomController::handleJoinRoom(json& request) {
        json response = roomService_->joinRoom(request);
        return response;
    }

    nlohmann::json RoomController::handleExitRoom(json& request) {
        json response = roomService_->exitRoom(request);
        return response;
    }

    nlohmann::json RoomController::handleListRooms(json& request) {
        auto response = roomService_->listRooms();
        return response;
    }

} // namespace game_server