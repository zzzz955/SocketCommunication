#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

// 동기 TCP 클라이언트 클래스
class SyncTCPClient {
public:
    SyncTCPClient(const std::string& raw_ip_address,
        unsigned short port_num) :
        m_ep(asio::ip::make_address(raw_ip_address), port_num), // 최신 버전에서는 from_string 대신 make_address 사용
        m_sock(m_ios) {

        m_sock.open(m_ep.protocol());
    }

    // 서버에 연결하는 함수
    void connect() {
        m_sock.connect(m_ep);
    }

    // 연결을 종료하는 함수
    void close() {
        m_sock.shutdown(asio::ip::tcp::socket::shutdown_both);
        m_sock.close();
    }

    // 서버에 오래 걸리는 작업을 요청하고 응답을 받는 함수
    std::string emulateLongComputationOp(unsigned int duration_sec) {
        std::string request = "EMULATE_LONG_COMP_OP " + std::to_string(duration_sec) + "\n";
        sendRequest(request);
        return receiveResponse();
    }

private:
    // 서버에 요청을 보내는 함수
    void sendRequest(const std::string& request) {
        asio::write(m_sock, asio::buffer(request));
    }

    // 서버의 응답을 받는 함수
    std::string receiveResponse() {
        asio::streambuf buf;
        asio::read_until(m_sock, buf, '\n');

        std::istream input(&buf);
        std::string response;
        std::getline(input, response);

        return response;
    }

private:
    asio::io_context m_ios; // io_service 대신 io_context 사용

    asio::ip::tcp::endpoint m_ep;
    asio::ip::tcp::socket m_sock;
};

int main() {
    const std::string raw_ip_address = "127.0.0.1";
    const unsigned short port_num = 3333;

    try {
        SyncTCPClient client(raw_ip_address, port_num);

        // 동기 연결
        client.connect();

        std::cout << "서버에 요청을 보냅니다..." << std::endl;

        std::string response = client.emulateLongComputationOp(10);

        std::cout << "응답을 받았습니다: " << response << std::endl;

        // 연결을 종료하고 리소스를 해제합니다.
        client.close();
    }
    catch (system::system_error& e) {
        std::cout << "오류 발생! 오류 코드 = " << e.code()
            << ". 메시지: " << e.what();

        return e.code().value();
    }

    return 0;
}
