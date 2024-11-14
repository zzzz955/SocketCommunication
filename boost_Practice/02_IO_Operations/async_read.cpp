#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

// �� ����Ʈ���� �о����� üũ�ϴ� ������ ������ �� �ִ�.
struct Session {
	std::shared_ptr<asio::ip::tcp::socket> sock;
	std::unique_ptr<char[]> buf;
	unsigned int buf_size;
};

// �񵿱� �б� ������ ��������� ������ �ʿ䰡 ��������.
void callback(const boost::system::error_code& ec,
	std::size_t bytes_transferred,
	std::shared_ptr<Session> s)
{
	if (ec.value() != 0) {
		std::cout << "Error occured! Error code = "
			<< ec.value()
			<< ". Message: " << ec.message();

		return;
	}
}

void readFromSocket(std::shared_ptr<asio::ip::tcp::socket> sock) {
	std::shared_ptr<Session> s(new Session);

	const unsigned int MESSAGE_SIZE = 7;

	// Step 1. ���� �Ҵ�
	s->buf.reset(new char[MESSAGE_SIZE]);

	s->sock = sock;
	s->buf_size = MESSAGE_SIZE;

	// Step 2. �񵿱� �б� ���� ����
	asio::async_read(*s->sock,
		asio::buffer(s->buf.get(), s->buf_size),
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

		std::shared_ptr<asio::ip::tcp::socket> sock(
			new asio::ip::tcp::socket(ios, ep.protocol()));

		sock->connect(ep);

		readFromSocket(sock);

		ios.run();
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}
