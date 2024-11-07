#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. 서버 프로그램이 사용할 프로토콜 포트 번호를 알고 있다고 가정
	unsigned short port_num = 3333;

	// Step 2. 호스트에서 쓸 수 있는 모든 IP 주소를 나타내는 클래스 객체 생성
	asio::ip::address ip_address = asio::ip::address_v4::any();

	// Step 3.
	asio::ip::tcp::endpoint ep(ip_address, port_num);

	return 0;
}