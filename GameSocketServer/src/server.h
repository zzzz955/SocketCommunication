#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <map>
#include <functional>
#include "db/db_pool.h"
#include "api/api_handler.h"

namespace game_server {

    class Session;

    class Server {
    public:
        Server(boost::asio::io_context& io_context,
            short port,
            const std::string& db_connection_string);

        ~Server();

        void run();
        void stop();

    private:
        void do_accept();

        boost::asio::io_context& io_context_;
        boost::asio::ip::tcp::acceptor acceptor_;
        std::unique_ptr<DbPool> db_pool_;
        std::map<std::string, std::shared_ptr<ApiHandler>> api_handlers_;
        bool running_;
    };

} // namespace game_server