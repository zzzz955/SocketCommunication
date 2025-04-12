// util/password_util.h
#pragma once
#include <string>

namespace game_server {

    class PasswordUtil {
    public:
        static std::string hashPassword(const std::string& password);

        static bool verifyPassword(const std::string& password, const std::string& hashedPassword);
    };

} // namespace game_server
