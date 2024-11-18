#include <boost/predef.h> // OS �ĺ� ����

// Windows XP, Windows Server 2003 ���Ͽ��� I/O �۾� ��� ����� Ȱ��ȭ�ϱ� ���� �ʿ�
// �ڼ��� ������ "http://www.boost.org/doc/libs/1_58_0/
// doc/html/boost_asio/reference/basic_stream_socket/"
// cancel/overload1.html" ����
#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINNT 0x0501

#if _WIN32_WINNT <= 0x0502 // Windows Server 2003 ����
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

// ��û�� �Ϸ�Ǿ��� �� ȣ��Ǵ� �ݹ� �Լ� ������ Ÿ��
typedef void(*Callback) (unsigned int request_id,
    const std::string& response,
    const system::error_code& ec);

// �ϳ��� ��û�� ���� ���ؽ�Ʈ�� ��Ÿ���� ����ü
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

    asio::ip::tcp::socket m_sock; // ��ſ� ���Ǵ� ����
    asio::ip::tcp::endpoint m_ep; // ���� ��������Ʈ
    std::string m_request;        // ��û ���ڿ�

    // ������ ������ streambuf
    asio::streambuf m_response_buf;
    std::string m_response; // ���� ���ڿ�

    // ��û �����ֱ� �� ������ �߻��ϸ� �� ������ �����
    system::error_code m_ec;

    unsigned int m_id; // ��û�� �Ҵ�� ���� ID

    // ��û�� �Ϸ�Ǿ��� �� ȣ��Ǵ� �ݹ� �Լ� ������
    Callback m_callback;

    bool m_was_cancelled; // ��û ��� ����
    std::mutex m_cancel_guard; // ��� �� ����ȭ�� ���� mutex
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

        // ��û ���ڿ� �غ�
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

        // Ȱ�� ���� ��Ͽ� �� ���� �߰�
        // ���� �����忡�� ������ �� �ֱ� ������ ����ȭ �ʿ�
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

    // ��û ���
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
        // �۾� ��ü�� �ı��Ͽ� I/O �����尡 ��� ���¿��� �������� �� �ֵ��� ��
        m_work.reset(NULL);

        // I/O ������ ���� ���
        m_thread->join();
    }

private:
    void onRequestComplete(std::shared_ptr<Session> session) {
        // ���� ���� �õ�. �����ص� ���� �ڵ忡 �Ű澲�� ����
        boost::system::error_code ignored_ec;

        session->m_sock.shutdown(
            asio::ip::tcp::socket::shutdown_both,
            ignored_ec);

        // Ȱ�� ���� ��Ͽ��� ���� ����
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

        // ����� ���� �ݹ� ȣ��
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
            << "�� �Ϸ�Ǿ����ϴ�. ����: "
            << response << std::endl;
    }
    else if (ec == asio::error::operation_aborted) {
        std::cout << "Request #" << request_id
            << "�� ����ڰ� ����߽��ϴ�."
            << std::endl;
    }
    else {
        std::cout << "Request #" << request_id
            << " ����! ���� �ڵ� = " << ec.value()
            << ". ���� �޽��� = " << ec.message()
            << std::endl;
    }

    return;
}

int main()
{
    try {
        AsyncTCPClient client;

        // ����ڰ� ��û�� ������

        // ID 1�� ��û ����
        client.emulateLongComputationOp(10, "127.0.0.1", 3333,
            handler, 1);
        // 5�� ���� �ƹ� �۾��� ���� ����
        std::this_thread::sleep_for(std::chrono::seconds(5));
        // ID 2�� ��û ����
        client.emulateLongComputationOp(11, "127.0.0.1", 3334,
            handler, 2);
        // ID 1�� ��û�� ���
        client.cancelRequest(1);
        // 6�� ���� �ƹ� �۾��� ���� ����
        std::this_thread::sleep_for(std::chrono::seconds(6));
        // ID 3�� ��û ����
        client.emulateLongComputationOp(9, "127.0.0.1", 3335,
            handler, 3);

        // ��� ��û�� �Ϸ�� ������ ���
        std::this_thread::sleep_for(std::chrono::seconds(30));
        client.close();
    }
    catch (std::exception& e) {
        std::cerr << "����: " << e.what() << std::endl;
    }

    return 0;
}                            
