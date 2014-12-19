#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/random.hpp>
#include <boost/function.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/regex.hpp>
#include <ctime>
#include <boost/foreach.hpp>

#include "staticcontent.hpp"

struct StaticContent
{
	StaticContent(boost::asio::io_service& io);

	void operator()(boost::system::error_code ec) {}

	void operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context);

	boost::asio::io_service& io_;

	typedef boost::regex Keywords;
	typedef std::vector<std::string> Messages;
	std::map<Keywords, Messages> static_contents_;

	boost::random::mt19937 g_;
	boost::uniform_int<> d_;

};

StaticContent::StaticContent(boost::asio::io_service& io)
	: io_(io)
	, d_(0, 10000)
{
	using namespace boost::property_tree::xml_parser;
	g_.seed(std::time(0));
	std::string filename = "static.xml";
	if(fs::exists(filename))
	{
		boost::property_tree::ptree pt;
		read_xml(filename, pt);
		BOOST_FOREACH(const auto & item,  pt.get_child("static"))
		{
			std::string keyword = item.second.get<std::string>("keyword");
			std::vector<std::string> messages;
			BOOST_FOREACH(const auto & message, item.second.get_child("messages"))
			{
				messages.push_back(message.second.get_value<std::string>());
			}
			static_contents_[boost::regex(keyword)] = messages;
		}
	}
}

void StaticContent::operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context)
{
	std::string text = msg.to_plain_text();

	BOOST_FOREACH(const auto & item,  static_contents_)
	{
		if(boost::regex_search(text, item.first))
		{
			sender(item.second[d_(g_) % item.second.size()], yield_context);
		}
	}
}

avbot_extension make_static_content(boost::asio::io_service& io, std::string channel_name)
{
	return avbot_extension(
		channel_name,
		StaticContent(io)
	);
}
