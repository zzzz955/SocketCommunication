#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. 서버의 IP 및 포트 번호를 안다고 가정
	std::string raw_ip_address = "127.0.0.1";
	unsigned short port_num = 3333;

	try {
		// Step 2. IP와 포트 번호를 통해 종료점 생성
		asio::ip::tcp::endpoint
			ep(asio::ip::address::from_string(raw_ip_address),
				port_num);

		asio::io_service ios;

		// Step 3. io_service 인스턴스와 종료점을 통해 소켓 생성
		asio::ip::tcp::socket sock(ios, ep.protocol());

		// Step 4. 서버에 연결 요청
		sock.connect(ep);
	}
	// 예외가 발생하지 않았다면 이제 서버와 통신 가능
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}
