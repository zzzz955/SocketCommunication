#include <boost/asio.hpp>
#include <iostream>
#include <thread>

using namespace boost;

int main()
{
    std::string raw_ip_address = "127.0.0.1";
    unsigned short port_num = 3333;

    try {
        asio::ip::tcp::endpoint ep(asio::ip::address::from_string(raw_ip_address), port_num);

        // �ֽ� Boost������ io_service ��� io_context ���
        asio::io_context ios;

        std::shared_ptr<asio::ip::tcp::socket> sock(
            new asio::ip::tcp::socket(ios, ep.protocol()));

        // �񵿱� ���� ��û
        sock->async_connect(ep,
            [sock](const boost::system::error_code& ec) // �ݹ� �Լ��� ���ٷ� �־���
            {
                if (ec.value() != 0) {
                    if (ec == asio::error::operation_aborted) {
                        std::cout << "Operation cancelled!" << std::endl;
                    }
                    else {
                        std::cout << "Error occured! "
                            << "Error code = " << ec.value()
                            << ". Message: " << ec.message() << std::endl;
                    }
                    return;
                }
                // ������ �Ϸ�Ǿ��� ��, ������ ����Ͽ� ���� ���ø����̼ǰ� ����� �� ����
            });

        // �񵿱� �۾��� �Ϸ�Ǹ� �ݹ��� ������ worker_thread ����
        std::thread worker_thread([&ios]() {
            try {
                ios.run();  // �񵿱� �۾� ó��
            }
            catch (system::system_error& e) {
                std::cout << "Error occured! "
                    << "Error code = " << e.code()
                    << ". Message: " << e.what() << std::endl;
            }
            });

        // ���Ƿ� ������ �ֱ� ���� 2�� ���
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // �񵿱� �۾� ���
        sock->cancel();

        // worker_thread�� ����� ������ ���
        worker_thread.join();
    }
    catch (system::system_error& e) {
        std::cout << "Error occured! Error code = " << e.code()
            << ". Message: " << e.what() << std::endl;

        return e.code().value();
    }

    return 0;
}
