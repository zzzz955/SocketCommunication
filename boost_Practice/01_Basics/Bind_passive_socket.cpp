#define BOOST_DISABLE_CURRENT_LOCATION
#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. 서버 프로그램이 사용할 프로토콜 포트 번호를 알고 있다고 가정
	unsigned short port_num = 3333;

	// Step 2. 모든 IP에 대한 종료점을 만든다.
	asio::ip::tcp::endpoint ep(asio::ip::address_v4::any(), port_num);

	// io_service 인스턴스 생성
	asio::io_service ios;

	// Step 3. 수동 소켓을 만든다.
	asio::ip::tcp::acceptor acceptor(ios, ep.protocol());

	boost::system::error_code ec;

	// Step 4. 수동 소켓을 바인딩한다.
	acceptor.bind(ep, ec);

	// 에러 핸들링
	if (ec.value() != 0) {
		// 수동 소켓 바인드 하지 못했다면
		std::cout << "Failed to bind the acceptor socket."
			<< "Error code = " << ec.value() << ". Message: "
			<< ec.message();

		return ec.value();
	}

	return 0;
}
