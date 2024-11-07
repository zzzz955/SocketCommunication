#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. ������ IP �� ��Ʈ ��ȣ�� �ȴٰ� ����
	std::string raw_ip_address = "127.0.0.1";
	unsigned short port_num = 3333;

	try {
		// Step 2. IP�� ��Ʈ ��ȣ�� ���� ������ ����
		asio::ip::tcp::endpoint
			ep(asio::ip::address::from_string(raw_ip_address),
				port_num);

		asio::io_service ios;

		// Step 3. io_service �ν��Ͻ��� �������� ���� ���� ����
		asio::ip::tcp::socket sock(ios, ep.protocol());

		// Step 4. ������ ���� ��û
		sock.connect(ep);
	}
	// ���ܰ� �߻����� �ʾҴٸ� ���� ������ ��� ����
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}
