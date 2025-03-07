#pragma once

#include "api_handler.h"
#include "../db/db_pool.h"
#include <string>
#include <nlohmann/json.hpp>

namespace game_server {

    class AuthApi : public ApiHandler {
    public:
        explicit AuthApi(DbPool* db_pool);

        std::string handle_request(
            const nlohmann::json& request,
            int user_id,
            const std::string& auth_token) override;

    private:
        std::string handle_register(const nlohmann::json& request);
        std::string handle_login(const nlohmann::json& request);
        std::string generate_auth_token(int user_id, const std::string& username);

        // Password hashing function (should use a secure library in production)
        std::string hash_password(const std::string& password);

        DbPool* db_pool_;
    };

} // namespace game_server