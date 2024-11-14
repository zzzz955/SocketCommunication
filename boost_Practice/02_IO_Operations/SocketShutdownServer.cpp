#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

void processRequest(asio::ip::tcp::socket& sock) {
    // Ȯ�� ������ ���� ��� (��û �޽��� ũ�� ����)
    asio::streambuf request_buf;

    system::error_code ec;

    // ��û �ޱ�
    asio::read(sock, request_buf, ec);

    if (ec != asio::error::eof)
        // ���� ó��
        throw system::system_error(ec);

    // ��û �Ϸ�, ���� �غ�
    const char response_buf[] = { 0x48, 0x69, 0x21 };

    // ���� ����
    asio::write(sock, asio::buffer(response_buf));

    // ���� ���� (���� �Ϸ�)
    sock.shutdown(asio::socket_base::shutdown_send);
}

int main()
{
    unsigned short port_num = 3333;

    try {
        // ��������Ʈ ���� (���� �ּҿ� ��Ʈ)
        asio::ip::tcp::endpoint ep(asio::ip::address_v4::any(), port_num);

        // io_context ����
        asio::io_context ios;

        // acceptor ����
        asio::ip::tcp::acceptor acceptor(ios, ep);

        // ���� ����
        asio::ip::tcp::socket sock(ios);

        // Ŭ���̾�Ʈ ���� ����
        acceptor.accept(sock);

        // ��û ó��
        processRequest(sock);
    }
    catch (system::system_error& e) {
        // ���� ���
        std::cout << "���� �߻�! ���� �ڵ� = " << e.code()
            << ". �޽���: " << e.what() << std::endl;

        return e.code().value();
    }

    return 0;
}
