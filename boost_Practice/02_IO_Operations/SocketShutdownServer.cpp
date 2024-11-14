#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

void processRequest(asio::ip::tcp::socket& sock) {
    // 확장 가능한 버퍼 사용 (요청 메시지 크기 미정)
    asio::streambuf request_buf;

    system::error_code ec;

    // 요청 받기
    asio::read(sock, request_buf, ec);

    if (ec != asio::error::eof)
        // 오류 처리
        throw system::system_error(ec);

    // 요청 완료, 응답 준비
    const char response_buf[] = { 0x48, 0x69, 0x21 };

    // 응답 전송
    asio::write(sock, asio::buffer(response_buf));

    // 소켓 종료 (전송 완료)
    sock.shutdown(asio::socket_base::shutdown_send);
}

int main()
{
    unsigned short port_num = 3333;

    try {
        // 엔드포인트 생성 (로컬 주소와 포트)
        asio::ip::tcp::endpoint ep(asio::ip::address_v4::any(), port_num);

        // io_context 생성
        asio::io_context ios;

        // acceptor 생성
        asio::ip::tcp::acceptor acceptor(ios, ep);

        // 소켓 생성
        asio::ip::tcp::socket sock(ios);

        // 클라이언트 연결 수락
        acceptor.accept(sock);

        // 요청 처리
        processRequest(sock);
    }
    catch (system::system_error& e) {
        // 오류 출력
        std::cout << "오류 발생! 오류 코드 = " << e.code()
            << ". 메시지: " << e.what() << std::endl;

        return e.code().value();
    }

    return 0;
}
