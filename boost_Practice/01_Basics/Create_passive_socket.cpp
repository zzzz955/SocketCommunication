#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. io_service 클래스 인스턴스 생성
	asio::io_service ios;

	// Step 2. TCP, IPv4 프로토콜 객체 생성
	asio::ip::tcp protocol = asio::ip::tcp::v4();

	// Step 3. 수동 소켓 인스턴스화.
	asio::ip::tcp::acceptor acceptor(ios);

	// 오류 정보 처리용
	boost::system::error_code ec;

	// Step 4. 수동 소켓을 연다.
	acceptor.open(protocol, ec);

	if (ec.value() != 0) {
		// 오류 발생 시
		std::cout
			<< "Failed to open the acceptor socket!"
			<< "Error code = "
			<< ec.value() << ". Message: " << ec.message();
		return ec.value();
	}

	return 0;
}
