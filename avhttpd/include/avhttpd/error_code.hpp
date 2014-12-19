//
// http_stream.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
// Copyright (c) 2013 microcai ( microcaiicai at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __AVHTTPD_ERROR_CODEC_HPP__
#define __AVHTTPD_ERROR_CODEC_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <string>
#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>

#ifndef BOOST_SYSTEM_NOEXCEPT
#define BOOST_SYSTEM_NOEXCEPT BOOST_NOEXCEPT
#endif

namespace avhttpd {

namespace detail {
	class error_category_impl;
}

template<class error_category>
const boost::system::error_category& error_category_single()
{
	static error_category error_category_instance;
	return reinterpret_cast<const boost::system::error_category&>(error_category_instance);
}

inline const boost::system::error_category& error_category()
{
	return error_category_single<detail::error_category_impl>();
}

namespace errc {

/// HTTP error codes.
/**
 * The enumerators of type @c errc_t are implicitly convertible to objects of
 * type @c boost::system::error_code.
 *
 * @par Requirements
 * @e Header: @c <error_codec.hpp> @n
 * @e Namespace: @c avhttp::errc
 */
enum errc_t
{
	// Client-generated errors.

	/// The request's uri line was malformed.
	malformed_request_line = 1,

	/// Invalid request method
	invalid_request_method,

	/// HTTP/1.1 demand that client send Host: header
	header_missing_host,

	post_without_content,

	/// The request's headers were malformed.
	malformed_request_headers,

	/// Header too large
	header_too_large,

	/// Invalid chunked encoding.
	invalid_chunked_encoding,

	// Server-generated status codes.

	version_not_supported = 505,
};

enum status_code{
		/// The server-generated status code "200 OK".
	ok = 200,

	/// The server-generated status code "201 Created".
	created = 201,

	/// The server-generated status code "202 Accepted".
	accepted = 202,

	/// The server-generated status code "203 Non-Authoritative Information".
	non_authoritative_information = 203,

	/// The server-generated status code "204 No Content".
	no_content = 204,

	/// The server-generated status code "205 Reset Content".
	reset_content = 205,

	/// The server-generated status code "206 Partial Content".
	partial_content = 206,

	/// The server-generated status code "300 Multiple Choices".
	multiple_choices = 300,

	/// The server-generated status code "301 Moved Permanently".
	moved_permanently = 301,

	/// The server-generated status code "302 Found".
	found = 302,

	/// The server-generated status code "303 See Other".
	see_other = 303,

	/// The server-generated status code "304 Not Modified".
	not_modified = 304,

	/// The server-generated status code "305 Use Proxy".
	use_proxy = 305,

	/// The server-generated status code "307 Temporary Redirect".
	temporary_redirect = 307,

	/// The server-generated status code "400 Bad Request".
	bad_request = 400,

	/// The server-generated status code "401 Unauthorized".
	unauthorized = 401,

	/// The server-generated status code "402 Payment Required".
	payment_required = 402,

	/// The server-generated status code "403 Forbidden".
	forbidden = 403,

	/// The server-generated status code "404 Not Found".
	not_found = 404,

	/// The server-generated status code "405 Method Not Allowed".
	method_not_allowed = 405,

	/// The server-generated status code "406 Not Acceptable".
	not_acceptable = 406,

	/// The server-generated status code "407 Proxy Authentication Required".
	proxy_authentication_required = 407,

	/// The server-generated status code "408 Request Time-out".
	request_timeout = 408,

	/// The server-generated status code "409 Conflict".
	conflict = 409,

	/// The server-generated status code "410 Gone".
	gone = 410,

	/// The server-generated status code "411 Length Required".
	length_required = 411,

	/// The server-generated status code "412 Precondition Failed".
	precondition_failed = 412,

	/// The server-generated status code "413 Request Entity Too Large".
	request_entity_too_large = 413,

	/// The server-generated status code "414 Request URI Too Large".
	request_uri_too_large = 414,

	/// The server-generated status code "415 Unsupported Media Type".
	unsupported_media_type = 415,

	/// The server-generated status code "416 Requested Range Not Satisfiable".
	requested_range_not_satisfiable = 416,

	/// The server-generated status code "417 Expectation Failed".
	expectation_failed = 417,

	/// The server-generated status code "500 Internal Server Error".
	internal_server_error = 500,

	/// The server-generated status code "501 Not Implemented".
	not_implemented = 501,

	/// The server-generated status code "502 Bad Gateway".
	bad_gateway = 502,

	/// The server-generated status code "503 Service Unavailable".
	service_unavailable = 503,

	/// The server-generated status code "504 Gateway Timeout".
	gateway_timeout = 504,

	/// The server-generated status code "505 HTTP Version Not Supported".
	_version_not_supported = 505,

};

/// Converts a value of type @c errc_t to a corresponding object of type
/// @c boost::system::error_code.
/**
 * @par Requirements
 * @e Header: @c <error_codec.hpp> @n
 * @e Namespace: @c avhttp::errc
 */
inline boost::system::error_code make_error_code(errc_t e)
{
	return boost::system::error_code(static_cast<int>(e), avhttpd::error_category());
}

} // namespace errc
} // namespace avhttp

namespace boost {
namespace system {

template <>
struct is_error_code_enum<avhttpd::errc::errc_t>
{
  static const bool value = true;
};

} // namespace system
} // namespace boost

namespace avhttpd {
namespace detail {

class error_category_impl
  : public boost::system::error_category
{
	virtual const char* name() const BOOST_SYSTEM_NOEXCEPT
	{
		return "HTTP";
	}

	virtual std::string message(int e) const
	{
		switch (e)
		{
		case errc::malformed_request_line:
			return "Malformed request line";
		case errc::version_not_supported:
			return "version not supported";
		case errc::invalid_request_method:
			return "invalid request method";
		case errc::header_missing_host:
			return "header missing host";
		case errc::malformed_request_headers:
			return "Malformed request headers";
		case errc::post_without_content:
			return "HTTP POST without content length";
		case errc::header_too_large:
			return "header too large";
		case errc::invalid_chunked_encoding:
			return "Invalid chunked encoding";
		default:
			return "Unknown HTTP error";
		}
	}
};

} // namespace detail
} // namespace avhttpd

#endif // __AVHTTPD_ERROR_CODEC_HPP__
