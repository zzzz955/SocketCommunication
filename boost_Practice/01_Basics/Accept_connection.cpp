#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// 대기열 큐의 최대 크기 정의
	const int BACKLOG_SIZE = 30;

	// Step 1. 서버 프로그램이 사용할 포트 번호
	unsigned short port_num = 3333;

	// Step 2. 서버의 종료점을 만든다(any() 사용)
	asio::ip::tcp::endpoint ep(asio::ip::address_v4::any(),
		port_num);

	// io_service 인스턴스 생성
	asio::io_service ios;

	try {
		// Step 3. 수용자 소켓을 인스턴스화 하고 연다.
		asio::ip::tcp::acceptor acceptor(ios, ep.protocol());

		// Step 4. 수용자 소켓을 ep에 바인드 한다.
		acceptor.bind(ep);

		// Step 5. 수용자 소켓을 리스닝 상태로 만든다.
		acceptor.listen(BACKLOG_SIZE);

		// Step 6. 능동 소켓을 생성한다.
		asio::ip::tcp::socket sock(ios);

		// Step 7. 연결 요청을 수락하고 클라이언트에 능동 소켓을 연결한다.
		acceptor.accept(sock);

		// 이 시점까지 catch문이 실행되지 않았다면 클라이언트와 통신할 준비가 완료되었다.
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}


