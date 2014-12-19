
#pragma once

#include <utility>
#include <algorithm>
#include <boost/locale.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/make_shared.hpp>

#include <avhttp.hpp>

#include "extension.hpp"

class urlpreview
{
	typedef std::pair<std::string, boost::posix_time::ptime> urllist_item_type;
	typedef boost::circular_buffer_space_optimized<urllist_item_type> urllist_type;
	std::shared_ptr<urllist_type>	urllist;
	boost::asio::io_service &io_service;
	boost::function<void ( std::string ) > m_sender;

public:
	template<class MsgSender>
	urlpreview( boost::asio::io_service &_io_service,  MsgSender sender)
		: m_sender(sender)
		, io_service(_io_service)
		, urllist(std::make_shared<boost::circular_buffer_space_optimized<std::pair<std::string, boost::posix_time::ptime> > >(20))
	{
	}
	// on_message 回调.
	void operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context);
private:
	bool can_preview(std::string speaker, std::string url);
	void do_urlpreview(std::string speaker, std::string url, boost::posix_time::ptime current);
};
