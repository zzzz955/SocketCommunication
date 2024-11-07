#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. 연결한 서버 프로그램의 DNs 이름과 포트번호를 문자열로 갖고 있다고 가정
	std::string host = "samplehost.com";
	std::string port_num = "3333";

	// Step 2.
	asio::io_service ios;

	// Step 3. 질의 생성
	asio::ip::tcp::resolver::query resolver_query(host,
		port_num, asio::ip::tcp::resolver::query::numeric_service);

	// Step 5. 해석기 생성
	asio::ip::tcp::resolver resolver(ios);

	// 오류 정보 용
	boost::system::error_code ec;

	// Step 6.
	asio::ip::tcp::resolver::iterator it =
		resolver.resolve(resolver_query, ec);

	if (ec != 0) {
		// DNS 이름을 해석하지 못한 경우
		std::cout << "Failed to resolve a DNS name. Error code = "
			<< ec.value() << ". Message = " << ec.message();

		return ec.value();
	}

	asio::ip::tcp::resolver::iterator it_end;

	for (; it != it_end; ++it) {
		// 종료점에 접근하기
		asio::ip::tcp::endpoint ep = it->endpoint();
	}

	return 0;
}