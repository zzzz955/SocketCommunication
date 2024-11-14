#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	std::string buf; // 버퍼 초기화
	buf = "Hello";   // 버퍼에 데이터 추가

	// Step 3. 버퍼를 const_buffer 시퀀스로 변환
	// 과거엔 buffer메서드에 buf만 전달해 주면 됐다.
	// 현재는 data(), size() 메서드를 통해 명시해 주어야 한다.
	asio::const_buffers_1 output_buf = asio::buffer(buf.data(), buf.size());

	// Step 4. 이제 output_buf를 출력 연산에 사용할 수 있다.

	return 0;
}
