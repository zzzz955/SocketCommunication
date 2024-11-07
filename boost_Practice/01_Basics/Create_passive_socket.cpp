#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. io_service Ŭ���� �ν��Ͻ� ����
	asio::io_service ios;

	// Step 2. TCP, IPv4 �������� ��ü ����
	asio::ip::tcp protocol = asio::ip::tcp::v4();

	// Step 3. ���� ���� �ν��Ͻ�ȭ.
	asio::ip::tcp::acceptor acceptor(ios);

	// ���� ���� ó����
	boost::system::error_code ec;

	// Step 4. ���� ������ ����.
	acceptor.open(protocol, ec);

	if (ec.value() != 0) {
		// ���� �߻� ��
		std::cout
			<< "Failed to open the acceptor socket!"
			<< "Error code = "
			<< ec.value() << ". Message: " << ec.message();
		return ec.value();
	}

	return 0;
}
