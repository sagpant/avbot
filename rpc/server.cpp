/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2013  microcai <microcai@fedoraproject.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <memory>
#include <functional>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>
namespace pt = boost::property_tree;
#include <boost/property_tree/json_parser.hpp>
namespace js = boost::property_tree::json_parser;
#include <boost/circular_buffer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/async_coro_queue.hpp>

#include <avhttp/detail/parsers.hpp>

#include "boost/avloop.hpp"
#include "acceptor/acceptor.hpp"

#include <soci-sqlite3.h>
#include <boost-optional.h>
#include <boost-tuple.h>
#include <boost-gregorian-date.h>

#include "rpc/server.hpp"
#include "avhttpd.hpp"
#include "../libavbot/avchannel.hpp"

// avbot_rpc_server 由 acceptor_server 这个辅助类调用
// 为其构造函数传入一个 m_socket, 是 shared_ptr 的.
class rpc_connection
	: boost::asio::coroutine
	, public std::enable_shared_from_this<rpc_connection>
{
public:
	typedef boost::signals2::signal <
		void( channel_identifier, avbotmsg )
	> on_message_signal_type;

	on_message_signal_type &broadcast_message;

	typedef boost::asio::ip::tcp Protocol;
	typedef boost::asio::ip::tcp::socket socket_type;

	template<typename T>
	explicit rpc_connection(boost::asio::io_service& io, socket_type&& _socket,
		on_message_signal_type & on_message, T do_search_func)
		: m_io_service(io)
		, m_socket( std::move(_socket) )
		, m_streambuf( new boost::asio::streambuf )
		, m_responses(boost::ref(m_io_service), 20)
		, broadcast_message(on_message)
		, do_search(do_search_func)
	{
	}

	void start()
	{
		avloop_idle_post(m_socket.get_io_service(),
			std::bind<void>(&rpc_connection::client_loop, shared_from_this(),
					boost::system::error_code(), 0 )
		);
		m_connect = broadcast_message.connect(std::bind<void>(&rpc_connection::callback_message, this, std::placeholders::_1, std::placeholders::_2));
	}
private:
	void get_response_sended(std::shared_ptr< boost::asio::streambuf > v, boost::system::error_code ec, std::size_t);
	void on_pop(std::shared_ptr<boost::asio::streambuf> v);

	// 循环处理客户端连接.
	void client_loop(boost::system::error_code ec, std::size_t bytestransfered);

	// signal 的回调到这里
	void callback_message(channel_identifier, avbotmsg);
	void done_search(boost::system::error_code ec, boost::property_tree::wptree);
private:
	boost::asio::io_service& m_io_service;
	socket_type m_socket;

	boost::signals2::scoped_connection m_connect;

	std::shared_ptr<boost::asio::streambuf> m_streambuf;
	avhttpd::request_opts m_request;

	boost::async_coro_queue<
		boost::circular_buffer_space_optimized<
			std::shared_ptr<boost::asio::streambuf>
		>
	> m_responses;

	std::function<void(
		std::string c,
		std::string q,
		std::string date,
		std::function<void (boost::system::error_code, pt::wptree)> cb
	)> do_search;

	std::function<void(std::string,avbotmsg)> send_broadcast_message;

	int process_post( std::size_t bytestransfered );
};


/**
 * avbot rpc 接受的 JSON 格式为
 *
 * 	{
		"protocol":"rpc",
		"channel":"",  // 留空表示所有频道广播
		"message":{
			"text" : "text message"
		}
	}
 */
int rpc_connection::process_post( std::size_t bytes_transfered )
{
	pt::wptree msg;
	std::string messagebody;
	messagebody.resize( bytes_transfered );
	m_streambuf->sgetn( &messagebody[0], bytes_transfered );

	try
	{
		auto channelname = avhttp::detail::wide_utf8(msg.get<std::wstring>(L"channel"));
		// TODO 格式化成 avbotmsg
		send_broadcast_message(channelname, from_json_string(messagebody));
	}
	catch( const pt::ptree_error &err )
	{
		// 其他错误.
		return avhttpd::errc::internal_server_error;
	}

	return 200;
}

void rpc_connection::get_response_sended(std::shared_ptr< boost::asio::streambuf > v,
	boost::system::error_code ec, std::size_t bytes_transfered)
{
	m_socket.get_io_service().post(
		std::bind(&rpc_connection::client_loop, shared_from_this(), ec, bytes_transfered)
	);
}

// 发送数据在这里
void rpc_connection::on_pop(std::shared_ptr<boost::asio::streambuf> v)
{
	avhttpd::response_opts opts;
	opts.insert(avhttpd::http_options::content_type, "application/json; charset=utf8");
	opts.insert(avhttpd::http_options::content_length, boost::lexical_cast<std::string>(v->size()));
	opts.insert("Cache-Control", "no-cache");
	opts.insert(avhttpd::http_options::connection, "keep-alive");
	opts.insert(avhttpd::http_options::http_version,
		m_request.find(avhttpd::http_options::http_version)
	);

	avhttpd::async_write_response(
		m_socket, 200, opts, *v,
		std::bind(&rpc_connection::get_response_sended, shared_from_this(), v, std::placeholders::_1, std::placeholders::_2)
	);
}

void rpc_connection::done_search(boost::system::error_code ec, boost::property_tree::wptree jsonout)
{
	std::shared_ptr<boost::asio::streambuf> v = std::make_shared<boost::asio::streambuf>();

	std::ostream outstream(v.get());

	std::wstringstream jsondata;
	boost::property_tree::json_parser::write_json(jsondata, jsonout);
	outstream << boost::locale::conv::utf_to_utf<char>(jsondata.str());

	avhttpd::response_opts opts;
	opts.insert(avhttpd::http_options::content_type, "application/json; charset=utf8");
	opts.insert(avhttpd::http_options::content_length, boost::lexical_cast<std::string>(v->size()));
	opts.insert("Cache-Control", "no-cache");
	opts.insert(avhttpd::http_options::connection, "keep-alive");
	opts.insert(avhttpd::http_options::http_version,
		m_request.find(avhttpd::http_options::http_version)
	);


	avhttpd::async_write_response(
		m_socket, 200, opts, *v,
		std::bind<void>(&rpc_connection::get_response_sended, shared_from_this(), v, std::placeholders::_1,std::placeholders::_2)
	);
}

// 数据操作跑这里，嘻嘻.
void rpc_connection::client_loop(boost::system::error_code ec, std::size_t bytestransfered)
{
	std::string uri;

	boost::smatch what;
	//for (;;)
	BOOST_ASIO_CORO_REENTER(this)
	{for (;;){

		m_request.clear();
		m_streambuf = std::make_shared<boost::asio::streambuf>();

		// 读取用户请求.
		BOOST_ASIO_CORO_YIELD avhttpd::async_read_request(
				m_socket, *m_streambuf, m_request,
				std::bind(&rpc_connection::client_loop, shared_from_this(),std::placeholders::_1, 0)
		);

		if(ec)
		{
			if (ec == avhttpd::errc::post_without_content)
			{
				BOOST_ASIO_CORO_YIELD avhttpd::async_write_response(m_socket, avhttpd::errc::no_content,
					std::bind(&rpc_connection::client_loop, shared_from_this(),std::placeholders::_1, 0)
				);
				return;
			}
			else if (ec == avhttpd::errc::header_missing_host)
			{
				BOOST_ASIO_CORO_YIELD avhttpd::async_write_response(m_socket, avhttpd::errc::bad_request,
					std::bind(&rpc_connection::client_loop, shared_from_this(),std::placeholders::_1, 0)
				);
				return;
			}
			return;
		}

		uri = m_request.find(avhttpd::http_options::request_uri);

		// 解析 HTTP
		if(m_request.find(avhttpd::http_options::request_method) == "GET" )
		{
			if(uri=="/message")
			{
				// 等待消息, 并发送.
				BOOST_ASIO_CORO_YIELD m_responses.async_pop(
					std::bind(&rpc_connection::on_pop, shared_from_this(), std::placeholders::_2)
				);
			}
			else if(
				boost::regex_match(uri, what,
					boost::regex("/search\\?channel=([^&]*)&q=([^&]*)&date=([^&]*).*")
				)
			)
			{
				// 取出这几个参数, 到数据库里查找, 返回结果吧.
				BOOST_ASIO_CORO_YIELD do_search(what[1], what[2], what[3],
					std::bind(&rpc_connection::done_search, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
				);
				return;
			}
			else if(boost::regex_match(uri, what,boost::regex("/search(\\?)?")))
			{
				// missing parameter
				BOOST_ASIO_CORO_YIELD avhttpd::async_write_response(
					m_socket,
					avhttpd::errc::internal_server_error,
					std::bind(
						&rpc_connection::client_loop,
						shared_from_this(),
						std::placeholders::_1, 0
					)
				);
				return;
			}
			else if (boost::regex_match(uri, what,boost::regex("/status(\\?)?")))
			{
				// 获取 avbot 的状态.
				//boost::regex_match();
			}
			else
			{
				BOOST_ASIO_CORO_YIELD avhttpd::async_write_response(
					m_socket,
					avhttpd::errc::not_found,
					std::bind(
						&rpc_connection::client_loop,
						shared_from_this(),
						std::placeholders::_1, 0
					)
				);
				return;
			}
		}
		else if( m_request.find(avhttpd::http_options::request_method) == "POST")
		{
			// 这里进入 POST 处理.
			// 读取 body
			BOOST_ASIO_CORO_YIELD boost::asio::async_read(
				m_socket,
				*m_streambuf,
				boost::asio::transfer_exactly(
					boost::lexical_cast<std::size_t>(
						m_request.find(avhttpd::http_options::content_length)
					) - m_streambuf->size()
				),
				std::bind(&rpc_connection::client_loop, shared_from_this(), std::placeholders::_1, std::placeholders::_2 )
			);
			// body 必须是合法有效的 JSON 格式
			BOOST_ASIO_CORO_YIELD avhttpd::async_write_response(
					m_socket,
					process_post(m_streambuf->size()),
					avhttpd::response_opts()
						(avhttpd::http_options::content_length, "4")
						(avhttpd::http_options::content_type, "text/plain")
						("Cache-Control", "no-cache")
						(avhttpd::http_options::http_version,
							m_request.find(avhttpd::http_options::http_version)),
					boost::asio::buffer("done"),
					std::bind(&rpc_connection::client_loop, shared_from_this(), std::placeholders::_1, 0)
			);
			if ( m_request.find(avhttpd::http_options::connection) != "keep-alive" )
				return;
		}

		// 继续
		BOOST_ASIO_CORO_YIELD avloop_idle_post(m_socket.get_io_service(),
			std::bind(&rpc_connection::client_loop, shared_from_this(), ec, 0)
		);
	}}
}

void rpc_connection::callback_message(channel_identifier, avbotmsg)
{
	std::shared_ptr<boost::asio::streambuf> buf(new boost::asio::streambuf);
	std::ostream stream(buf.get());
	std::stringstream teststream;

	// TODO 格式化为 json
	// js::write_json(stream, jsonmessage);

	m_responses.push(buf);
}

void avlog_do_search(boost::asio::io_service & io_service, boost::logger& logger, std::string c, std::string q, std::string date,
	std::function<void (boost::system::error_code, pt::wptree)> handler, soci::session & db);

bool avbot_start_rpc(boost::asio::io_service & io_service, boost::logger& logger, int port, avbot & mybot, soci::session & avlogdb)
{
	using tcp = boost::asio::ip::tcp;
	std::shared_ptr<tcp::acceptor> accept_socket;

	accept_socket.reset(new tcp::acceptor(io_service));

	if (!acceptor::listen(*accept_socket, std::to_string(port), logger.err()))
	{
		return false;
	}

	acceptor::async_accept(*accept_socket, [&io_service]() -> tcp::socket{
		return boost::asio::ip::tcp::socket(io_service);
	}, [&io_service, &logger, &mybot, &avlogdb](tcp::socket&& sock, boost::system::error_code ec)
	{
		if (ec)
			return false;
		std::make_shared<rpc_connection>(io_service, std::move(sock), mybot.on_message,
			std::bind(&avlog_do_search, std::ref(io_service), std::ref(logger),
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
				std::ref(avlogdb)
			)
		)->start();
		return true;
	}, [accept_socket](){});

	return true;
}
