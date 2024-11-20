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

    // ��û�� �񵿱������� �о���̰� ó�� ����
    void StartHandling() {
        // 'shared_ptr'�� �񵿱� �ݹ鿡 ����
        auto self = shared_ptr<Service>(this);  // 'this'�� shared_ptr�� ���μ� ����

        asio::async_read_until(*m_sock, m_request, '\n',
            [self](const system::error_code& ec, std::size_t bytes_transferred) {
                self->onRequestReceived(ec, bytes_transferred);
            });
    }

private:
    // ��û�� ���� �� ó���ϴ� �Լ�
    void onRequestReceived(const system::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cerr << "Error occurred! Error code = "
                << ec.value() << ". Message: " << ec.message() << "\n";
            return;
        }

        // ��û ó��
        m_response = ProcessRequest(m_request);

        // �񵿱������� ���� ����
        auto self = shared_ptr<Service>(this);  // ���䵵 �����ϰ� shared_ptr�� ���μ� ����
        asio::async_write(*m_sock, asio::buffer(m_response),
            [self](const system::error_code& ec, std::size_t bytes_transferred) {
                self->onResponseSent(ec, bytes_transferred);
            });
    }

    // ������ ������ ���� ó��
    void onResponseSent(const system::error_code& ec, std::size_t bytes_transferred) {
        if (ec) {
            std::cerr << "Error occurred! Error code = "
                << ec.value() << ". Message: " << ec.message() << "\n";
        }

        // �ڵ����� shared_ptr�� cleanup�� ó����
    }

    // ��û ó�� (���⼭�� �ܼ��� �۾��� �ùķ��̼�)
    std::string ProcessRequest(asio::streambuf& request) {
        int i = 0;
        while (i != 1000000)
            ++i;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        return "Response\n";  // ������ ���� ��ȯ
    }

private:
    std::shared_ptr<asio::ip::tcp::socket> m_sock;  // Ŭ���̾�Ʈ ����
    asio::streambuf m_request;  // ��û�� ���� ����
    std::string m_response;  // ���� ���ڿ�
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
