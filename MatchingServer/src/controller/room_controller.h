// controller/room_controller.h
#pragma once
#include "controller.h"
#include "../service/room_service.h"
#include <memory>

namespace game_server {

    class RoomController : public Controller {
    public:
        explicit RoomController(std::shared_ptr<RoomService> roomService);
        ~RoomController() override = default;

        nlohmann::json handleRequest(nlohmann::json& request) override;

    private:
        nlohmann::json handleCreateRoom(nlohmann::json& request);
        nlohmann::json handleJoinRoom(nlohmann::json& request);
        nlohmann::json handleExitRoom(nlohmann::json& request);
        nlohmann::json handleListRooms(nlohmann::json& request);

        std::shared_ptr<RoomService> roomService_;
    };

} // namespace game_server
