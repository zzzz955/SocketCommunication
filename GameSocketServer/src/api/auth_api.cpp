#include "auth_api.h"
#include <spdlog/spdlog.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <pqxx/pqxx>

namespace game_server {

    using json = nlohmann::json;

    AuthApi::AuthApi(DbPool* db_pool)
        : db_pool_(db_pool)
    {
    }

    std::string AuthApi::handle_request(
        const json& request,
        int user_id,
        const std::string& auth_token)
    {
        std::string action = request["action"];

        if (action == "register") {
            return handle_register(request);
        }
        else if (action == "login") {
            return handle_login(request);
        }
        else {
            json error_response = {
                {"status", "error"},
                {"message", "Unknown auth action"}
            };
            return error_response.dump();
        }
    }

    std::string AuthApi::handle_register(const json& request)
    {
        try {
            std::string username = request["username"];
            std::string password = request["password"];

            // Validate username
            if (username.empty() || username.length() < 3 || username.length() > 20) {
                json error_response = {
                    {"status", "error"},
                    {"message", "Username must be between 3 and 20 characters"}
                };
                return error_response.dump();
            }

            // Validate password
            if (password.empty() || password.length() < 6) {
                json error_response = {
                    {"status", "error"},
                    {"message", "Password must be at least 6 characters"}
                };
                return error_response.dump();
            }

            // Hash password
            // NOTE: In production, use a more secure algorithm like bcrypt or Argon2
            std::string hashed_password = hash_password(password);

            // Get database connection
            auto conn = db_pool_->get_connection();

            // Check for username duplicate
            pqxx::work txn(*conn);
            pqxx::result check_user = txn.exec_params(
                "SELECT user_id FROM Users WHERE username = $1",
                username);

            if (!check_user.empty()) {
                txn.abort();
                json error_response = {
                    {"status", "error"},
                    {"message", "Username already exists"}
                };
                return error_response.dump();
            }

            // Register new user
            pqxx::result result = txn.exec_params(
                "INSERT INTO Users (username, password, created_at) "
                "VALUES ($1, $2, CURRENT_TIMESTAMP) RETURNING user_id",
                username, hashed_password);

            txn.commit();

            // Create success response
            int user_id = result[0][0].as<int>();
            json success_response = {
                {"status", "success"},
                {"message", "Registration successful"},
                {"user_id", user_id},
                {"username", username}
            };

            spdlog::info("New user registered: {} (ID: {})", username, user_id);
            return success_response.dump();

        }
        catch (const pqxx::sql_error& e) {
            spdlog::error("SQL error during registration: {}", e.what());
            json error_response = {
                {"status", "error"},
                {"message", "Database error during registration"}
            };
            return error_response.dump();
        }
        catch (const std::exception& e) {
            spdlog::error("Error during registration: {}", e.what());
            json error_response = {
                {"status", "error"},
                {"message", "Internal server error"}
            };
            return error_response.dump();
        }
    }

    std::string AuthApi::handle_login(const json& request)
    {
        try {
            std::string username = request["username"];
            std::string password = request["password"];

            // Hash password
            std::string hashed_password = hash_password(password);

            // Get database connection
            auto conn = db_pool_->get_connection();

            // Authenticate user
            pqxx::work txn(*conn);
            pqxx::result result = txn.exec_params(
                "SELECT user_id, username, rating, total_games, total_wins "
                "FROM Users WHERE username = $1 AND password = $2",
                username, hashed_password);

            if (result.empty()) {
                txn.abort();
                json error_response = {
                    {"status", "error"},
                    {"message", "Invalid username or password"}
                };
                return error_response.dump();
            }

            // Update last_login
            int user_id = result[0][0].as<int>();
            txn.exec_params(
                "UPDATE Users SET last_login = CURRENT_TIMESTAMP WHERE user_id = $1",
                user_id);

            txn.commit();

            // Generate auth token
            std::string auth_token = generate_auth_token(user_id, username);

            // Create success response
            json success_response = {
                {"status", "success"},
                {"message", "Login successful"},
                {"user_id", user_id},
                {"username", result[0][1].as<std::string>()},
                {"rating", result[0][2].as<int>()},
                {"total_games", result[0][3].as<int>()},
                {"total_wins", result[0][4].as<int>()},
                {"token", auth_token}
            };

            spdlog::info("User logged in: {} (ID: {})", username, user_id);
            return success_response.dump();

        }
        catch (const pqxx::sql_error& e) {
            spdlog::error("SQL error during login: {}", e.what());
            json error_response = {
                {"status", "error"},
                {"message", "Database error during login"}
            };
            return error_response.dump();
        }
        catch (const std::exception& e) {
            spdlog::error("Error during login: {}", e.what());
            json error_response = {
                {"status", "error"},
                {"message", "Internal server error"}
            };
            return error_response.dump();
        }
    }

    std::string AuthApi::generate_auth_token(int user_id, const std::string& username)
    {
        // NOTE: In production, use a proper JWT or token library
        // This is a simplified example

        std::stringstream ss;
        std::time_t now = std::time(nullptr);

        // Create payload
        json payload = {
            {"user_id", user_id},
            {"username", username},
            {"exp", now + 3600}, // Expires in 1 hour
            {"iat", now}
        };

        // Simple hashing (use a secure library in production)
        unsigned char hmac[SHA256_DIGEST_LENGTH];

        // SECURITY NOTE: In production, this key should be stored securely in env variables
        std::string secret_key = "your_secret_key_here";
        std::string data_to_hash = payload.dump();

        HMAC(EVP_sha256(), secret_key.c_str(), secret_key.length(),
            reinterpret_cast<const unsigned char*>(data_to_hash.c_str()), data_to_hash.length(),
            hmac, nullptr);

        // Convert binary to hex string
        std::stringstream signature;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            signature << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hmac[i]);
        }

        // Create token: base64(header).base64(payload).base64(signature)
        // Simplified for this example: JSON payload + signature
        return payload.dump() + "." + signature.str();
    }

    std::string AuthApi::hash_password(const std::string& password)
    {
        // SECURITY NOTE: In production, use bcrypt, scrypt, or Argon2 for secure password hashing
        // This is a basic SHA-256 example only suitable for demonstration
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.length(), hash);

        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }

        return ss.str();
    }

} // namespace game_server