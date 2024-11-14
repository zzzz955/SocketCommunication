#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	asio::streambuf buf;

	std::ostream output(&buf);

	// ��Ʈ�� ��� ���ۿ� �޽��� �Ҵ�
	output << "Message1\nMessage2";

	// �� �ٲ��� ���� �� ���� streambuf���� �����͸� �д´�.
	std::istream input(&buf);

	// ���ڿ��� �Ҵ��� ���� �ʱ�ȭ
	std::string message1;

	std::getline(input, message1);

	// message1 ������ Message1 ���ڿ��� �Ҵ�Ǿ���.
	std::cout << message1;

	return 0;
}
