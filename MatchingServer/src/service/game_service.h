#pragma once
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace game_server {

    class GameRepository;

    class GameService {
    public:
        virtual ~GameService() = default;

        virtual nlohmann::json startGame(nlohmann::json& request) = 0;
        virtual nlohmann::json endGame(nlohmann::json& request) = 0;

        static std::unique_ptr<GameService> create(std::shared_ptr<GameRepository> roomRepo);
    };

} // namespace game_server
