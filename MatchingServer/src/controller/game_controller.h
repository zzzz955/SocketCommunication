#pragma once
#include "controller.h"
#include "../service/game_service.h"
#include <memory>

namespace game_server {

    class GameController : public Controller {
    public:
        explicit GameController(std::shared_ptr<GameService> gameService);
        ~GameController() override = default;

        nlohmann::json handleRequest(nlohmann::json& request) override;

    private:
        nlohmann::json handleStartGame(nlohmann::json& request);
        nlohmann::json handleEndGame(nlohmann::json& request);

        std::shared_ptr<GameService> gameService_;
    };

} // namespace game_server
