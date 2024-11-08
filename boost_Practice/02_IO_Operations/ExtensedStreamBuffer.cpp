#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

int main()
{
	asio::streambuf buf;

	std::ostream output(&buf);

	// 스트림 기반 버퍼에 메시지 할당
	output << "Message1\nMessage2";

	// 줄 바꿈을 만날 때 까지 streambuf에서 데이터를 읽는다.
	std::istream input(&buf);

	// 문자열을 할당할 변수 초기화
	std::string message1;

	std::getline(input, message1);

	// message1 변수에 Message1 문자열이 할당되었다.
	std::cout << message1;

	return 0;
}
