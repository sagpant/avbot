/*
 * Copyright (C) 2013-2014  微蔡 <microcai@fedoraproject.org>
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
#include <string>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/regex.hpp>
//#include <boost/type_traits/remove_reference.hpp>
#include <type_traits>
#include <avbotui.hpp>

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

namespace decoder{

namespace detail{

inline bool is_vc(std::string str)
{
	boost::cmatch what;
	boost::regex ex("[a-z0-9A-Z][a-z0-9A-Z][a-z0-9A-Z][a-z0-9A-Z]");
	return boost::regex_match(str.c_str(), what, ex);
}

template<class Handler>
class ui_decoder_op : boost::asio::coroutine
{
public:
	ui_decoder_op(boost::asio::io_service & io_service, const std::string & buffer, Handler handler)
		: m_io_service(io_service)
		, m_handler(handler)
		, m_buffer(buffer)
	{
	}

	void operator()(std::string str)
	{
		std::string tmp;

		bool re_do = false;

		BOOST_ASIO_CORO_REENTER(this)
		{
			do{
				// 等待输入
				BOOST_ASIO_CORO_YIELD show_vc_and_get_input_with_timeout(m_buffer, 30, *this);

				str = boost::trim_copy(str);
				// 检查 str
				if(str.length() == 4 && is_vc(str))
				{
					// 是 vc 的话就调用 handler
					m_io_service.post(
						boost::asio::detail::bind_handler(
							m_handler, boost::system::error_code(), std::string("GUI 验证码输入"), str, std::function<void()>()
						)
					);
					return;
				}

				// 重来
				re_do = (str!="");

			}while(re_do);

			m_io_service.post(
				boost::asio::detail::bind_handler(
					m_handler,
					boost::system::error_code(),
					std::string(" GUI 验证码输入"),
					std::string(),
					std::function<void()>()
				)
			);
		}
	}
private:
	boost::asio::io_service & m_io_service;
	std::string m_buffer;
	Handler m_handler;
};

}

class ui_decoder_t
{
public:
	ui_decoder_t(boost::asio::io_service & io_service)
	  : m_io_service(io_service)
	{
	}

	template <class Handler>
	void operator()(const std::string &buffer, Handler handler)
	{
		detail::ui_decoder_op<Handler>(m_io_service, buffer, handler)("");
	}

private:
	boost::asio::io_service & m_io_service;
};

ui_decoder_t ui_decoder(boost::asio::io_service & io_service)
{
	return ui_decoder_t(io_service);
}

} // namespace decoder
