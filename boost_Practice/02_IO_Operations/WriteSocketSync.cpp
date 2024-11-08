#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

void writeToSocketEnhanced(asio::ip::tcp::socket& sock) {
	// 버퍼에 채울 값을 초기화 한다.
	std::string buf = "Hello";

	// 버퍼 전체를 소켓에 쓴다.
	asio::write(sock, asio::buffer(buf));
}

void writeToSocket(asio::ip::tcp::socket& sock) {
	// 버퍼에 채울 값을 초기화 한다.
	std::string buf = "Hello";

	std::size_t total_bytes_written = 0;

	// 모든 데이터를 소켓에 쓸 때까지 루프를 돈다.
	while (total_bytes_written != buf.length()) {
		total_bytes_written += sock.write_some(
			asio::buffer(buf.c_str() +
				total_bytes_written,
				buf.length() - total_bytes_written));
	}
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

		asio::ip::tcp::socket sock(ios, ep.protocol());

		sock.connect(ep);

		writeToSocket(sock);
		//writeToSocketEnhanced(sock);
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}
