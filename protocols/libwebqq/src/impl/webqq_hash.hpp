
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

#include <iostream>
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace js = boost::property_tree::json_parser;

#include <boost/property_tree/json_parser.hpp>
namespace pt = boost::property_tree;

#include <boost/format.hpp>

#include <avhttp.hpp>
#include <avhttp/async_read_body.hpp>

#include <boost/hash.hpp>

#include "boost/timedcall.hpp"
#include "boost/urlencode.hpp"
#include "boost/stringencodings.hpp"

#include "webqq_impl.hpp"
#include "constant.hpp"
#include "lwqq_status.hpp"
#include "webqq_group_qqnumber.hpp"

namespace webqq {
namespace qqimpl {
namespace detail {

/*
u = function(uin, ptwebqq) {
	for (var N = ptwebqq + "password error", T = "", V = [];;)
		if (T.length <= N.length) {
		T += uin;
		if (T.length == N.length) break
		}
		else {
			T = T.slice(0, N.length);
			break
		}
		for (var U = 0; U < T.length; U++) V[U] = T.charCodeAt(U) ^ N.charCodeAt(U);
		N = ["0", "1", "2", "3",
			"4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"
		];
		T = "";
		for (U = 0; U < V.length; U++) {
			T += N[V[U] >> 4 & 15];
			T += N[V[U] & 15]
		}
		return T
}
*/
template<typename var>
std::string hash_func_u(var uin, var ptwebqq)
{
	var T;
	std::vector<unsigned char> V;
	var N = ptwebqq + "password error";
	for (;;)
	{
		if (T.length() <= N.length()) {
			T += uin;
			if (T.length() == N.length())
				break;
		}
		else {
			T = T.substr(0, N.length());
			break;
		}
	}

	V.resize(T.length());

	for (int U = 0; U < T.length(); U++)
	{
		V[U] = (unsigned)(T[U]) ^ (unsigned)(N[U]);
	}

	return avhttp::detail::to_hex(std::string((const char*)V.data(), V.size()));
}

} // namespace detail
} // namespace qqimpl
} // namespace webqq
