#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

// ���� ��ü�� ���� �����Ϳ� ����, ����Ʈ ���� �����ϴ� ī���� ������ ����ü�� ����
struct Session {
	std::shared_ptr<asio::ip::tcp::socket> sock;
	std::string buf;
	std::size_t total_bytes_written = 0;
};

// �񵿱� ���� ������ �ݹ��Լ�
// ���Ͽ��� ��� �����͸� ����� Ȯ���ϰ�, �ʿ��ϴٸ� ���� �񵿱� ���� ���� ����
void callback(const boost::system::error_code& ec,
	std::size_t bytes_transferred,
	std::shared_ptr<Session> s)
{
	if (ec.value() != 0) {
		std::cout << "Error occured! Error code = "
			<< ec.value() << ". Message: " << ec.message();
		return;
	}
	// ���� ���Ͽ� ������ ��� �����Ͱ� ���� ���°� �ƴ϶��?
	// ���������� ���� ������ �Ϸ�� ��ġ���� �񵿱� ���� ������ �ٽ� �����Ѵ�.
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

	// Step 4. Session�� �����͸� �ʱ�ȭ �Ѵ�.(����, ����, �Է��� ����Ʈ ��)
	s->buf = std::string("Hello");
	s->sock = sock;

	// Step 5. �񵿱� ���� ������ �����Ѵ�.
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

		// Step 3. ������ �ʱ�ȭ �ϰ� ������ �����͸� �ʱ�ȭ �Ѵ�.
		std::shared_ptr<asio::ip::tcp::socket> sock(
			new asio::ip::tcp::socket(ios, ep.protocol()));

		sock->connect(ep);

		writeToSocket(sock);

		// Step 6. ��!
		ios.run();
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}
