#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step1. 서버 IP 주소와 포트 번호를 이미 알고있다고 가정
	std::string raw_ip_address = "127.0.0.1";
	unsigned short port_num = 12345;

	// 오류 정보 저장용
	boost::system::error_code ec;

	// Step2. IP 프로토콜 버전과 관계 없는 주소 형식 사용
	asio::ip::address ip_address =
		asio::ip::address::from_string(raw_ip_address, ec);

	if (ec.value() != 0) {
		// IP 주소가 유효하지 않을 경우
		std::cout
			<< "Failed to parse the IP address. Error code = "
			<< ec.value() << ". Message: " << ec.message();
		return ec.value();
	}

	// Step 3.
	asio::ip::tcp::endpoint ep(ip_address, port_num);

	return 0;
}