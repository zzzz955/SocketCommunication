#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. ���� ���α׷��� ����� �������� ��Ʈ ��ȣ�� �˰� �ִٰ� ����
	unsigned short port_num = 3333;

	// Step 2. ȣ��Ʈ���� �� �� �ִ� ��� IP �ּҸ� ��Ÿ���� Ŭ���� ��ü ����
	asio::ip::address ip_address = asio::ip::address_v4::any();

	// Step 3.
	asio::ip::tcp::endpoint ep(ip_address, port_num);

	return 0;
}