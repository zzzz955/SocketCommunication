#include <boost/asio.hpp>
#include <iostream>
#include <memory> // std::unique_ptr<>를 사용하기 위해 추가

using namespace boost;

int main()
{
	// 20바이트 보다 작은 데이터를 받는다고 가정
	const size_t BUF_SIZE_BYTES = 20;

	// Step 1. 버퍼를 할당한다.
	std::unique_ptr<char[]> buf(new char[BUF_SIZE_BYTES]);

	// Step 2. MutableBuffer 시퀀스 개념을 만족하는 버퍼 형식으로 변환한다.
	asio::mutable_buffers_1 input_buf =
		asio::buffer(static_cast<void*>(buf.get()),
			BUF_SIZE_BYTES);

	// Step 3. 이제 input_buf를 입력 연산에 사용할 수 있다.

	return 0;
}