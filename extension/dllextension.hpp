/*
 * Copyright (C) 2014  microcai <microcai@fedoraproject.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#ifndef __DLL_EXTENSION_HPP_
#define __DLL_EXTENSION_HPP_

#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>

#include <boost/cfunction.hpp>
#include "extension.hpp"

template<class MsgSender>
class dllextention_caller
{
private:
	boost::asio::io_service &io_service;
	std::string m_channel;
	typename boost::remove_reference<MsgSender>::type m_sender;
public:
	dllextention_caller(boost::asio::io_service & _io_service, std::string _channel, MsgSender sender)
		: m_sender(sender)
		, io_service(_io_service)
		, m_channel(_channel)
	{
	}

	typedef void (*message_sender_t)(const char * message, void* _apitag);

	typedef void (*avbot_on_message_t)(const char * speaker,const char * message, const char * channel, message_sender_t sender, void* _apitag);

	void operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context)
	{
		std::string textmsg = msg.to_plain_text();

		std::string speaker = msg.sender.nick;

		boost::thread t(boost::bind(&dllextention_caller<MsgSender>::call_dll_message, this, speaker, textmsg, m_channel, m_sender));
		t.detach();
	}

	void call_dll_message(std::string speaker, std::string textmsg, std::string channel, boost::function<void(std::string)> sender)
	{
		std::shared_ptr<void> dll_module;

		dll_module.reset(
			LoadLibraryW(L"avbotextension"),
			[](void * p){
			if (p)
			{
				FreeLibrary((HMODULE)p);
				OutputDebugStringW(L"已卸载 avbotextension.dll");
			}
		});

		if (dll_module)
		{
			OutputDebugStringW(L"已加载 avbotextension.dll!!!");
		}
		else
		{
			OutputDebugStringW(L"没有找到 avbotextension.dll!!!");
			return;
		}


		auto p = GetProcAddress((HMODULE)(dll_module.get()), "avbot_on_message2");
		avbot_on_message_t f = reinterpret_cast<avbot_on_message_t>(p);

		if (!f)
		{
			MessageBoxW(0, L"avbot_on_message2 函数没找到", L"avbotdllextension.dll", MB_OK);
		}

		// 调用 f 吧！
		boost::cfunction<message_sender_t, void(std::string)> sender_for_c = sender;
		f(speaker.c_str(), textmsg.c_str(), channel.c_str(), sender_for_c.c_func_ptr(), sender_for_c.c_void_ptr());

		OutputDebugStringW(L"called avbot_on_message2()");
	}
};

template<class MsgSender>
avbot_extension make_dllextention(boost::asio::io_service & io_service,
	std::string _channel, const MsgSender & sender)
{
	return avbot_extension(
		_channel,
		dllextention_caller<typename boost::remove_reference<MsgSender>::type>(io_service, _channel, sender)
	);
}

#endif // __DLL_EXTENSION_HPP_
