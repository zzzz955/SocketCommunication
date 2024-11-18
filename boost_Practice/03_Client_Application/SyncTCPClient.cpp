#include <boost/asio.hpp>
#include <iostream>

using namespace boost;

// ���� TCP Ŭ���̾�Ʈ Ŭ����
class SyncTCPClient {
public:
    SyncTCPClient(const std::string& raw_ip_address,
        unsigned short port_num) :
        m_ep(asio::ip::make_address(raw_ip_address), port_num), // �ֽ� ���������� from_string ��� make_address ���
        m_sock(m_ios) {

        m_sock.open(m_ep.protocol());
    }

    // ������ �����ϴ� �Լ�
    void connect() {
        m_sock.connect(m_ep);
    }

    // ������ �����ϴ� �Լ�
    void close() {
        m_sock.shutdown(asio::ip::tcp::socket::shutdown_both);
        m_sock.close();
    }

    // ������ ���� �ɸ��� �۾��� ��û�ϰ� ������ �޴� �Լ�
    std::string emulateLongComputationOp(unsigned int duration_sec) {
        std::string request = "EMULATE_LONG_COMP_OP " + std::to_string(duration_sec) + "\n";
        sendRequest(request);
        return receiveResponse();
    }

private:
    // ������ ��û�� ������ �Լ�
    void sendRequest(const std::string& request) {
        asio::write(m_sock, asio::buffer(request));
    }

    // ������ ������ �޴� �Լ�
    std::string receiveResponse() {
        asio::streambuf buf;
        asio::read_until(m_sock, buf, '\n');

        std::istream input(&buf);
        std::string response;
        std::getline(input, response);

        return response;
    }

private:
    asio::io_context m_ios; // io_service ��� io_context ���

    asio::ip::tcp::endpoint m_ep;
    asio::ip::tcp::socket m_sock;
};

int main() {
    const std::string raw_ip_address = "127.0.0.1";
    const unsigned short port_num = 3333;

    try {
        SyncTCPClient client(raw_ip_address, port_num);

        // ���� ����
        client.connect();

        std::cout << "������ ��û�� �����ϴ�..." << std::endl;

        std::string response = client.emulateLongComputationOp(10);

        std::cout << "������ �޾ҽ��ϴ�: " << response << std::endl;

        // ������ �����ϰ� ���ҽ��� �����մϴ�.
        client.close();
    }
    catch (system::system_error& e) {
        std::cout << "���� �߻�! ���� �ڵ� = " << e.code()
            << ". �޽���: " << e.what();

        return e.code().value();
    }

    return 0;
}
