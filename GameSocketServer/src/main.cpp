#include "server.h"
#include <boost/asio.hpp>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <csignal>

// Global server variable (for signal handler)
std::unique_ptr<game_server::Server> server;

// Signal handler function
void signal_handler(int signal)
{
    spdlog::info("Received signal {}, shutting down...", signal);
    if (server) {
        server->stop();
    }
    exit(signal);
}

int main(int argc, char* argv[])
{
    try {
        // Initialize logger
        auto console = spdlog::stdout_color_mt("console");
        spdlog::set_default_logger(console);
        spdlog::set_level(spdlog::level::info);
        spdlog::info("Starting Game Socket Server");

        // Register signal handler
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // Default settings
        short port = 8080;
        std::string db_connection_string =
            "dbname=GameData user=admin password=admin host=localhost port=5432";

        // Process command line arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--port" && i + 1 < argc) {
                port = std::stoi(argv[++i]);
            }
            else if (arg == "--db" && i + 1 < argc) {
                db_connection_string = argv[++i];
            }
            else if (arg == "--help") {
                std::cout << "Usage: " << argv[0] << " [options]\n"
                    << "Options:\n"
                    << "  --port PORT       Server port (default: 8080)\n"
                    << "  --db CONNSTRING   Database connection string\n"
                    << "  --help            Show this help message\n";
                return 0;
            }
        }

        // Create IO context and server
        boost::asio::io_context io_context;
        server = std::make_unique<game_server::Server>(
            io_context, port, db_connection_string);

        // Run server
        server->run();

        // Run IO context (event loop)
        spdlog::info("Server running on port {}", port);
        io_context.run();
    }
    catch (std::exception& e) {
        spdlog::error("Exception: {}", e.what());
        return 1;
    }

    return 0;
}