#include "db_pool.h"

// Include standard library headers
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <stdexcept>

// Logging library
#include <spdlog/spdlog.h>

// PostgreSQL client library
#include <pqxx/pqxx>

namespace game_server {

    DbPool::DbPool(const std::string& connection_string, int pool_size)
        : connection_string_(connection_string)
    {
        try {
            connections_.reserve(pool_size);
            in_use_.reserve(pool_size);

            for (int i = 0; i < pool_size; ++i) {
                // Create new connection and add to pool
                auto conn = std::make_shared<pqxx::connection>(connection_string);
                connections_.push_back(conn);
                in_use_.push_back(false);

                spdlog::info("Created database connection {}/{}", i + 1, pool_size);
            }

            spdlog::info("Database connection pool initialized with {} connections", pool_size);
        }
        catch (const std::exception& e) {
            spdlog::error("Failed to initialize database connection pool: {}", e.what());
            throw;
        }
    }

    DbPool::~DbPool()
    {
        // Close all connections
        connections_.clear();
        spdlog::info("Database connection pool destroyed");
    }

    std::shared_ptr<pqxx::connection> DbPool::get_connection()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Find available connection
        for (size_t i = 0; i < connections_.size(); ++i) {
            if (!in_use_[i]) {
                in_use_[i] = true;

                // Check if connection is valid
                if (!connections_[i]->is_open()) {
                    spdlog::warn("Connection {} was closed, reconnecting...", i);
                    try {
                        connections_[i] = std::make_shared<pqxx::connection>(connection_string_);
                    }
                    catch (const std::exception& e) {
                        spdlog::error("Failed to reconnect: {}", e.what());
                        in_use_[i] = false;
                        throw;
                    }
                }

                return connections_[i];
            }
        }

        // If no available connections, create a new one
        spdlog::warn("No available connections in pool, creating a new one");
        try {
            auto conn = std::make_shared<pqxx::connection>(connection_string_);
            connections_.push_back(conn);
            in_use_.push_back(true);
            return conn;
        }
        catch (const std::exception& e) {
            spdlog::error("Failed to create new connection: {}", e.what());
            throw;
        }
    }

    void DbPool::return_connection(std::shared_ptr<pqxx::connection> conn)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (size_t i = 0; i < connections_.size(); ++i) {
            if (connections_[i] == conn) {
                in_use_[i] = false;
                return;
            }
        }

        spdlog::warn("Attempted to return a connection not from this pool");
    }

} // namespace game_server