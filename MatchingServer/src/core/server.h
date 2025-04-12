// core/server.h
#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>
#include "../controller/controller.h"
#include "../util/db_pool.h"

namespace game_server {

    using json = nlohmann::json;

    class Session;

    class Server {
    public:
        Server(boost::asio::io_context& io_context,
            short port,
            const std::string& db_connection_string,
            const std::string& version);
        ~Server();

        void run();
        void stop();

        // 세션 관리 메서드
        std::string registerSession(std::shared_ptr<Session> session);
        void registerMirrorSession(std::shared_ptr<Session> session, int port);
        void removeSession(const std::string& token, int userId);
        void removeMirrorSession(int port);
        std::shared_ptr<Session> getSession(const std::string& token);
        std::shared_ptr<Session> getMirrorSession(int port);
        int getCCU();
        int getRoomCapacity();
        std::string generateSessionToken();
        void setSessionTimeout(std::chrono::seconds timeout);
        void startSessionTimeoutCheck();
        void startBroadcastTimer();
        bool checkAlreadyLogin(int userId);
        std::string getServerVersion();
        std::vector<std::shared_ptr<Session>> getWaitingSessions();
        void broadcastCCU();
        void broadcastLogin(const std::string& nickName);
        void broadcastChat(const std::string& nickName, const std::string& message);
        void broadcastActiveUser(const std::string& message, const std::vector<std::shared_ptr<Session>>& activeSessions);
        void setSessionStatus(const json& users, bool flag);
        bool allowConnection(const std::string& ipAddress);
        void removeConnection(const std::string& ipAddress);
    private:
        void do_accept();
        void init_controllers();
        void check_inactive_sessions();
        void scheduleBroadcast();

        boost::asio::io_context& io_context_;
        boost::asio::ip::tcp::acceptor acceptor_;
        std::unique_ptr<DbPool> db_pool_;
        std::map<std::string, std::shared_ptr<Controller>> controllers_;
        bool running_;

        // 세션 관리 데이터
        std::unordered_map<int, std::weak_ptr<Session>> mirrors_;
        std::mutex mirrors_mutex_;
        std::unordered_map<std::string, std::weak_ptr<Session>> sessions_;
        std::mutex sessions_mutex_;
        std::unordered_set<std::string> connected_ips_;
        std::mutex connected_ips_mutex_;
        std::unordered_map<int, std::string> tokens_;
        std::mutex tokens_mutex_;
        boost::uuids::random_generator uuid_generator_;
        std::chrono::seconds session_timeout_{ 12 }; // 기본 12초
        boost::asio::steady_timer session_check_timer_;
        bool timeout_check_running_{ false };

        boost::asio::steady_timer broadcast_timer_;
        bool broadcast_running_ = false;
        const std::chrono::seconds broadcast_interval_ = std::chrono::seconds(3);
        
        // 버전 관리 데이터
        std::string version_;
    };

} // namespace game_server
