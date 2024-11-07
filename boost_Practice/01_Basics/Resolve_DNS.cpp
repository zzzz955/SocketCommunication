#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step 1. ������ ���� ���α׷��� DNs �̸��� ��Ʈ��ȣ�� ���ڿ��� ���� �ִٰ� ����
	std::string host = "samplehost.com";
	std::string port_num = "3333";

	// Step 2.
	asio::io_service ios;

	// Step 3. ���� ����
	asio::ip::tcp::resolver::query resolver_query(host,
		port_num, asio::ip::tcp::resolver::query::numeric_service);

	// Step 5. �ؼ��� ����
	asio::ip::tcp::resolver resolver(ios);

	// ���� ���� ��
	boost::system::error_code ec;

	// Step 6.
	asio::ip::tcp::resolver::iterator it =
		resolver.resolve(resolver_query, ec);

	if (ec != 0) {
		// DNS �̸��� �ؼ����� ���� ���
		std::cout << "Failed to resolve a DNS name. Error code = "
			<< ec.value() << ". Message = " << ec.message();

		return ec.value();
	}

	asio::ip::tcp::resolver::iterator it_end;

	for (; it != it_end; ++it) {
		// �������� �����ϱ�
		asio::ip::tcp::endpoint ep = it->endpoint();
	}

	return 0;
}