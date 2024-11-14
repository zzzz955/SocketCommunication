#include <boost/asio.hpp>
#include <iostream>
#include <memory> // std::unique_ptr<>�� ����ϱ� ���� �߰�

using namespace boost;

int main()
{
	// 20����Ʈ ���� ���� �����͸� �޴´ٰ� ����
	const size_t BUF_SIZE_BYTES = 20;

	// Step 1. ���۸� �Ҵ��Ѵ�.
	std::unique_ptr<char[]> buf(new char[BUF_SIZE_BYTES]);

	// Step 2. MutableBuffer ������ ������ �����ϴ� ���� �������� ��ȯ�Ѵ�.
	asio::mutable_buffers_1 input_buf =
		asio::buffer(static_cast<void*>(buf.get()),
			BUF_SIZE_BYTES);

	// Step 3. ���� input_buf�� �Է� ���꿡 ����� �� �ִ�.

	return 0;
}