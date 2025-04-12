// controller/auth_controller.h
#pragma once
#include "controller.h"
#include "../service/auth_service.h"
#include <memory>

namespace game_server {

    class AuthController : public Controller {
    public:
        explicit AuthController(std::shared_ptr<AuthService> authService);
        ~AuthController() override = default;

        nlohmann::json handleRequest(nlohmann::json& request) override;

    private:
        nlohmann::json handleRegister(nlohmann::json& request);
        nlohmann::json handleLogin(nlohmann::json& request);
        nlohmann::json handleRegisterCheckAndLogin(nlohmann::json& request);
        nlohmann::json handleUpdateNickName(nlohmann::json& request);

        std::shared_ptr<AuthService> authService_;
    };

} // namespace game_server
