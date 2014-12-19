
#include <iostream>
#include <boost/make_shared.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <avhttpd.hpp>

class session_op{
public:
	session_op(boost::asio::io_service & io_service, boost::shared_ptr<boost::asio::ip::tcp::socket> clientsocket)
		: m_io_service(io_service)
		, m_clientsocket(clientsocket)
		, m_request_opts(boost::make_shared<avhttpd::request_opts>())
		, m_streambuf(boost::make_shared<boost::asio::streambuf>())
		, coro(0)
	{
		avhttpd::async_read_request(*m_clientsocket, *m_streambuf, *m_request_opts, *this);
	}

	void operator()(boost::system::error_code ec)
	{
		if (coro++ == 0 )
		{
			// print out request_opts
			std::cout <<  m_request_opts->header_string() << std::endl;

			if (m_request_opts->find(avhttpd::http_options::request_uri) == "/123")
			{
				avhttpd::response_opts opts;
				opts("content-length", "3");
				opts("connection", "close");
				opts("Content-Type", "text/plain");
				opts(avhttpd::http_options::http_version, m_request_opts->find(avhttpd::http_options::http_version));
				avhttpd::async_write_response(*m_clientsocket, 200, opts, boost::asio::buffer("123", 3), *this);
			}else
				avhttpd::async_write_response(*m_clientsocket, 200, *this);
		}
		else
		{

		}
	}

	void operator()(boost::system::error_code ec, std::size_t bytes_transfered)
	{
	}
private:
	boost::asio::io_service & m_io_service;
	boost::shared_ptr<boost::asio::ip::tcp::socket> m_clientsocket;
	boost::shared_ptr<avhttpd::request_opts> m_request_opts;
	boost::shared_ptr<boost::asio::streambuf> m_streambuf;

	int coro;
};

class async_accept_op{
public:
	async_accept_op(boost::asio::io_service & io_service, boost::asio::ip::tcp::acceptor & acceptor)
	  : m_io_service(io_service)
	  , m_acceptor(acceptor)
	{
		boost::shared_ptr<boost::asio::ip::tcp::socket> clientsocket
			= boost::make_shared<boost::asio::ip::tcp::socket>(boost::ref(m_io_service));
		m_acceptor.async_accept(*clientsocket,  boost::bind<void>(*this, _1, clientsocket));
	}

	void operator()(boost::system::error_code ec, boost::shared_ptr<boost::asio::ip::tcp::socket> clientsocket)
	{
		if (!ec)
			{session_op op(m_io_service, clientsocket);}
		{async_accept_op op(m_io_service, m_acceptor);}
	}

private:
	boost::asio::io_service & m_io_service;
	boost::asio::ip::tcp::acceptor & m_acceptor;
};

int main(int argc, char **argv)
{
	boost::asio::io_service io_service;

	boost::asio::ip::tcp::acceptor acceptor(io_service,
		boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 4000)
	);

	{async_accept_op op(io_service, acceptor);}

    io_service.run();
    return 0;
}
