// controller/auth_controller.cpp
// 인증 컨트롤러 구현 파일
// 사용자 등록 및 로그인 요청을 처리하는 컨트롤러
#include "auth_controller.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;

    AuthController::AuthController(std::shared_ptr<AuthService> authService)
        : authService_(authService) {
    }

    nlohmann::json AuthController::handleRequest(json& request) {
        // 요청의 action 필드에 따라 적절한 핸들러 호출
        std::string action = request["action"];

        if (action == "register") {
            return handleRegister(request);
        }
        else if (action == "login") {
            return handleLogin(request);
        }
        else if (action == "SSAFYlogin") {
            return handleRegisterCheckAndLogin(request);
        }
        else if (action == "updateNickName") {
            return handleUpdateNickName(request);
        }
        else {
            json error_response = {
                {"status", "error"},
                {"message", "알 수 없는 인증 액션"}
            };
            return error_response;
        }
    }

    nlohmann::json AuthController::handleRegister(json& request) {
        // 서비스 계층 호출하여 사용자 등록 실행
        json response = authService_->registerUser(request);
        return response;
    }

    nlohmann::json AuthController::handleLogin(json& request) {
        json response = authService_->loginUser(request);
        return response;
    }

    nlohmann::json AuthController::handleRegisterCheckAndLogin(nlohmann::json& request) {
        json response = authService_->registerCheckAndLogin(request);
        return response;
    }

    nlohmann::json AuthController::handleUpdateNickName(nlohmann::json& request) {
        json response = authService_->updateNickName(request);
        return response;
    }

} // namespace game_server