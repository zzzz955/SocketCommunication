#include <boost/asio.hpp>
#include <thread>
#include <memory>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <sstream>

using namespace boost;

class Client {
public:
    Client(std::shared_ptr<asio::ip::tcp::socket> socket, std::string nickname)
        : m_socket(socket),
        m_nickname(std::move(nickname)) {}

    std::shared_ptr<asio::ip::tcp::socket> getSocket() const {
        return m_socket;
    }

    std::string getNickname() const {
        return m_nickname;
    }

private:
    std::shared_ptr<asio::ip::tcp::socket> m_socket;
    std::string m_nickname;
};

class Service : public std::enable_shared_from_this<Service> {
public:
    Service(std::shared_ptr<asio::ip::tcp::socket> sock, std::unordered_map<std::string, std::shared_ptr<Client>>& clients, std::mutex& clients_mutex)
        : m_sock(sock),
        m_clients(clients),
        m_clients_mutex(clients_mutex) {}

    // 요청을 비동기적으로 읽어들이고 처리 시작
    void StartHandling() {
        // 'shared_ptr'을 비동기 콜백에 전달
        auto self = shared_from_this();

        asio::async_read_until(*m_sock, m_request, '\n',
            [self](const system::error_code& ec, std::size_t bytes_transferred) {
                self->onRequestReceived(ec, bytes_transferred);
            });
    }

private:
    // 요청을 받은 후 처리하는 함수
    void onRequestReceived(const system::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cerr << "오류 발생! 오류 코드 = "
                << ec.value() << ". 메세지: " << ec.message() << "\n";
            return;
        }

        std::istream request_stream(&m_request);
        std::string message;
        std::getline(request_stream, message);

        if (!m_nickname.empty()) {
            broadcastMessage(m_nickname + ": " + message);
        }
        else {
            m_nickname = message;
            if (!registerClient()) return;
            broadcastMessage(m_nickname + "님이 입장하셨습니다.");
        }

        StartHandling();
    }

    void broadcastMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(m_clients_mutex);

        for (const auto& pair : m_clients) {
            auto client = pair.second;
            auto sock = client->getSocket();
            asio::async_write(*sock,
                asio::buffer(message + "\n"),
                [](const system::error_code& ec, std::size_t bytes_trasnferred) {
                    if (ec) {
                        std::cerr << "오류 발생! 오류 코드 = "
                            << ec.value() << ". 메세지: " << ec.message() << "\n";
                    }
                });
        }
    }

    bool registerClient() {
        std::lock_guard<std::mutex> lock(m_clients_mutex);

        if (m_clients.find(m_nickname) == m_clients.end()) {
            m_clients[m_nickname] = std::make_shared<Client>(m_sock, m_nickname);
        }
        else {
            asio::async_write(*m_sock, asio::buffer("해당 닉네임이 이미 사용중입니다, 다시 시도해 주세요\n"),
                [](const system::error_code& ec, std::size_t bytes_transferred) {});
            m_sock->close();
            return false;
        }
        return true;
    }

private:
    std::shared_ptr<asio::ip::tcp::socket> m_sock;  // 클라이언트 소켓
    asio::streambuf m_request;  // 요청을 담을 버퍼
    std::string m_nickname;
    std::unordered_map<std::string, std::shared_ptr<Client>>& m_clients;
    std::mutex& m_clients_mutex;
};

class Acceptor {
public:
    Acceptor(asio::io_context& ios, unsigned short port_num, std::unordered_map<std::string, std::shared_ptr<Client>>& clients, std::mutex& clients_mutex)
        : m_ios(ios),
        m_acceptor(m_ios, asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port_num)),
        m_clients(clients),
        m_clients_mutex(clients_mutex) {}

    void Start() {
        doAccept();
    }

    void Stop() {
        m_acceptor.close();
    }

private:
    void doAccept() {
        auto sock = std::make_shared<asio::ip::tcp::socket>(m_ios);

        m_acceptor.async_accept(*sock,
            [this, sock](const system::error_code& ec) {
                if (!ec) {
                    std::make_shared<Service>(sock, m_clients, m_clients_mutex)->StartHandling();
                }
                else {
                    std::cerr << "오류 발생! 오류 코드 = "
                        << ec.value() << ". 메세지: " << ec.message() << "\n";
                }

                if (m_acceptor.is_open()) {
                    doAccept();
                }
            });
    }

private:
    asio::io_context& m_ios;
    asio::ip::tcp::acceptor m_acceptor;
    std::unordered_map<std::string, std::shared_ptr<Client>>& m_clients;
    std::mutex& m_clients_mutex;
};

class Server {
public:
    void Start(unsigned short port_num, unsigned int thread_pool_size) {
        assert(thread_pool_size > 0);

        acc = std::make_unique<Acceptor>(m_ios, port_num, clients, clients_mutex);
        acc->Start();

        for (unsigned int i = 0; i < thread_pool_size; ++i) {
            thread_pool.emplace_back([this]() { m_ios.run(); });
        }
    }

    void Stop() {
        acc->Stop();
        m_ios.stop();

        for (auto& thread : thread_pool) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    asio::io_context m_ios;
    std::unique_ptr<Acceptor> acc;
    std::vector<std::thread> thread_pool;
    std::unordered_map<std::string, std::shared_ptr<Client>> clients;
    std::mutex clients_mutex;
};

const unsigned int DEFAULT_THREAD_POOL_SIZE = 2;

int main() {
    const unsigned short port_num = 12345;

    try {
        Server srv;

        unsigned int thread_pool_size = std::thread::hardware_concurrency() * 2;
        if (thread_pool_size == 0) {
            thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
        }

        srv.Start(port_num, thread_pool_size);
        while (1);
        srv.Stop();
    }
    catch (const std::exception& e) {
        std::cerr << "오류 발생! 내용: " << e.what() << "\n";
    }

    return 0;
}
