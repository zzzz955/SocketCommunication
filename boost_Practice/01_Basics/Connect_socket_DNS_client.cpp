#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// Step1. �����ϰ��� �ϴ� ���� ���α׷��� DNS �̸��� ��Ʈ ��ȣ �ʱ�ȭ
	std::string host = "samplehost.book";
	std::string port_num = "3333";

	// io_service �ν��Ͻ� ����
	asio::io_service ios;

	// resolver�� ������ ���� �ۼ�
	asio::ip::tcp::resolver::query resolver_query(host, port_num,
		asio::ip::tcp::resolver::query::numeric_service);

	// resolver ����
	asio::ip::tcp::resolver resolver(ios);

	try {
		// Step 2. DNS �̸� �ؼ�
		asio::ip::tcp::resolver::iterator it =
			resolver.resolve(resolver_query);

		// Step 3. ���� ����
		asio::ip::tcp::socket sock(ios);

		// Step 4. ���������� ����� �� ���� �ݺ��ڸ� ��ȯ
		// � ep�͵� ������ �� ���ų� ���� �߻� �� ���� ó��
		asio::connect(sock, it);

		// catch���� ������� �ʾҴٸ� �������� ���� ��� ����
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}

