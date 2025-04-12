// service/room_service.h
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace game_server {

    class RoomRepository;

    class RoomService {
    public:
        virtual ~RoomService() = default;

        virtual nlohmann::json createRoom(nlohmann::json& request) = 0;
        virtual nlohmann::json joinRoom(nlohmann::json& request) = 0;
        virtual nlohmann::json exitRoom(nlohmann::json& request) = 0;
        virtual nlohmann::json listRooms() = 0;

        static std::unique_ptr<RoomService> create(std::shared_ptr<RoomRepository> roomRepo);
    };

} // namespace game_server
