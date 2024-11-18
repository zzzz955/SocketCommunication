#include <boost/predef.h> // OS 식별 도구

// Windows XP, Windows Server 2003 이하에서 I/O 작업 취소 기능을 활성화하기 위해 필요
// 자세한 내용은 "http://www.boost.org/doc/libs/1_58_0/
// doc/html/boost_asio/reference/basic_stream_socket/"
// cancel/overload1.html" 참조
#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINNT 0x0501

#if _WIN32_WINNT <= 0x0502 // Windows Server 2003 이하
#define BOOST_ASIO_DISABLE_IOCP
#define BOOST_ASIO_ENABLE_CANCELIO    
#endif
#endif

#include <boost/asio.hpp>

#include <thread>
#include <mutex>
#include <memory>
#include <iostream>
#include <map>

using namespace boost;

// 요청이 완료되었을 때 호출되는 콜백 함수 포인터 타입
typedef void(*Callback) (unsigned int request_id,
    const std::string& response,
    const system::error_code& ec);

// 하나의 요청에 대한 컨텍스트를 나타내는 구조체
struct Session {
    Session(asio::io_context& ios,
        const std::string& raw_ip_address,
        unsigned short port_num,
        const std::string& request,
        unsigned int id,
        Callback callback) :
        m_sock(ios),
        m_ep(asio::ip::address::from_string(raw_ip_address),
            port_num),
        m_request(request),
        m_id(id),
        m_callback(callback),
        m_was_cancelled(false) {}

    asio::ip::tcp::socket m_sock; // 통신에 사용되는 소켓
    asio::ip::tcp::endpoint m_ep; // 원격 엔드포인트
    std::string m_request;        // 요청 문자열

    // 응답을 저장할 streambuf
    asio::streambuf m_response_buf;
    std::string m_response; // 응답 문자열

    // 요청 생애주기 중 오류가 발생하면 그 설명이 저장됨
    system::error_code m_ec;

    unsigned int m_id; // 요청에 할당된 고유 ID

    // 요청이 완료되었을 때 호출되는 콜백 함수 포인터
    Callback m_callback;

    bool m_was_cancelled; // 요청 취소 여부
    std::mutex m_cancel_guard; // 취소 시 동기화를 위한 mutex
};

class AsyncTCPClient {
public:
    AsyncTCPClient() {
        m_work.reset(new boost::asio::io_context::work(m_ios));

        m_thread.reset(new std::thread([this]() {
            m_ios.run();
            }));
    }

    void emulateLongComputationOp(
        unsigned int duration_sec,
        const std::string& raw_ip_address,
        unsigned short port_num,
        Callback callback,
        unsigned int request_id) {

        // 요청 문자열 준비
        std::string request = "EMULATE_LONG_CALC_OP "
            + std::to_string(duration_sec)
            + "\n";

        std::shared_ptr<Session> session =
            std::shared_ptr<Session>(new Session(m_ios,
                raw_ip_address,
                port_num,
                request,
                request_id,
                callback));

        session->m_sock.open(session->m_ep.protocol());

        // 활성 세션 목록에 새 세션 추가
        // 다중 스레드에서 접근할 수 있기 때문에 동기화 필요
        std::unique_lock<std::mutex>
            lock(m_active_sessions_guard);
        m_active_sessions[request_id] = session;
        lock.unlock();

        session->m_sock.async_connect(session->m_ep,
            [this, session](const system::error_code& ec)
            {
                if (!ec) {
                    session->m_ec = ec;
                    onRequestComplete(session);
                    return;
                }

                std::unique_lock<std::mutex>
                    cancel_lock(session->m_cancel_guard);

                if (session->m_was_cancelled) {
                    onRequestComplete(session);
                    return;
                }

                asio::async_write(session->m_sock,
                    asio::buffer(session->m_request),
                    [this, session](const boost::system::error_code& ec,
                        std::size_t bytes_transferred)
                    {
                        if (!ec) {
                            session->m_ec = ec;
                            onRequestComplete(session);
                            return;
                        }

                        std::unique_lock<std::mutex>
                            cancel_lock(session->m_cancel_guard);

                        if (session->m_was_cancelled) {
                            onRequestComplete(session);
                            return;
                        }

                        asio::async_read_until(session->m_sock,
                            session->m_response_buf,
                            '\n',
                            [this, session](const boost::system::error_code& ec,
                                std::size_t bytes_transferred)
                            {
                                if (!ec) {
                                    session->m_ec = ec;
                                }
                                else {
                                    std::istream strm(&session->m_response_buf);
                                    std::getline(strm, session->m_response);
                                }

                                onRequestComplete(session);
                            }); }); });
    };

    // 요청 취소
    void cancelRequest(unsigned int request_id) {
        std::unique_lock<std::mutex>
            lock(m_active_sessions_guard);

        auto it = m_active_sessions.find(request_id);
        if (it != m_active_sessions.end()) {
            std::unique_lock<std::mutex>
                cancel_lock(it->second->m_cancel_guard);

            it->second->m_was_cancelled = true;
            it->second->m_sock.cancel();
        }
    }

    void close() {
        // 작업 객체를 파괴하여 I/O 스레드가 대기 상태에서 빠져나갈 수 있도록 함
        m_work.reset(NULL);

        // I/O 스레드 종료 대기
        m_thread->join();
    }

private:
    void onRequestComplete(std::shared_ptr<Session> session) {
        // 연결 종료 시도. 실패해도 오류 코드에 신경쓰지 않음
        boost::system::error_code ignored_ec;

        session->m_sock.shutdown(
            asio::ip::tcp::socket::shutdown_both,
            ignored_ec);

        // 활성 세션 목록에서 세션 제거
        std::unique_lock<std::mutex>
            lock(m_active_sessions_guard);

        auto it = m_active_sessions.find(session->m_id);
        if (it != m_active_sessions.end())
            m_active_sessions.erase(it);

        lock.unlock();

        boost::system::error_code ec;

        if (session->m_ec.value() == 0 && session->m_was_cancelled)
            ec = asio::error::operation_aborted;
        else
            ec = session->m_ec;

        // 사용자 제공 콜백 호출
        session->m_callback(session->m_id,
            session->m_response, ec);
    };

private:
    asio::io_context m_ios;
    std::map<int, std::shared_ptr<Session>> m_active_sessions;
    std::mutex m_active_sessions_guard;
    std::unique_ptr<boost::asio::io_context::work> m_work;
    std::unique_ptr<std::thread> m_thread;
};

void handler(unsigned int request_id,
    const std::string& response,
    const system::error_code& ec)
{
    if (!ec) {
        std::cout << "Request #" << request_id
            << "가 완료되었습니다. 응답: "
            << response << std::endl;
    }
    else if (ec == asio::error::operation_aborted) {
        std::cout << "Request #" << request_id
            << "가 사용자가 취소했습니다."
            << std::endl;
    }
    else {
        std::cout << "Request #" << request_id
            << " 실패! 오류 코드 = " << ec.value()
            << ". 오류 메시지 = " << ec.message()
            << std::endl;
    }

    return;
}

int main()
{
    try {
        AsyncTCPClient client;

        // 사용자가 요청을 시작함

        // ID 1인 요청 시작
        client.emulateLongComputationOp(10, "127.0.0.1", 3333,
            handler, 1);
        // 5초 동안 아무 작업도 하지 않음
        std::this_thread::sleep_for(std::chrono::seconds(5));
        // ID 2인 요청 시작
        client.emulateLongComputationOp(11, "127.0.0.1", 3334,
            handler, 2);
        // ID 1인 요청을 취소
        client.cancelRequest(1);
        // 6초 동안 아무 작업도 하지 않음
        std::this_thread::sleep_for(std::chrono::seconds(6));
        // ID 3인 요청 시작
        client.emulateLongComputationOp(9, "127.0.0.1", 3335,
            handler, 3);

        // 모든 요청이 완료될 때까지 대기
        std::this_thread::sleep_for(std::chrono::seconds(30));
        client.close();
    }
    catch (std::exception& e) {
        std::cerr << "예외: " << e.what() << std::endl;
    }

    return 0;
}                            
