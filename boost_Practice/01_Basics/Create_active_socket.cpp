#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. io_service 클래스의 인스턴스 생성
	asio::io_service ios;

	// Step 2. tcp, IPv4 프로토콜 객체 생성
	asio::ip::tcp protocol = asio::ip::tcp::v4();

	// Step 3. 능동 TCP 소켓 인스턴스화
	asio::ip::tcp::socket sock(ios);

	// 오류 정보 처리용
	boost::system::error_code ec;

	// Step 4. 소켓을 연다.
	sock.open(protocol, ec);

	if (ec.value() != 0) {
		// 오류 발생 시
		std::cout
			<< "Failed to open the socket! Error code = "
			<< ec.value() << ". Message: " << ec.message();
		return ec.value();
	}

	return 0;
}