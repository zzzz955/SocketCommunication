#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	std::string buf; // ���� �ʱ�ȭ
	buf = "Hello";   // ���ۿ� ������ �߰�

	// Step 3. ���۸� const_buffer �������� ��ȯ
	// ���ſ� buffer�޼��忡 buf�� ������ �ָ� �ƴ�.
	// ����� data(), size() �޼��带 ���� ����� �־�� �Ѵ�.
	asio::const_buffers_1 output_buf = asio::buffer(buf.data(), buf.size());

	// Step 4. ���� output_buf�� ��� ���꿡 ����� �� �ִ�.

	return 0;
}
