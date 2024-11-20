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

        th.detach();  // 스레드를 분리하여 백그라운드로 처리
    }

private:
    void HandleClient(std::shared_ptr<asio::ip::tcp::socket> sock) {
        try {
            asio::streambuf request;
            asio::read_until(*sock, request, '\n');  // 클라이언트로부터 데이터를 읽음

            // 요청 처리 시뮬레이션
            int i = 0;
            while (i != 1000000) i++;

            std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 잠시 대기

            // 응답 전송
            std::string response = "Response\n";
            asio::write(*sock, asio::buffer(response));  // 응답을 클라이언트에게 전송
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

        m_acceptor.accept(*sock);  // 연결 수락

        auto service = std::make_shared<Service>();
        service->StartHandlingClient(sock);  // 클라이언트 처리 시작
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
        m_thread->join();  // 서버 종료 시 스레드 종료 대기
    }

private:
    void Run(unsigned short port_num) {
        Acceptor acc(m_ios, port_num);

        while (!m_stop.load()) {
            acc.Accept();  // 클라이언트의 연결을 기다림
        }
    }

    std::unique_ptr<std::thread> m_thread;
    std::atomic<bool> m_stop;
    asio::io_context m_ios;  // 최신 버전에서는 io_context를 사용
};

int main() {
    unsigned short port_num = 3333;

    try {
        Server srv;
        srv.Start(port_num);

        std::this_thread::sleep_for(std::chrono::seconds(60));  // 서버 60초 동안 실행

        srv.Stop();  // 서버 종료
    }
    catch (system::system_error& e) {
        std::cout << "Error occurred! Error code = "
            << e.code() << ". Message: "
            << e.what() << std::endl;
    }

    return 0;
}
