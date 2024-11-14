#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

void communicate(asio::ip::tcp::socket& sock) {
    // 요청 데이터 준비
    const char request_buf[] = { 0x48, 0x65, 0x0, 0x6c, 0x6c, 0x6f };

    // 요청 전송
    asio::write(sock, asio::buffer(request_buf));

    // 소켓 종료 (전송 완료)
    sock.shutdown(asio::socket_base::shutdown_send);

    // 응답 버퍼
    asio::streambuf response_buf;

    system::error_code ec;
    // 응답 읽기
    asio::read(sock, response_buf, ec);

    if (ec == asio::error::eof) {
        // 응답 완료
    }
    else {
        // 오류 처리
        throw system::system_error(ec);
    }
}

int main()
{
    std::string raw_ip_address = "127.0.0.1";
    unsigned short port_num = 3333;

    try {
        // 엔드포인트 생성
        asio::ip::tcp::endpoint ep(asio::ip::address::from_string(raw_ip_address), port_num);

        // io_context 생성
        asio::io_context ios;

        // 소켓 생성
        asio::ip::tcp::socket sock(ios, ep.protocol());

        // 서버 연결
        sock.connect(ep);

        // 통신
        communicate(sock);
    }
    catch (system::system_error& e) {
        // 오류 출력
        std::cout << "오류 발생! 오류 코드 = " << e.code()
            << ". 메시지: " << e.what() << std::endl;

        return e.code().value();
    }

    return 0;
}
