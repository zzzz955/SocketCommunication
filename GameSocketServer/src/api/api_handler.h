#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace game_server {

    // API handler interface
    class ApiHandler {
    public:
        virtual ~ApiHandler() = default;

        // Request handling method
        virtual std::string handle_request(
            const nlohmann::json& request,
            int user_id,
            const std::string& auth_token) = 0;
    };

} // namespace game_server