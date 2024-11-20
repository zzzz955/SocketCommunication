#include <boost/asio.hpp>
#include <thread>
#include <memory>
#include <iostream>
#include <vector>

using namespace boost;

class Service {
public:
    Service(std::shared_ptr<asio::ip::tcp::socket> sock)
        : m_sock(sock) {}

    // 요청을 비동기적으로 읽어들이고 처리 시작
    void StartHandling() {
        // 'shared_ptr'을 비동기 콜백에 전달
        auto self = shared_ptr<Service>(this);  // 'this'를 shared_ptr로 감싸서 전달

        asio::async_read_until(*m_sock, m_request, '\n',
            [self](const system::error_code& ec, std::size_t bytes_transferred) {
                self->onRequestReceived(ec, bytes_transferred);
            });
    }

private:
    // 요청을 받은 후 처리하는 함수
    void onRequestReceived(const system::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cerr << "Error occurred! Error code = "
                << ec.value() << ". Message: " << ec.message() << "\n";
            return;
        }

        // 요청 처리
        m_response = ProcessRequest(m_request);

        // 비동기적으로 응답 전송
        auto self = shared_ptr<Service>(this);  // 응답도 동일하게 shared_ptr로 감싸서 전달
        asio::async_write(*m_sock, asio::buffer(m_response),
            [self](const system::error_code& ec, std::size_t bytes_transferred) {
                self->onResponseSent(ec, bytes_transferred);
            });
    }

    // 응답을 전송한 후의 처리
    void onResponseSent(const system::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cerr << "Error occurred! Error code = "
                << ec.value() << ". Message: " << ec.message() << "\n";
        }

        // 자동으로 shared_ptr가 cleanup을 처리함
    }

    // 요청 처리 (여기서는 단순한 작업을 시뮬레이션)
    std::string ProcessRequest(asio::streambuf& request) {
        int i = 0;
        while (i != 1000000)
            ++i;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        return "Response\n";  // 간단한 응답 반환
    }

private:
    std::shared_ptr<asio::ip::tcp::socket> m_sock;  // 클라이언트 소켓
    asio::streambuf m_request;  // 요청을 담을 버퍼
    std::string m_response;  // 응답 문자열
};

class Acceptor {
public:
    Acceptor(asio::io_context& ios, unsigned short port_num)
        : m_ios(ios),
        m_acceptor(m_ios, asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port_num)) {}

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
                    std::make_shared<Service>(sock)->StartHandling();
                }
                else {
                    std::cerr << "Error occurred! Error code = "
                        << ec.value() << ". Message: " << ec.message() << "\n";
                }

                if (m_acceptor.is_open()) {
                    doAccept();
                }
            });
    }

private:
    asio::io_context& m_ios;
    asio::ip::tcp::acceptor m_acceptor;
};

class Server {
public:
    void Start(unsigned short port_num, unsigned int thread_pool_size) {
        assert(thread_pool_size > 0);

        acc = std::make_unique<Acceptor>(m_ios, port_num);
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
};

const unsigned int DEFAULT_THREAD_POOL_SIZE = 2;

int main() {
    const unsigned short port_num = 3333;

    try {
        Server srv;

        unsigned int thread_pool_size = std::thread::hardware_concurrency() * 2;
        if (thread_pool_size == 0) {
            thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
        }

        srv.Start(port_num, thread_pool_size);

        std::this_thread::sleep_for(std::chrono::seconds(60));

        srv.Stop();
    }
    catch (const std::exception& e) {
        std::cerr << "Error occurred! Message: " << e.what() << "\n";
    }

    return 0;
}
