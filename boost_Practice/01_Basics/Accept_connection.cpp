#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	// ��⿭ ť�� �ִ� ũ�� ����
	const int BACKLOG_SIZE = 30;

	// Step 1. ���� ���α׷��� ����� ��Ʈ ��ȣ
	unsigned short port_num = 3333;

	// Step 2. ������ �������� �����(any() ���)
	asio::ip::tcp::endpoint ep(asio::ip::address_v4::any(),
		port_num);

	// io_service �ν��Ͻ� ����
	asio::io_service ios;

	try {
		// Step 3. ������ ������ �ν��Ͻ�ȭ �ϰ� ����.
		asio::ip::tcp::acceptor acceptor(ios, ep.protocol());

		// Step 4. ������ ������ ep�� ���ε� �Ѵ�.
		acceptor.bind(ep);

		// Step 5. ������ ������ ������ ���·� �����.
		acceptor.listen(BACKLOG_SIZE);

		// Step 6. �ɵ� ������ �����Ѵ�.
		asio::ip::tcp::socket sock(ios);

		// Step 7. ���� ��û�� �����ϰ� Ŭ���̾�Ʈ�� �ɵ� ������ �����Ѵ�.
		acceptor.accept(sock);

		// �� �������� catch���� ������� �ʾҴٸ� Ŭ���̾�Ʈ�� ����� �غ� �Ϸ�Ǿ���.
	}
	catch (system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
			<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}


