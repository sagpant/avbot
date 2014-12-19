
#pragma once

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>

#include "error_code.hpp"
#include "settings.hpp"

namespace avhttpd {
namespace detail {

template<class Stream, class Allocator, class Handler>
class async_read_request_op : boost::asio::coroutine
{
public:
	async_read_request_op(Stream & stream,
					boost::asio::basic_streambuf<Allocator> & streambuf,
					request_opts & opts, Handler handler)
		: m_stream(stream)
		, m_strembuf(streambuf)
		, m_opts(opts)
		, m_handler(handler)
	{
		boost::asio::async_read_until(m_stream, m_strembuf, std::string("\r\n"), *this);
	};

	void operator()(boost::system::error_code ec, std::size_t bytes_transferred)
	{
		std::string request_line;
		std::string one_header_line;
		boost::smatch what;

		BOOST_ASIO_CORO_REENTER(this)
		{
			if (ec)
			{
				return invoke_handler(ec);
			}

			// 完成 REQUEST LINE 的读取，用 boost::regex 处理
			request_line.resize(bytes_transferred);
			m_strembuf.sgetn(&request_line[0], bytes_transferred);
			request_line.resize(bytes_transferred-2);

			if (!boost::regex_match(request_line, what,
					boost::regex("([a-zA-Z]+)[ ]+([^ ]+)([ ]+(.*))?")))
			{
				return invoke_handler(
					errc::make_error_code(errc::malformed_request_line));
			};

			m_opts(avhttpd::http_options::request_method,
				boost::to_upper_copy(std::string(what[1])));

			m_opts(avhttpd::http_options::request_uri, what[2]);

			if (what[3].matched)
			{
				std::string http_version =
					boost::to_upper_copy(boost::trim_left_copy(std::string(what[3])));
				m_opts(http_options::http_version, http_version);
				if ( (http_version != "HTTP/1.1") && (http_version != "HTTP/1.0") )
				{	return invoke_handler(errc::make_error_code(
									errc::version_not_supported));
				}
			}else{
				m_opts(http_options::http_version, "HTTP/1.0");
			}

			// 读取 余下的.
			BOOST_ASIO_CORO_YIELD boost::asio::async_read_until(
						m_stream, m_strembuf, std::string("\r\n"), *this);


			while((bytes_transferred > 2) && !ec)
			{
				one_header_line.resize(bytes_transferred);
				m_strembuf.sgetn(&one_header_line[0], bytes_transferred);
				one_header_line.resize(bytes_transferred-2);

				if(!boost::regex_match(one_header_line, what,
									   boost::regex("^([^:]*): *(.*)$")))
				{
					return invoke_handler(errc::make_error_code(
								errc::malformed_request_headers));
				}
				m_opts(what[1], what[2]);
				BOOST_ASIO_CORO_YIELD boost::asio::async_read_until(
							m_stream, m_strembuf, std::string("\r\n"), *this);
			}

			if (bytes_transferred == 2){
				one_header_line.resize(bytes_transferred);
				m_strembuf.sgetn(&one_header_line[0], bytes_transferred);
				BOOST_ASSERT( one_header_line == "\r\n");
			}

			if (m_opts.find(http_options::http_version) == "HTTP/1.1"
				&& m_opts.find(http_options::host).empty())
			{
				return invoke_handler(errc::make_error_code(errc::header_missing_host));
			}

			if (m_opts.find(http_options::request_method) == "POST"
				&& m_opts.find(http_options::content_length).empty())
			{
				return invoke_handler(errc::make_error_code(errc::post_without_content));
			}

			invoke_handler(ec);
		}
	}
private:
	template<class EC>
	inline void invoke_handler(const EC &ec)
	{
		m_stream.get_io_service().post(
			boost::asio::detail::bind_handler(m_handler, ec)
		);
	}
private:
	// 传入的变量.
	Stream & m_stream;
	boost::asio::basic_streambuf<Allocator>& m_strembuf;
	request_opts & m_opts;
	Handler m_handler;

	// 这里是协程用到的变量.

};

template<class Stream, class Allocator, class Handler>
async_read_request_op<Stream, Allocator, Handler>
make_async_read_request_op(Stream & s,
						boost::asio::basic_streambuf<Allocator> & streambuf,
						request_opts & opts, Handler handler)
{
	return async_read_request_op<Stream, Allocator, Handler>(
				s, streambuf, opts, handler);
}

}

/*@}*/
/**
 * @defgroup async_read_request avhttpd::async_read_request
 *
 * @brief Start an asynchronous operation to read a http header from a stream.
 */
/*@{*/

/// Start an asynchronous operation to read a certain amount of data from a
/// stream.
/**
 * This function is used to asynchronously read a http header from a stream.
 * The function call always returns immediately. The asynchronous operation
 * will continue all http request header have been read or
 *
 * @li The header is too big to fit in the internal buffer.(likely not even
 *     an http request)
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. The
 * program must ensure that the stream performs no other read operations (such
 * as async_read, the stream's async_read_some function, or any other composed
 * operations that perform reads) until this operation completes. If streambuf
 * already has all http headers, then async_read_request will just consume the
 * parsed http header. If not,  async_read_request will perform more calls to
 * the stream's async_read_some function, and store the results into streambuf
 * Again, async_read_request will consume the HTTP header and only that header.
 *
 * If error occurres, then the data in which streambuf contain is undefined.
 * The stream is not speaking HTTP protocol, and there for the data can not be
 * trusted.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param opts The avhttpd::request_opts object whitch the header data is be be
 * filled into.
 *
 * @param streambuf The buffer stream that async_read_request will operat on.
 * async_read_request will delete an data coresponed to HTTP header. And leveaves
 * the remaining HTTP body in. The buffer stream can already hold some data.
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

template<class Stream, class Allocator,  class Handler>
	inline BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code))
async_read_request(Stream & s,
	boost::asio::basic_streambuf<Allocator> &streambuf,
		request_opts & opts, BOOST_ASIO_MOVE_ARG(Handler) handler)
{
	using namespace boost::asio;

	boost::asio::detail::async_result_init<Handler, void(boost::system::error_code)>
		init(BOOST_ASIO_MOVE_CAST(Handler)(handler));

	detail::make_async_read_request_op(s, streambuf, opts, init.handler);

	return init.result.get();
}

}
