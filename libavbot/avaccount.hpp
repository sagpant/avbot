#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <boost/signals2.hpp>

#include "avchannel.hpp"

// 一个基类, 用于实现用户自定义帐号, 以支持非 qq/xmpp/irc/avim 帐号
class avaccount
{
public:
	// 这个就是子类里必须要发射的信号
	boost::signals2::signal<void(channel_identifier, avbotmsg)> on_message;

	// 这个是子类里必须要实现的用于发送消息
	std::function<void(channel_identifier, avbotmsg)> send_message;
};
