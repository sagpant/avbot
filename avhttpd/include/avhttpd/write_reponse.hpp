
#pragma once

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>

#include "error_code.hpp"
#include "settings.hpp"

namespace avhttpd {
namespace detail {

inline const char * strstatus(int status)
{
	switch (status)
	{
	case errc::ok:
		return "OK";
	case errc::created:
		return "Created";
	case errc::accepted:
		return "Accepted";
	case errc::non_authoritative_information:
		return "Non-authoritative information";
	case errc::no_content:
		return "No content";
	case errc::reset_content:
		return "Reset content";
	case errc::partial_content:
		return "Partial content";
	case errc::multiple_choices:
		return "Multiple choices";
	case errc::moved_permanently:
		return "Moved permanently";
	case errc::found:
		return "Found";
	case errc::see_other:
		return "See other";
	case errc::not_modified:
		return "Not modified";
	case errc::use_proxy:
		return "Use proxy";
	case errc::temporary_redirect:
		return "Temporary redirect";
	case errc::bad_request:
		return "Bad request";
	case errc::unauthorized:
		return "Unauthorized";
	case errc::payment_required:
		return "Payment required";
	case errc::forbidden:
		return "Forbidden";
	case errc::not_found:
		return "Not found";
	case errc::method_not_allowed:
		return "Method not allowed";
	case errc::not_acceptable:
		return "Not acceptable";
	case errc::proxy_authentication_required:
		return "Proxy authentication required";
	case errc::request_timeout:
		return "Request time-out";
	case errc::conflict:
		return "Conflict";
	case errc::gone:
		return "Gone";
	case errc::length_required:
		return "Length required";
	case errc::precondition_failed:
		return "Precondition failed";
	case errc::request_entity_too_large:
		return "Request entity too large";
	case errc::request_uri_too_large:
		return "Request URI too large";
	case errc::unsupported_media_type:
		return "Unsupported media type";
	case errc::requested_range_not_satisfiable:
		return "Requested range not satisfiable";
	case errc::expectation_failed:
		return "Expectation failed";
	case errc::internal_server_error:
		return "Internal server error";
	case errc::not_implemented:
		return "Not implemented";
	case errc::bad_gateway:
		return "Bad gateway";
	case errc::service_unavailable:
		return "Service unavailable";
	case errc::gateway_timeout:
		return "Gateway time-out";
	case errc::version_not_supported:
		return "HTTP version not supported";
	}
	return "bad code";
}

template<class Stream, class Handler>
class async_write_response_op
{
public:
	async_write_response_op(Stream & stream, int status, const response_opts & _opts,
							Handler handler)
		: m_stream(stream), m_handler(handler)
	{
		response_opts opts = _opts;
		m_headers = boost::make_shared<boost::asio::streambuf>();
		std::ostream out(m_headers.get());

		if (opts.find(avhttpd::http_options::http_version)=="HTTP/1.1")
			out << "HTTP/1.1 ";
		else out <<  "HTTP ";

		opts.remove(avhttpd::http_options::http_version);

		out <<  status <<  " " << strstatus(status)  << "\r\n";

		if (opts.size())
			out  <<  opts.header_string();
		out <<  "\r\n";

		boost::asio::async_write(m_stream, *m_headers, *this);
	};

	void operator()(boost::system::error_code ec, size_t bytes_transfered)
	{
		m_handler(ec, bytes_transfered);
	}

private:
	// 传入的变量.
	Stream & m_stream;
	Handler m_handler;

	// 这里是协程用到的变量.
	boost::shared_ptr<boost::asio::streambuf> m_headers;
};

template<class Stream, class Handler>
async_write_response_op<Stream, Handler>
make_async_write_response_op(Stream & s, int status, const response_opts & opts, Handler handler)
{
	return async_write_response_op<Stream, Handler>(s, status, opts, handler);
}


template<class Stream, class ConstBufferSequence, class Handler>
class async_write_response_with_body_op
{

public:
	async_write_response_with_body_op(Stream & stream, int status,
					const response_opts & opts,
					const ConstBufferSequence & buffers,
					Handler handler)
		: m_stream(stream)
		, m_buffers(buffers)
		, m_handler(handler)
	{
		make_async_write_response_op(m_stream, status, opts, *this);
	};

	void operator()(boost::system::error_code ec)
	{
		boost::asio::async_write(m_stream, m_buffers, *this);
	}

	void operator()(boost::system::error_code ec, size_t bytes_transfered)
	{
		m_handler(ec, bytes_transfered);
	}
private:
	// 传入的变量.
	Stream & m_stream;
	Handler m_handler;

	ConstBufferSequence m_buffers;

	// 这里是协程用到的变量.
	boost::shared_ptr<boost::asio::streambuf> m_headers;
};

template<class Stream, class Allocator, class Handler>
class async_write_response_with_streambuf_op
{
public:
	async_write_response_with_streambuf_op(Stream & stream, int status,
					const response_opts & opts,
					const boost::asio::basic_streambuf<Allocator> & streambuf,
					Handler handler)
		: m_stream(stream)
		, m_streambuf(streambuf)
		, m_handler(handler)
		, coro(false)
	{
		make_async_write_response_op(m_stream, status, opts, *this);
	};

	void operator()(boost::system::error_code ec, size_t bytes_transfered)
	{
		if ((coro=(!coro)))
		{
			boost::asio::async_write(m_stream, m_streambuf.data(), *this);
		}
		else
		{
			m_handler(ec, bytes_transfered);
		}
	}
private:
	bool coro;
	// 传入的变量.
	Stream & m_stream;
	Handler m_handler;

	const boost::asio::basic_streambuf<Allocator> & m_streambuf;
	// 这里是协程用到的变量.
	boost::shared_ptr<boost::asio::streambuf> m_headers;
};

template<class Stream, class ConstBufferSequence, class Handler>
async_write_response_with_body_op<Stream, ConstBufferSequence, Handler>
make_async_write_response_op(Stream & s, int status, const response_opts & opts,
							const ConstBufferSequence& buffers, Handler handler)
{
	return async_write_response_with_body_op<Stream, ConstBufferSequence, Handler>
				(s, status, opts, buffers, handler);
}

template<class Stream, class Allocator, class Handler>
async_write_response_with_streambuf_op<Stream, Allocator, Handler>
make_async_write_response_op(Stream & s, int status, const response_opts & opts,
						const boost::asio::basic_streambuf<Allocator> & streambuf, Handler handler)
{
	return async_write_response_with_streambuf_op<Stream, Allocator, Handler>
				(s, status, opts, streambuf, handler);
}


}

/*@}*/
/**
 * @defgroup async_write_response avhttpd::async_write_response
 *
 * @brief Start an asynchronous operation to write a http header to a stream.
 */
/*@{*/

/// Start an asynchronous operation to write a http header to a stream
/**
 * This function is used to asynchronously write a http header to a stream.
 * The function call always returns immediately. The asynchronous operation
 * will continue all http request header have been write or
 *
 * @li The header is too big to fit in the internal buffer.(likely not even
 *     an http response)
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_write_some function, and is known as a <em>composed operation</em>. The
 * program must ensure that the stream performs no other read operations (such
 * as async_read, the stream's async_write_some function, or any other composed
 * operations that perform reads) until this operation completes.
 *
 * If error occurres, then the data in which streambuf contain is undefined.
 * The stream is not speaking HTTP protocol, and there for the data can not be
 * trusted.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param status the HTTP status code
 *
 * @param opts The avhttpd::response_opts object whitch the header data is be be
 * filled into.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   const boost::system::error_code& error, // Result of operation.
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. Invocation of
 * the handler will be performed in a manner equivalent to using
 * boost::asio::io_service::post().
 *
 */

template<class Stream, class Handler>
inline BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code, size_t))
async_write_response(Stream & s, int status, BOOST_ASIO_MOVE_ARG(Handler) handler)
{
	response_opts opts;
	using namespace boost::asio;

	boost::asio::detail::async_result_init<Handler, void(boost::system::error_code, size_t)>
		init(BOOST_ASIO_MOVE_CAST(Handler)(handler));


	detail::async_write_response_op<
		Stream, BOOST_ASIO_HANDLER_TYPE(Handler, void(boost::system::error_code, size_t))
	>(s, status, opts, init.handler);

	return init.result.get();
}


/*@}*/
/**
 * @defgroup async_write_response avhttpd::async_write_response
 *
 * @brief Start an asynchronous operation to write a http header to a stream.
 */
/*@{*/

/// Start an asynchronous operation to write a http header to a stream
/**
 * This function is used to asynchronously write a http header to a stream.
 * The function call always returns immediately. The asynchronous operation
 * will continue all http request header have been write or
 *
 * @li The header is too big to fit in the internal buffer.(likely not even
 *     an http response)
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_write_some function, and is known as a <em>composed operation</em>. The
 * program must ensure that the stream performs no other read operations (such
 * as async_read, the stream's async_write_some function, or any other composed
 * operations that perform reads) until this operation completes.
 *
 * If error occurres, then the data in which streambuf contain is undefined.
 * The stream is not speaking HTTP protocol, and there for the data can not be
 * trusted.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param status the HTTP status code
 *
 * @param opts The avhttpd::response_opts object whitch the header data is be be
 * filled into.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   const boost::system::error_code& error, // Result of operation.
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. Invocation of
 * the handler will be performed in a manner equivalent to using
 * boost::asio::io_service::post().
 *
 */

template<class Stream, class Handler>
inline BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code, size_t))
async_write_response(Stream & s, int status,
	const response_opts & opts, BOOST_ASIO_MOVE_ARG(Handler) handler)
{
	using namespace boost::asio;

	boost::asio::detail::async_result_init<Handler, void(boost::system::error_code, size_t)>
		init(BOOST_ASIO_MOVE_CAST(Handler)(handler));

	detail::make_async_write_response_op(s, status, opts, init.handler);

	return init.result.get();
}

/*@}*/
/**
 * @defgroup async_write_response avhttpd::async_write_response
 *
 * @brief Start an asynchronous operation to write a http response to a stream.
 */
/*@{*/

/// Start an asynchronous operation to write a http response to a stream
/**
 * This function is used to asynchronously write a http header and the body
 * to a stream. The function call always returns immediately. The asynchronous
 * operation will continue all http request header have been write or
 *
 * @li The header is too big to fit in the internal buffer.(likely not even
 *     an http response)
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_write_some function, and is known as a <em>composed operation</em>. The
 * program must ensure that the stream performs no other read operations (such
 * as async_write, the stream's async_write_some function, or any other composed
 * operations that perform writes) until this operation completes.
 *
 * If error occurres, then the data in which streambuf contain is undefined.
 * The stream is not speaking HTTP protocol, and there for the data can not be
 * trusted.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param status the HTTP status code
 *
 * @param opts The avhttpd::response_opts object that caray http header
 *
 * @param buffers The buffers that contains the response body
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   const boost::system::error_code& error, // Result of operation.
 *   size_t bytes_transferred, // The bytes that have written to the stream
 *                             // as http body. Does not contain header length
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. Invocation of
 * the handler will be performed in a manner equivalent to using
 * boost::asio::io_service::post().
 *
 */

template<class Stream, class ConstBufferSequence, class Handler>
inline BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code, size_t))
async_write_response(Stream & s, int status, const response_opts & opts,
	const ConstBufferSequence& buffers, BOOST_ASIO_MOVE_ARG(Handler) handler)
{
	using namespace boost::asio;

	boost::asio::detail::async_result_init<Handler, void(boost::system::error_code, size_t)>
		init(BOOST_ASIO_MOVE_CAST(Handler)(handler));

	detail::make_async_write_response_op(s, status, opts, buffers, init.handler);

	return init.result.get();
}

template<class Stream, class Allocator, class Handler>
inline BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code, size_t))
async_write_response(Stream & s, int status, const response_opts & opts,
	const boost::asio::basic_streambuf<Allocator> & streambuf, BOOST_ASIO_MOVE_ARG(Handler) handler)
{

	using namespace boost::asio;

	boost::asio::detail::async_result_init<Handler, void(boost::system::error_code, size_t)>
		init(BOOST_ASIO_MOVE_CAST(Handler)(handler));

	detail::make_async_write_response_op(s, status, opts, streambuf, init.handler);

	return init.result.get();
}

}
