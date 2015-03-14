
/*
 * Copyright (C) 2012 - 2013  微蔡 <microcai@fedoraproject.org>
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
#include <memory>

#include <functional>
#include <boost/system/system_error.hpp>
#include <boost/asio.hpp>
#include <boost/asio/coroutine.hpp>

#include <avhttp.hpp>

namespace webqq {
namespace qqimpl {
class WebQQ;
namespace detail {

struct get_hash_file_op : boost::asio::coroutine
{
	template<class Handler>
	get_hash_file_op(std::shared_ptr<qqimpl::WebQQ> webqq, Handler handler)
		: m_webqq(webqq), m_handler(handler)
	{
	}

	void operator()(boost::system::error_code, std::size_t bytes_transfered = 0);

private:
	std::shared_ptr<qqimpl::WebQQ> m_webqq;
	std::function<void(boost::system::error_code)> m_handler;

	std::shared_ptr<avhttp::http_stream> m_stream;
	std::shared_ptr<boost::asio::streambuf> m_buffer;
};

template<class Handler>
void  async_get_hash_file(std::shared_ptr<qqimpl::WebQQ> webqq, Handler handler)
{
	get_hash_file_op(webqq, handler)(boost::system::error_code());
}

std::string hash_func(std::string x, std::string K);

} // namespace detail
} // namespace qqimpl
} // namespace webqq
