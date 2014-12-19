
#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem.hpp>

#include "extension.hpp"

class bulletin
{
	boost::asio::io_service &io_service;
	boost::function<void ( std::string ) > m_sender;
	std::shared_ptr<boost::asio::deadline_timer> m_timer;
	std::shared_ptr<std::vector<std::string> > m_settings;
	std::string m_channel_name;
	void load_settings();
	void schedule_next() const;
	void send_msg_file( std::string msgfile)const;
public:

	template<class MsgSender>
	bulletin( boost::asio::io_service &_io_service,  MsgSender sender, std::string channel_name )
	  : m_sender(sender)
	  , m_settings(new std::vector<std::string>)
	  , m_channel_name(channel_name)
	  , m_timer(new boost::asio::deadline_timer(_io_service))
	  , io_service(_io_service)
	{
		// 读取公告配置.
		// 公告配置分两个部分, 一个是公告文件  $qqlog/$channel_name/bulletin.txt
		// 公告文件每次发送的时候读取,  而不是一次性读取.
		// 另一个是公告频次配置
		// 里面是一个时间的数组
		// 时间是 YY-MM-DD-HH-MM 这个的格式
		// 留 * 表示 每
		// 比如 *-*-*-08-00 表示每天早上 8 点
		// 保存在  $qqlog/$channel_name/bulletin_setting

		// 那么, 读取公告设置吧!
		try{
			load_settings();
		}catch (std::runtime_error){}

		schedule_next();
	}

	// on_message 回调.
	void operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context) const;
	// 超时的回调 - 重算调度.
	void operator()(boost::system::error_code);
	// 超时的回调 - 回显.
	void operator()(boost::system::error_code, std::string);
};
