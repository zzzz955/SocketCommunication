// controller/controller.h
#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace game_server {

    class Controller {
    public:
        virtual ~Controller() = default;

        virtual nlohmann::json handleRequest(nlohmann::json& request) = 0;
    };

} // namespace game_server
