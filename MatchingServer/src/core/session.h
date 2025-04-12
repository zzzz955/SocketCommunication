// core/session.h
#pragma once
#include "../controller/controller.h"
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <array>
#include <map>
#include <nlohmann/json.hpp>

namespace game_server {

    using json = nlohmann::json;
    class Server;

    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(boost::asio::ip::tcp::socket socket,
            std::map<std::string, std::shared_ptr<Controller>>& controllers,
            Server* server);
        ~Session();

        void start();
        const std::string& getToken() const;
        void initialize();
        void handlePing();
        bool isActive(std::chrono::seconds timeout) const;
        void handle_error(const std::string& error_message);
        void setToken(const std::string& token);
        int getUserId();
        std::string getUserNickName();
        void setStatus(const std:: string& status);
        std::string getStatus();
        void write_broadcast(const std::string& response);

    private:
        void read_message();
        void process_request(json& request);
        void write_response(const std::string& response);
        void init_current_user(const json& response);
        void read_handshake();
        void write_handshake_response(const std::string& response);
        void write_mirror(const std::string& response, std::shared_ptr<Session> mirror);

        boost::asio::ip::tcp::socket socket_;
        std::map<std::string, std::shared_ptr<Controller>>& controllers_;
        std::array<char, 8192> buffer_;
        std::string message_;
        int user_id_;
        std::string user_name_;
        std::string nick_name_;
        std::string status_;
        Server* server_;
        std::chrono::steady_clock::time_point last_activity_time_;
        std::string token_;
        bool is_mirror_ = false;
        int mirror_port_;
        std::string remote_ip_;
    };

} // namespace game_server
