
#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/signals2.hpp>
#include <boost/atomic.hpp>

#include <avproto.hpp>
#include <avproto/avjackif.hpp>

#include "avim_group.hpp"

class avim_group_impl : public std::enable_shared_from_this<avim_group_impl>
{
public:
	avim_group_impl(boost::asio::io_service& io, std::string key, std::string cert, std::string groupdeffile);
	~avim_group_impl();

public:
	void start();

	void start_login();

	void send_group_message(std::string sender, std::vector<avim_msg>);

	// callback when there is a message
	boost::signals2::signal<void(std::string reciver, std::string sender, std::vector<avim_msg>)> on_message;
	boost::signals2::signal<void(std::string roomname)> on_group_created;
	boost::atomic<bool> m_quitting;

private:
	void internal_login_coroutine(boost::asio::yield_context);
	void internal_loop_coroutine(boost::asio::yield_context);

	void forward_client_message( std::string );

private:
	boost::asio::io_service& m_io_service;

	std::shared_ptr<RSA> m_key;
	std::shared_ptr<X509> m_cert;

	avkernel m_core;

	std::shared_ptr<avjackif> m_con;
	std::string m_me_addr;

    std::string m_groupdef;

	boost::signals2::signal<void(std::string content)> recive_client_message;
};
