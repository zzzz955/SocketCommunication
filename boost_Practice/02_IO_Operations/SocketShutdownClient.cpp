#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

void communicate(asio::ip::tcp::socket& sock) {
    // ��û ������ �غ�
    const char request_buf[] = { 0x48, 0x65, 0x0, 0x6c, 0x6c, 0x6f };

    // ��û ����
    asio::write(sock, asio::buffer(request_buf));

    // ���� ���� (���� �Ϸ�)
    sock.shutdown(asio::socket_base::shutdown_send);

    // ���� ����
    asio::streambuf response_buf;

    system::error_code ec;
    // ���� �б�
    asio::read(sock, response_buf, ec);

    if (ec == asio::error::eof) {
        // ���� �Ϸ�
    }
    else {
        // ���� ó��
        throw system::system_error(ec);
    }
}

int main()
{
    std::string raw_ip_address = "127.0.0.1";
    unsigned short port_num = 3333;

    try {
        // ��������Ʈ ����
        asio::ip::tcp::endpoint ep(asio::ip::address::from_string(raw_ip_address), port_num);

        // io_context ����
        asio::io_context ios;

        // ���� ����
        asio::ip::tcp::socket sock(ios, ep.protocol());

        // ���� ����
        sock.connect(ep);

        // ���
        communicate(sock);
    }
    catch (system::system_error& e) {
        // ���� ���
        std::cout << "���� �߻�! ���� �ڵ� = " << e.code()
            << ". �޽���: " << e.what() << std::endl;

        return e.code().value();
    }

    return 0;
}
