#define BOOST_DISABLE_CURRENT_LOCATION
#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. ���� ���α׷��� ����� �������� ��Ʈ ��ȣ�� �˰� �ִٰ� ����
	unsigned short port_num = 3333;

	// Step 2. ��� IP�� ���� �������� �����.
	asio::ip::tcp::endpoint ep(asio::ip::address_v4::any(), port_num);

	// io_service �ν��Ͻ� ����
	asio::io_service ios;

	// Step 3. ���� ������ �����.
	asio::ip::tcp::acceptor acceptor(ios, ep.protocol());

	boost::system::error_code ec;

	// Step 4. ���� ������ ���ε��Ѵ�.
	acceptor.bind(ep, ec);

	// ���� �ڵ鸵
	if (ec.value() != 0) {
		// ���� ���� ���ε� ���� ���ߴٸ�
		std::cout << "Failed to bind the acceptor socket."
			<< "Error code = " << ec.value() << ". Message: "
			<< ec.message();

		return ec.value();
	}

	return 0;
}
