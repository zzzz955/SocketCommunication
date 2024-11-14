#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

// 소켓 객체에 대한 포인터와 버퍼, 바이트 수를 저장하는 카운터 변수를 구조체로 정의
struct Session {
	std::shared_ptr<asio::ip::tcp::socket> sock;
	std::string buf;
	std::size_t total_bytes_written = 0;
};

// 비동기 쓰기 연산의 콜백함수
// 소켓에서 모든 데이터를 썼는지 확인하고, 필요하다면 다음 비동기 쓰기 연산 수행
void callback(const boost::system::error_code& ec,
	std::size_t bytes_transferred,
	std::shared_ptr<Session> s)
{
	if (ec.value() != 0) {
		std::cout << "Error occured! Error code = "
			<< ec.value() << ". Message: " << ec.message();
		return;
	}
	// 아직 소켓에 버퍼의 모든 데이터가 써진 상태가 아니라면?
	// 마지막으로 쓰기 연산이 완료된 위치에서 비동기 쓰기 연산을 다시 시작한다.
	s->total_bytes_written += bytes_transferred;

	if (s->total_bytes_written < s->buf.size()) {
		s->sock->async_write_some(
			asio::buffer(s->buf.c_str() + s->total_bytes_written,
				s->buf.size() - s->total_bytes_written),
			std::bind(callback,
				std::placeholders::_1,
				std::placeholders::_2,
				s));
	}
}

void writeToSocket(std::shared_ptr<asio::ip::tcp::socket> sock) {

	std::shared_ptr<Session> s(new Session);

	// Step 4. Session의 데이터를 초기화 한다.(버퍼, 소켓, 입력한 바이트 수)
	s->buf = std::string("Hello");
	s->sock = sock;

	// Step 5. 비동기 쓰기 연산을 시작한다.
	s->sock->async_write_some(
		asio::buffer(s->buf),
		std::bind(callback,
			std::placeholders::_1,
			std::placeholders::_2,
			s));
}

int main()
{
	std::string raw_ip_address = "127.0.0.1";
	unsigned short port_num = 3333;

	try {
		asio::ip::tcp::endpoint
			ep(asio::ip::address::from_string(raw_ip_address),
				port_num);

		asio::io_service ios;

		// Step 3. 소켓을 초기화 하고 소켓의 포인터를 초기화 한다.
		std::shared_ptr<asio::ip::tcp::socket> sock(
			new asio::ip::tcp::socket(ios, ep.protocol()));

		sock->connect(ep);

		writeToSocket(sock);

		// Step 6. 런!
		ios.run();
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}
