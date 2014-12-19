
#include <boost/circular_buffer.hpp>

#include <vector>
#include <string>
#include <iostream>


#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/signals2.hpp>
#include <boost/async_coro_queue.hpp>
#include "avhttp/logging.hpp"
#include "boost/avproxy.hpp"
#include "boost/timedcall.hpp"
#include "boost/logger.hpp"

#include "./irc.hpp"

namespace irc {
namespace impl {

class client_impl : public std::enable_shared_from_this<client_impl>
{
public:
	client_impl(boost::asio::io_service &_io_service, boost::logger& _logger,
		const std::string& user, const std::string& user_pwd = "", const std::string& server = "irc.freenode.net",
		const unsigned int max_retry_count = 1000)
		: io_service(_io_service)
		, socket_(io_service)
		, user_(user)
		, pwd_(user_pwd)
		, server_(server)
		, retry_count_(max_retry_count)
		, c_retry_cuont(0)
		, quitting_(false)
		, messages_send_queue_(_io_service, 50) // 缓存最后  50 条消息
		, irc_command_send_queue_(_io_service, 200) // 缓存最后  200 条命令
		, logger(_logger)
	{
	}

	boost::asio::io_service & get_io_service()
	{
		return io_service;
	}

	void start();

	void stop()
	{
		quitting_ = true;
		boost::system::error_code ec;
		socket_.close(ec);
	}

public:
	void on_privmsg_message(const privmsg_cb &cb)
	{
		cb_.connect(cb);
	}

	void join(const std::string& ch, const std::string &pwd)
	{
		std::string msg;

		pwd.empty() ? msg = "JOIN " + ch : msg = "JOIN " + ch + " " + pwd;

		join_queue_.push_back(msg);

		get_io_service().post(boost::bind(boost::ref(on_new_room_cb), ch));
	}

	void chat(const std::string whom, const std::string msg)
	{
		std::vector<std::string> msgs;
		boost::split(msgs, msg, boost::is_any_of("\r\n"));
		BOOST_FOREACH(std::string _msg,  msgs)
		{
			if(_msg.length() > 0)
				messages_send_queue_.push("PRIVMSG " + whom + " :" + _msg);
		}
	}

	void send_command(const std::string& cmd)
	{
		std::string data = cmd + "\r\n";
		irc_command_send_queue_.push(data);
	}

	void oper(const std::string& user, const std::string& pwd)
	{
		send_command("OPER " + user + " " + pwd);
	}

public:
	void process_request(std::size_t bytes_transferred)
	{
		boost::smatch what;
		std::string req;

		req.resize(bytes_transferred);

		response_.sgetn(&req[0], bytes_transferred);

		req.resize(bytes_transferred - 2);

#ifdef DEBUG
		std::cout << req << std::endl;
#endif

		//add auto modify a nick
		if(req.find("Nickname is already in use.") != std::string::npos)
		{
			user_ += "_";
			send_command("NICK " + user_);
			send_command("USER " + user_ + " 0 * " + user_);

			BOOST_FOREACH(std::string & str, join_queue_)
			send_command(str);
			return;
		}

		//PING :5211858A
		if (boost::regex_match(req, what, boost::regex("PING ([^ ]+).*")))
		{
			send_command("PONG " + what[1]);
			return;
		}

		if (boost::regex_match(req, what,
			boost::regex(":([^!]+)!([^ ]+) PRIVMSG ([^ ]+) :(.*)[\\r\\n]*")))
		{
			irc_msg m;
			m.whom = what[1];
			m.locate = what[2];
			m.from = what[3];
			m.msg = what[4];

			cb_(m);

			c_retry_cuont = 0;
		};
	}

public:
	boost::asio::io_service& io_service;

	std::string user_;
	std::string pwd_;
	std::string server_;

	boost::asio::streambuf response_;
	boost::signals2::signal<void(irc_msg)> cb_;
	boost::signals2::signal<void(std::string)> on_new_room_cb;

	std::vector<std::string> join_queue_;

	const unsigned int retry_count_;
	unsigned int c_retry_cuont;

	boost::asio::ip::tcp::socket socket_;

	bool quitting_;

	boost::async_coro_queue<
		boost::circular_buffer_space_optimized<
			std::string
		>
	> irc_command_send_queue_;

	boost::async_coro_queue<
		boost::circular_buffer_space_optimized<
			std::string
		>
	> messages_send_queue_;

	boost::logger& logger;
};



class msg_sender_loop : boost::asio::coroutine
{
public:
	msg_sender_loop(std::shared_ptr<client_impl> _client)
		: m_client(_client)
		, m_value(std::make_shared<std::string>())
		, m_last_line(std::make_shared<std::string>())
	{
		boost::system::error_code ec;
		m_client->irc_command_send_queue_.async_pop(
			std::bind<void>(*this, std::placeholders::_1, 0, std::placeholders::_2)
		);
	}

	void operator()(boost::system::error_code ec,
		std::size_t bytes_transferred, std::string value)
	{
		int i;

		BOOST_ASIO_CORO_REENTER(this)
		{for (;!m_client->quitting_ && ec != boost::system::errc::operation_canceled;) {
			*m_value = value;
			// 发送
			BOOST_ASIO_CORO_YIELD boost::asio::async_write(
				m_client->socket_,
				boost::asio::buffer(*m_value),
				boost::asio::transfer_all(),
				std::bind<void>(*this, std::placeholders::_1, std::placeholders::_2, value)
			);

			if (ec)
			{
				// 错误? 恩 ~~~ 糟糕咯
				m_client->socket_.close(ec);
				return;
			}

			BOOST_ASIO_CORO_YIELD boost::delayedcallms(
				m_client->get_io_service(), 468,
				std::bind<void>(*this, ec, 0, value)
			);

			BOOST_ASIO_CORO_YIELD m_client->irc_command_send_queue_.async_pop(
				std::bind<void>(*this, std::placeholders::_1, 0, std::placeholders::_2)
			);
		}}
	}

private:
	template<class Handler>
	void async_send_line(std::string line, Handler handler)
	{
		*m_last_line = line + "\r\n";

		boost::asio::async_write(
			m_client->socket_,
			boost::asio::buffer(*m_last_line),
			handler
		);
	};
private:
	std::shared_ptr<client_impl> m_client;
	std::shared_ptr<std::string> m_value;
	std::shared_ptr<std::string> m_last_line;
};

msg_sender_loop make_sender_loop(std::shared_ptr<client_impl> _client)
{
	return msg_sender_loop(_client);
}

class msg_reader_loop
{
public:
	msg_reader_loop(std::shared_ptr<client_impl> _client)
		: m_client(_client)
	{
		boost::asio::async_read_until(
			m_client->socket_,
			m_client->response_,
			"\r\n",
			*this
		);
	}

	void operator()(boost::system::error_code ec, std::size_t bytes_transferred)
	{
		if (ec) {
			m_client->socket_.close(ec);
			// 立即取消等待!
			m_client->messages_send_queue_.cancele();
			return;
		}

		m_client->process_request(bytes_transferred);

		boost::asio::async_read_until(
			m_client->socket_,
			m_client->response_,
			"\r\n",
			*this
		);
	}

private:
	std::shared_ptr<client_impl> m_client;
};

msg_reader_loop make_msg_reader_loop(std::shared_ptr<client_impl> _client)
{
	return msg_reader_loop(_client);
}

class irc_main_loop : boost::asio::coroutine
{
public:
	irc_main_loop(std::shared_ptr<client_impl> _client)
		: m_client(_client)
	{
		// start routine now!
		m_client->io_service.post(
			std::bind<void>(*this, boost::system::error_code(), 0, std::string())
		);
	}

	void operator()(boost::system::error_code ec,
		std::size_t bytes_transferred, std::string message)
	{
		if (m_client->quitting_)
			return;

		BOOST_ASIO_CORO_REENTER(this)
		{
			// 成功? 失败?
			do
			{
				if (m_client->c_retry_cuont > m_client->retry_count_)
				{
					m_client->logger.err() <<  "irc: to many retries! quite irc!";
					return;
				}

				m_client->logger.info() <<  "irc: connecting to server: "
					<< m_client->server_  << " ...";

				BOOST_ASIO_CORO_YIELD async_connect_irc(
					std::bind<void>(*this, std::placeholders::_1, bytes_transferred, message)
				);

				if (ec)
				{

					m_client->logger.err() << "irc: error connecting to server : "
						<< ec.message();

					m_client->logger.info() <<  "retry in 15s ...";

					m_client->c_retry_cuont ++;

					BOOST_ASIO_CORO_YIELD boost::delayedcallsec(
						m_client->io_service,
						15,
						std::bind<void>(*this, ec, bytes_transferred, message)
					);
				}

			} while (ec);

			m_client->logger.info() <<  "irc: conneted to server "
				<< m_client->server_  << " .";

			m_client->response_.consume(m_client->response_.size());
			m_client->irc_command_send_queue_.clear();

			// 立即开启 读协程.
			make_msg_reader_loop(m_client);
			// 立即开启 写协程.
			make_sender_loop(m_client);

			// 读写协程进入后台完成

			// 开始登录.


			// 完成登录! 接着该发送登录数据了!
			// 登录过程.
			if(! m_client->pwd_.empty())
			{
				m_client->send_command("PASS " + m_client->pwd_);
			}

			m_client->send_command("NICK " + m_client->user_);

			m_client->send_command(
				"USER " + m_client->user_ + " 0 * " + m_client->user_
			);

			BOOST_ASIO_CORO_YIELD boost::delayedcallsec(
				m_client->get_io_service(), 2,
				std::bind<void>(*this, ec, bytes_transferred, message)
			);

			BOOST_ASIO_CORO_YIELD boost::delayedcallsec(
				m_client->io_service,
				5,
				std::bind<void>(*this, ec, bytes_transferred, message)
			);

			//  发送 join 信息,  加入频道.

			for ( i = 0; i < m_client->join_queue_.size(); i++)
			{
				m_client->send_command(m_client->join_queue_[i]);

				BOOST_ASIO_CORO_YIELD boost::delayedcallsec(
					m_client->get_io_service(), 2,
					std::bind<void>(*this, ec, bytes_transferred, message)
				);
			}

			// 登录完成, 进入开启消息循环.

			do {
				// 消息循环, 每次阻塞在 async_pop 上.
				BOOST_ASIO_CORO_YIELD m_client->messages_send_queue_.async_pop(
					std::bind<void>(*this, std::placeholders::_1, bytes_transferred, std::placeholders::_2)
				);

				if (!ec)
				{
					// 发送消息.
					m_client->send_command(message);

					// 等待 500ms ,  一秒最多发一条信息!
					BOOST_ASIO_CORO_YIELD boost::delayedcallms(
						m_client->get_io_service(), 488,
						std::bind<void>(*this, ec, bytes_transferred, message)
					);
				}

			} while (ec != boost::system::errc::operation_canceled);

			// 如果 pop 失败, 通常是读消息的协程遇到了错误, 并将 socket 关闭.
			// 说明需要重新登录!

			// 取消发送协程 !
			// 注意,  这个 abort 错误是读协程给出的,  所以读协程其实已经退出了.
			m_client->irc_command_send_queue_.cancele();

			// 等待 20s
			BOOST_ASIO_CORO_YIELD boost::delayedcallsec(
				m_client->get_io_service(), 20,
				std::bind<void>(*this, ec, bytes_transferred, message)
			);

			// 开启新的循环.
			make_main_loop(m_client);
			// 本协程退出
			return;
		}
	}

private:
	template<class Handler>
	void async_connect_irc(Handler handler)
	{
		using namespace boost::asio::ip;

		std::string server, port;
		boost::smatch what;

		if(boost::regex_match(m_client->server_, what,
			boost::regex("([a-zA-Z0-9\\.]+)(:([\\d]+))?")))
		{
			server = what[1];

			if(what[2].matched)
			{
				port = std::string(what[2]).substr(1);
			}
			else
			{
				port = "6667";
			}
		}
		else
		{
			boost::throw_exception(
				std::invalid_argument("bad server name for irc")
			);
		};

		avproxy::async_proxy_connect(
			avproxy::autoproxychain(
				m_client->socket_, tcp::resolver::query(server, port)
			),
			handler
		);
	}

private:
	std::shared_ptr<client_impl> m_client;

	int i;
public:
	static irc_main_loop make_main_loop(std::shared_ptr<client_impl> _client)
	{
		return irc_main_loop(_client);
	}
};

} // namespace impl

client::client(boost::asio::io_service& _io_service, boost::logger& _logger,
	const std::string& user, const std::string& user_pwd, const std::string& server,
	const unsigned int max_retry_count)
{
	impl = std::make_shared<impl::client_impl>(
		std::ref(_io_service), std::ref(_logger), user, user_pwd, server, max_retry_count
	);
	impl->start();
}
client::~client()
{
	impl->stop();
}

void client::on_privmsg_message(const privmsg_cb& cb)
{
	impl->on_privmsg_message(cb);
}

void client::on_new_room(std::function<void(std::string)> cb)
{
	impl->on_new_room_cb.connect(cb);
}

void client::join(const std::string& ch, const std::string& pwd)
{
	impl->join(ch, pwd);
}

void client::chat(const std::string whom, const std::string msg)
{
	impl->chat(whom, msg);
}

void impl::client_impl::start()
{
	impl::irc_main_loop::make_main_loop(shared_from_this());
}

} // namespace irc
