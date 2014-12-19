#pragma once
#include <boost/asio/io_service.hpp>
#include <functional>
#include <memory>

class avim_group_impl;

struct avim_msg{
	std::string text;
	std::string image, image_cname;
	std::string url;
};

class avim
{
public:
	avim(boost::asio::io_service& io, std::string key, std::string cert, std::string groupdeffile);
	~avim();

	void on_message(std::function<void(std::string reciver, std::string sender, std::vector<avim_msg>)>);
	void on_group_created(std::function<void(std::string)>);
	void send_group_message(std::string speaker, std::vector<avim_msg>);

private:
	boost::asio::io_service& m_io_service;
	std::shared_ptr<avim_group_impl> m_impl;
};
