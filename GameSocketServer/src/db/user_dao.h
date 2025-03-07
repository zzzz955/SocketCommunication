#pragma once

#include "db_pool.h"
#include <string>
#include <optional>
#include <vector>

namespace game_server {

    // User data structure
    struct UserInfo {
        int user_id;
        std::string username;
        int rating;
        int total_games;
        int total_wins;
        std::string created_at;
        std::string last_login;
    };

    // Data Access Object for User-related operations
    class UserDao {
    public:
        explicit UserDao(DbPool* db_pool);

        // Find user by ID
        std::optional<UserInfo> find_by_id(int user_id);

        // Find user by username
        std::optional<UserInfo> find_by_username(const std::string& username);

        // Create new user
        int create(const std::string& username, const std::string& hashed_password);

        // Update user rating
        bool update_rating(int user_id, int new_rating);

        // Update game statistics
        bool update_stats(int user_id, bool is_win);

        // Update last login time
        bool update_last_login(int user_id);

        // Get top players by rating
        std::vector<UserInfo> get_top_players(int limit = 10);

    private:
        DbPool* db_pool_;
    };

} // namespace game_server