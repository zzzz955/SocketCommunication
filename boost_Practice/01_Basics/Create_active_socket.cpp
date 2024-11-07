#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. io_service Ŭ������ �ν��Ͻ� ����
	asio::io_service ios;

	// Step 2. tcp, IPv4 �������� ��ü ����
	asio::ip::tcp protocol = asio::ip::tcp::v4();

	// Step 3. �ɵ� TCP ���� �ν��Ͻ�ȭ
	asio::ip::tcp::socket sock(ios);

	// ���� ���� ó����
	boost::system::error_code ec;

	// Step 4. ������ ����.
	sock.open(protocol, ec);

	if (ec.value() != 0) {
		// ���� �߻� ��
		std::cout
			<< "Failed to open the socket! Error code = "
			<< ec.value() << ". Message: " << ec.message();
		return ec.value();
	}

	return 0;
}