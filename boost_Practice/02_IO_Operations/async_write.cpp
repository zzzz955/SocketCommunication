#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

// �� ����Ʈ���� ����� üũ�� ������ �ʿ䰡 ����.
struct Session {
    std::shared_ptr<asio::ip::tcp::socket> sock;
    std::string buf;
};

// �ݹ��� �����ϴ�.
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

    // ���� �۾��� �Ϸᰡ �Ǿ��ٸ� ���
    std::cout << "Successfully written " << bytes_transferred << " bytes to the socket.\n";
}

void writeToSocket(std::shared_ptr<asio::ip::tcp::socket> sock) {

    std::shared_ptr<Session> s(new Session);

    s->buf = std::string("Hello");
    s->sock = sock;

    // Step 5. �񵿱� ���� �۾��� �����Ѵ�.
    asio::async_write(
        *s->sock,  // ������ ���ڷ� �־��־�� �Ѵ�.
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

        // Step 6. �񵿱� ó�� �Ϸ�� ��
        ios.run();
    }
    catch (system::system_error& e) {
        std::cout << "Error occurred! Error code = " << e.code()
            << ". Message: " << e.what();
        return e.code().value();
    }

    return 0;
}
