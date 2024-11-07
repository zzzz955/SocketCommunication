#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step1. ���� IP �ּҿ� ��Ʈ ��ȣ�� �̹� �˰��ִٰ� ����
	std::string raw_ip_address = "127.0.0.1";
	unsigned short port_num = 12345;

	// ���� ���� �����
	boost::system::error_code ec;

	// Step2. IP �������� ������ ���� ���� �ּ� ���� ���
	asio::ip::address ip_address =
		asio::ip::address::from_string(raw_ip_address, ec);

	if (ec.value() != 0) {
		// IP �ּҰ� ��ȿ���� ���� ���
		std::cout
			<< "Failed to parse the IP address. Error code = "
			<< ec.value() << ". Message: " << ec.message();
		return ec.value();
	}

	// Step 3.
	asio::ip::tcp::endpoint ep(ip_address, port_num);

	return 0;
}