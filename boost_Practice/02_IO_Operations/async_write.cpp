#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

// 몇 바이트까지 썼는지 체크를 진행할 필요가 없다.
struct Session {
    std::shared_ptr<asio::ip::tcp::socket> sock;
    std::string buf;
};

// 콜백은 동일하다.
void callback(const boost::system::error_code& ec,
    std::size_t bytes_transferred,
    std::shared_ptr<Session> s)
{
    if (ec.value() != 0) {
        std::cout << "Error occurred! Error code = "
            << ec.value()
            << ". Message: " << ec.message();
        return;
    }

    // 쓰기 작업이 완료가 되었다면 출력
    std::cout << "Successfully written " << bytes_transferred << " bytes to the socket.\n";
}

void writeToSocket(std::shared_ptr<asio::ip::tcp::socket> sock) {

    std::shared_ptr<Session> s(new Session);

    s->buf = std::string("Hello");
    s->sock = sock;

    // Step 5. 비동기 쓰기 작업을 시작한다.
    asio::async_write(
        *s->sock,  // 소켓을 인자로 넣어주어야 한다.
        asio::buffer(s->buf),
        std::bind(callback,
            std::placeholders::_1,
            std::placeholders::_2,
            s));
}

int main()
{
    std::string raw_ip_address = "127.0.0.1";
    unsigned short port_num = 3333;

    try {
        asio::ip::tcp::endpoint ep(asio::ip::address::from_string(raw_ip_address), port_num);
        asio::io_service ios;

        std::shared_ptr<asio::ip::tcp::socket> sock(
            new asio::ip::tcp::socket(ios, ep.protocol()));

        sock->connect(ep);

        writeToSocket(sock);

        // Step 6. 비동기 처리 완료시 런
        ios.run();
    }
    catch (system::system_error& e) {
        std::cout << "Error occurred! Error code = " << e.code()
            << ". Message: " << e.what();
        return e.code().value();
    }

    return 0;
}
