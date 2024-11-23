#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <string>

using namespace boost;

class ChatClient {
public:
    ChatClient(asio::io_context& ios, const std::string& host, unsigned short port)
        : m_socket(ios), m_resolver(ios) {
        asio::ip::tcp::resolver::query query(host, std::to_string(port));
        m_endpoints = m_resolver.resolve(query);
    }

    void connect(const std::string nickname) {
        asio::async_connect(m_socket, m_endpoints,
            [this, nickname](const system::error_code& ec, const asio::ip::tcp::endpoint&) {
                if (!ec) {
                    std::cout << "서버 연결 완료.\n";
                    sendMessage(nickname);
                    readMessage();
                }
                else {
                    std::cerr << "에러 발생 : " << ec.message() << "\n";
                }
            });
    }

    void sendMessage(const std::string& message) {
        auto msg = std::make_shared<std::string>(message + "\n"); // 문자열 관리
        asio::async_write(m_socket, asio::buffer(*msg),
            [this, msg](const system::error_code& ec, std::size_t bytes_transferred) {
                if (ec) {
                    std::cerr << "메시지 전송 실패 : " << ec.message() << "\n";
                }
            });
    }


private:
    void readMessage() {
        asio::async_read_until(m_socket, m_buffer, '\n',
            [this](const system::error_code& ec, std::size_t bytes_transferred) {
                if (!ec) {
                    // 스트림 버퍼에서 데이터를 안전하게 읽기
                    std::string message{ buffers_begin(m_buffer.data()), buffers_begin(m_buffer.data()) + bytes_transferred - 1 };
                    m_buffer.consume(bytes_transferred); // 버퍼 정리
                    std::cout << message << "\n";

                    readMessage();  // 재귀 호출을 통해 계속 메시지를 받기.
                }
                else {
                    std::cerr << "메시지 수신 실패 : " << ec.message() << "\n";
                }
            });
    }

    asio::ip::tcp::socket m_socket;
    asio::ip::tcp::resolver m_resolver;
    asio::ip::tcp::resolver::results_type m_endpoints;
    asio::streambuf m_buffer;
};

int main() {
    const std::string server_address = "3.35.173.11";
    const unsigned short server_port = 12345;

    try {
        asio::io_context ios;

        std::cout << "사용할 닉네임을 입력해 주세요 : ";
        std::string nickname;
        std::getline(std::cin, nickname);

        ChatClient client(ios, server_address, server_port);
        client.connect(nickname);

        std::thread client_thread([&ios]() { ios.run(); });

        std::string message;
        while (true) {
            std::getline(std::cin, message);
            if (message == "/quit") {
                break;
            }
            client.sendMessage(message);
        }

        ios.stop();
        client_thread.join();
    }
    catch (const std::exception& e) {
        std::cerr << "에러 발생 : " << e.what() << "\n";
    }

    return 0;
}
