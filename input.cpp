
#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#if !defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
#include <boost/thread.hpp>
#endif
#include <boost/stringencodings.hpp>
#include "input.hpp"

static boost::signals2::signal< void(std::string)> input_got_one_line;

#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
static void inputread(const boost::system::error_code & ec, std::size_t length,
	std::shared_ptr<boost::asio::posix::stream_descriptor> stdin,
	std::shared_ptr<boost::asio::streambuf> inputbuffer)
{
	std::istream input(inputbuffer.get());
	std::string line;
	std::getline(input,line);

	input_got_one_line(line);

	boost::asio::async_read_until(
		*stdin, *inputbuffer, '\n',
		boost::bind(inputread , _1,_2, stdin, inputbuffer)
	);
}
#else
// workarround windows that can use posix stream for stdin
static void input_thread(boost::asio::io_service & io_service)
{
	while (!boost::this_thread::interruption_requested() && !std::cin.eof())
	{
		std::string line;
		std::getline(std::cin, line);
		io_service.post([line]{
			input_got_one_line(ansi_utf8(line));
		});
	}
}
#endif

void start_stdinput(boost::asio::io_service & io_service)
{
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
		std::shared_ptr<boost::asio::posix::stream_descriptor> stdin(
			new boost::asio::posix::stream_descriptor( io_service, 0 )
		);
		std::shared_ptr<boost::asio::streambuf> inputbuffer(
			new boost::asio::streambuf
		);
		boost::asio::async_read_until(
			*stdin, *inputbuffer, '\n',
			boost::bind(inputread , _1, _2, stdin, inputbuffer)
		);
#else
		boost::thread(
			boost::bind(input_thread, boost::ref(io_service))
		);
#endif
}

void connect_stdinput(boost::function<void(std::string)> cb)
{
	input_got_one_line.connect(cb);
}
