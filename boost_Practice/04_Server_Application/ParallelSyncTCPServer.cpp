#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <memory>
#include <iostream>

using namespace boost;

class Service {
public:
    Service() {}

    void StartHandlingClient(std::shared_ptr<asio::ip::tcp::socket> sock) {
        std::thread th([this, sock]() {
            HandleClient(sock);
            });

        th.detach();  // �����带 �и��Ͽ� ��׶���� ó��
    }

private:
    void HandleClient(std::shared_ptr<asio::ip::tcp::socket> sock) {
        try {
            asio::streambuf request;
            asio::read_until(*sock, request, '\n');  // Ŭ���̾�Ʈ�κ��� �����͸� ����

            // ��û ó�� �ùķ��̼�
            int i = 0;
            while (i != 1000000) i++;

            std::this_thread::sleep_for(std::chrono::milliseconds(500));  // ��� ���

            // ���� ����
            std::string response = "Response\n";
            asio::write(*sock, asio::buffer(response));  // ������ Ŭ���̾�Ʈ���� ����
        }
        catch (system::system_error& e) {
            std::cout << "Error occurred! Error code = "
                << e.code() << ". Message: "
                << e.what() << std::endl;
        }

        // Clean-up
    }
};

class Acceptor {
public:
    Acceptor(asio::io_context& ios, unsigned short port_num) :
        m_ios(ios),
        m_acceptor(m_ios, asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port_num)) {
        m_acceptor.listen();
    }

    void Accept() {
        auto sock = std::make_shared<asio::ip::tcp::socket>(m_ios);

        m_acceptor.accept(*sock);  // ���� ����

        auto service = std::make_shared<Service>();
        service->StartHandlingClient(sock);  // Ŭ���̾�Ʈ ó�� ����
    }

private:
    asio::io_context& m_ios;
    asio::ip::tcp::acceptor m_acceptor;
};

class Server {
public:
    Server() : m_stop(false) {}

    void Start(unsigned short port_num) {
        m_thread.reset(new std::thread([this, port_num]() {
            Run(port_num);
            }));
    }

    void Stop() {
        m_stop.store(true);
        m_thread->join();  // ���� ���� �� ������ ���� ���
    }

private:
    void Run(unsigned short port_num) {
        Acceptor acc(m_ios, port_num);

        while (!m_stop.load()) {
            acc.Accept();  // Ŭ���̾�Ʈ�� ������ ��ٸ�
        }
    }

    std::unique_ptr<std::thread> m_thread;
    std::atomic<bool> m_stop;
    asio::io_context m_ios;  // �ֽ� ���������� io_context�� ���
};

int main() {
    unsigned short port_num = 3333;

    try {
        Server srv;
        srv.Start(port_num);

        std::this_thread::sleep_for(std::chrono::seconds(60));  // ���� 60�� ���� ����

        srv.Stop();  // ���� ����
    }
    catch (system::system_error& e) {
        std::cout << "Error occurred! Error code = "
            << e.code() << ". Message: "
            << e.what() << std::endl;
    }

    return 0;
}
