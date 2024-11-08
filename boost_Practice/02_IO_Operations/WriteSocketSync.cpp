#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

void writeToSocketEnhanced(asio::ip::tcp::socket& sock) {
	// ���ۿ� ä�� ���� �ʱ�ȭ �Ѵ�.
	std::string buf = "Hello";

	// ���� ��ü�� ���Ͽ� ����.
	asio::write(sock, asio::buffer(buf));
}

void writeToSocket(asio::ip::tcp::socket& sock) {
	// ���ۿ� ä�� ���� �ʱ�ȭ �Ѵ�.
	std::string buf = "Hello";

	std::size_t total_bytes_written = 0;

	// ��� �����͸� ���Ͽ� �� ������ ������ ����.
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
