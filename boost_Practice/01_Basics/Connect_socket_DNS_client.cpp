#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step1. 연결하고자 하는 서버 프로그램의 DNS 이름과 포트 번호 초기화
	std::string host = "samplehost.book";
	std::string port_num = "3333";

	// io_service 인스턴스 생성
	asio::io_service ios;

	// resolver에 전달할 질의 작성
	asio::ip::tcp::resolver::query resolver_query(host, port_num,
		asio::ip::tcp::resolver::query::numeric_service);

	// resolver 생성
	asio::ip::tcp::resolver resolver(ios);

	try {
		// Step 2. DNS 이름 해석
		asio::ip::tcp::resolver::iterator it =
			resolver.resolve(resolver_query);

		// Step 3. 소켓 생성
		asio::ip::tcp::socket sock(ios);

		// Step 4. 성공적으로 연결될 때 까지 반복자를 순환
		// 어떤 ep와도 연결할 수 없거나 오류 발생 시 예외 처리
		asio::connect(sock, it);

		// catch문이 실행되지 않았다면 이제부터 서버 통신 가능
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}

