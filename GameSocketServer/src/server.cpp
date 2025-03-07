#include "server.h"
#include "session.h"
#include "api/auth_api.h"
#include <iostream>

namespace game_server {

    Server::Server(boost::asio::io_context& io_context,
        short port,
        const std::string& db_connection_string)
        : io_context_(io_context),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
        running_(false)
    {
        // 데이터베이스 연결 풀 초기화
        db_pool_ = std::make_unique<DbPool>(db_connection_string, 5); // 5개의 연결 생성

        // API 핸들러 등록
        api_handlers_["auth"] = std::make_shared<AuthApi>(db_pool_.get());

        std::cout << "Server initialized on port " << port << std::endl;
    }

    Server::~Server()
    {
        if (running_) {
            stop();
        }
    }

    void Server::run()
    {
        running_ = true;
        do_accept();
        std::cout << "Server is running and accepting connections..." << std::endl;
    }

    void Server::stop()
    {
        running_ = false;
        acceptor_.close();
        std::cout << "Server stopped" << std::endl;
    }

    void Server::do_accept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    // 새 세션 생성 및 시작
                    std::make_shared<Session>(std::move(socket), api_handlers_)->start();
                }

                // 계속해서 연결 수락 (서버가 실행 중인 경우)
                if (running_) {
                    do_accept();
                }
            }
        );
    }

} // namespace game_server