#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <memory>
#include <iostream>

using namespace boost;

class Service {
public:
	Service() {}

	void HandleClient(asio::ip::tcp::socket& sock) {
		try {
			asio::streambuf request;
			asio::read_until(sock, request, '\n');

			// ��û ó�� ���ķ��̼�
			int i = 0;
			while (i != 1000000)
				i++;
			std::this_thread::sleep_for(
				std::chrono::milliseconds(500));

			// ���� ����
			std::string response = "Response\n";
			asio::write(sock, asio::buffer(response));
		}
		catch (system::system_error& e) {
			std::cout << "���� �߻�! ���� �ڵ� = "
				<< e.code() << ". �޽���: "
				<< e.what();
		}
	}
};

class Acceptor {
public:
	Acceptor(asio::io_context& ioc, unsigned short port_num) :
		m_ioc(ioc),
		m_acceptor(m_ioc,
			asio::ip::tcp::endpoint(
				asio::ip::address_v4::any(),
				port_num))
	{
		m_acceptor.listen();
	}

	void Accept() {
		asio::ip::tcp::socket sock(m_ioc);

		m_acceptor.accept(sock);

		Service svc;
		svc.HandleClient(sock);
	}

private:
	asio::io_context& m_ioc;
	asio::ip::tcp::acceptor m_acceptor;
};

class Server {
public:
	Server() : m_stop(false), m_work_guard(asio::make_work_guard(m_ioc)) {} // �����ڿ��� work_guard �ʱ�ȭ

	void Start(unsigned short port_num) {
		m_thread.reset(new std::thread([this, port_num]() {
			Run(port_num);
			}));
	}

	void Stop() {
		m_stop.store(true);
		m_work_guard.reset(); // io_context�� �۾��� ������ �˸�
		m_thread->join();
	}

private:
	void Run(unsigned short port_num) {
		Acceptor acc(m_ioc, port_num);

		while (!m_stop.load()) {
			acc.Accept();
		}
	}

	std::unique_ptr<std::thread> m_thread;
	std::atomic<bool> m_stop;
	asio::io_context m_ioc;
	asio::executor_work_guard<asio::io_context::executor_type> m_work_guard; // �����ڿ��� �ʱ�ȭ�� work_guard
};

int main()
{
	unsigned short port_num = 3333;

	try {
		Server srv;
		srv.Start(port_num);

		std::this_thread::sleep_for(std::chrono::seconds(60));

		srv.Stop();
	}
	catch (system::system_error& e) {
		std::cout << "���� �߻�! ���� �ڵ� = "
			<< e.code() << ". �޽���: "
			<< e.what();
	}

	return 0;
}
