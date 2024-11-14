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

        // 최신 Boost에서는 io_service 대신 io_context 사용
        asio::io_context ios;

        std::shared_ptr<asio::ip::tcp::socket> sock(
            new asio::ip::tcp::socket(ios, ep.protocol()));

        // 비동기 연결 요청
        sock->async_connect(ep,
            [sock](const boost::system::error_code& ec) // 콜백 함수를 람다로 넣었음
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
                // 연결이 완료되었을 때, 소켓을 사용하여 원격 애플리케이션과 통신할 수 있음
            });

        // 비동기 작업이 완료되면 콜백을 실행할 worker_thread 생성
        std::thread worker_thread([&ios]() {
            try {
                ios.run();  // 비동기 작업 처리
            }
            catch (system::system_error& e) {
                std::cout << "Error occured! "
                    << "Error code = " << e.code()
                    << ". Message: " << e.what() << std::endl;
            }
            });

        // 임의로 지연을 주기 위해 2초 대기
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // 비동기 작업 취소
        sock->cancel();

        // worker_thread가 종료될 때까지 대기
        worker_thread.join();
    }
    catch (system::system_error& e) {
        std::cout << "Error occured! Error code = " << e.code()
            << ". Message: " << e.what() << std::endl;

        return e.code().value();
    }

    return 0;
}
