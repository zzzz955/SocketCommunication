// repository/room_repository.h
#pragma once
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>

namespace game_server {

    class DbPool;

    class RoomRepository {
    public:
        virtual ~RoomRepository() = default;

        virtual std::vector<nlohmann::json> findAllOpen() = 0;
        virtual nlohmann::json createRoomWithHost(int hostId, const std::string& roomName, int maxPlayers) = 0;
        virtual bool addPlayer(int roomId, int userId) = 0;
        virtual bool removePlayer(int userId) = 0;
        virtual int getPlayerCount(int roomId) = 0;
        virtual std::vector<int> getPlayersInRoom(int roomId) = 0;

        static std::unique_ptr<RoomRepository> create(DbPool* dbPool);
    };

} // namespace game_server
