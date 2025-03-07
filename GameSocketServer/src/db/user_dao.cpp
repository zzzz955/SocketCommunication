#include "user_dao.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

namespace game_server {

    UserDao::UserDao(DbPool* db_pool)
        : db_pool_(db_pool)
    {
    }

    std::optional<UserInfo> UserDao::find_by_id(int user_id)
    {
        try {
            auto conn = db_pool_->get_connection();
            pqxx::work txn(*conn);

            pqxx::result result = txn.exec_params(
                "SELECT user_id, username, rating, total_games, total_wins, "
                "       created_at, last_login "
                "FROM Users WHERE user_id = $1",
                user_id);

            txn.commit();

            if (result.empty()) {
                return std::nullopt;
            }

            UserInfo user;
            user.user_id = result[0][0].as<int>();
            user.username = result[0][1].as<std::string>();
            user.rating = result[0][2].as<int>();
            user.total_games = result[0][3].as<int>();
            user.total_wins = result[0][4].as<int>();
            user.created_at = result[0][5].as<std::string>();

            // Handle potential NULL for last_login
            if (!result[0][6].is_null()) {
                user.last_login = result[0][6].as<std::string>();
            }

            return user;
        }
        catch (const std::exception& e) {
            spdlog::error("Error finding user by ID: {}", e.what());
            return std::nullopt;
        }
    }

    std::optional<UserInfo> UserDao::find_by_username(const std::string& username)
    {
        try {
            auto conn = db_pool_->get_connection();
            pqxx::work txn(*conn);

            pqxx::result result = txn.exec_params(
                "SELECT user_id, username, rating, total_games, total_wins, "
                "       created_at, last_login "
                "FROM Users WHERE username = $1",
                username);

            txn.commit();

            if (result.empty()) {
                return std::nullopt;
            }

            UserInfo user;
            user.user_id = result[0][0].as<int>();
            user.username = result[0][1].as<std::string>();
            user.rating = result[0][2].as<int>();
            user.total_games = result[0][3].as<int>();
            user.total_wins = result[0][4].as<int>();
            user.created_at = result[0][5].as<std::string>();

            // Handle potential NULL for last_login
            if (!result[0][6].is_null()) {
                user.last_login = result[0][6].as<std::string>();
            }

            return user;
        }
        catch (const std::exception& e) {
            spdlog::error("Error finding user by username: {}", e.what());
            return std::nullopt;
        }
    }

    int UserDao::create(const std::string& username, const std::string& hashed_password)
    {
        try {
            auto conn = db_pool_->get_connection();
            pqxx::work txn(*conn);

            pqxx::result result = txn.exec_params(
                "INSERT INTO Users (username, password, created_at) "
                "VALUES ($1, $2, CURRENT_TIMESTAMP) RETURNING user_id",
                username, hashed_password);

            txn.commit();

            if (result.empty()) {
                return -1;
            }

            return result[0][0].as<int>();
        }
        catch (const std::exception& e) {
            spdlog::error("Error creating user: {}", e.what());
            return -1;
        }
    }

    bool UserDao::update_rating(int user_id, int new_rating)
    {
        try {
            auto conn = db_pool_->get_connection();
            pqxx::work txn(*conn);

            pqxx::result result = txn.exec_params(
                "UPDATE Users SET rating = $1 WHERE user_id = $2 RETURNING user_id",
                new_rating, user_id);

            txn.commit();

            return !result.empty();
        }
        catch (const std::exception& e) {
            spdlog::error("Error updating user rating: {}", e.what());
            return false;
        }
    }

    bool UserDao::update_stats(int user_id, bool is_win)
    {
        try {
            auto conn = db_pool_->get_connection();
            pqxx::work txn(*conn);

            std::string query;
            if (is_win) {
                query = "UPDATE Users SET total_games = total_games + 1, "
                    "                 total_wins = total_wins + 1 "
                    "WHERE user_id = $1 RETURNING user_id";
            }
            else {
                query = "UPDATE Users SET total_games = total_games + 1 "
                    "WHERE user_id = $1 RETURNING user_id";
            }

            pqxx::result result = txn.exec_params(query, user_id);

            txn.commit();

            return !result.empty();
        }
        catch (const std::exception& e) {
            spdlog::error("Error updating user stats: {}", e.what());
            return false;
        }
    }

    bool UserDao::update_last_login(int user_id)
    {
        try {
            auto conn = db_pool_->get_connection();
            pqxx::work txn(*conn);

            pqxx::result result = txn.exec_params(
                "UPDATE Users SET last_login = CURRENT_TIMESTAMP "
                "WHERE user_id = $1 RETURNING user_id",
                user_id);

            txn.commit();

            return !result.empty();
        }
        catch (const std::exception& e) {
            spdlog::error("Error updating last login: {}", e.what());
            return false;
        }
    }

    std::vector<UserInfo> UserDao::get_top_players(int limit)
    {
        std::vector<UserInfo> users;

        try {
            auto conn = db_pool_->get_connection();
            pqxx::work txn(*conn);

            pqxx::result result = txn.exec_params(
                "SELECT user_id, username, rating, total_games, total_wins, "
                "       created_at, last_login "
                "FROM Users ORDER BY rating DESC LIMIT $1",
                limit);

            txn.commit();

            for (const auto& row : result) {
                UserInfo user;
                user.user_id = row[0].as<int>();
                user.username = row[1].as<std::string>();
                user.rating = row[2].as<int>();
                user.total_games = row[3].as<int>();
                user.total_wins = row[4].as<int>();
                user.created_at = row[5].as<std::string>();

                // Handle potential NULL for last_login
                if (!row[6].is_null()) {
                    user.last_login = row[6].as<std::string>();
                }

                users.push_back(user);
            }
        }
        catch (const std::exception& e) {
            spdlog::error("Error getting top players: {}", e.what());
        }

        return users;
    }

} // namespace game_server