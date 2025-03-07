#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <memory>

// Forward declaration to reduce header dependencies
namespace pqxx {
    class connection;
}

namespace game_server {

    class DbPool {
    public:
        DbPool(const std::string& connection_string, int pool_size);
        ~DbPool();

        // Get a connection from pool
        std::shared_ptr<pqxx::connection> get_connection();

        // Return a connection to pool
        void return_connection(std::shared_ptr<pqxx::connection> conn);

    private:
        std::string connection_string_;
        std::vector<std::shared_ptr<pqxx::connection>> connections_;
        std::vector<bool> in_use_;
        std::mutex mutex_;
    };

} // namespace game_server