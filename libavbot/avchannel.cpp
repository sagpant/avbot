
#include "avchannel.hpp"
#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

#include <boost/property_tree/json_parser.hpp>
#include <boost/locale/encoding_utf.hpp>

avchannel::avchannel(std::string name)
	: m_name(std::move(name))
{
}

bool avchannel::can_handle(channel_identifier channel_id) const
{
    // 看是否属于自己频道
    auto found = std::find(std::begin(m_rooms), std::end(m_rooms), channel_id);
    return found != std::end(m_rooms);
}

void avchannel::handle_message(channel_identifier channel_id, avbotmsg msg, send_avbot_message_t send_avbot_message, boost::asio::yield_context yield_context)
{
#ifdef HAVE_SYSTEMD
	// watchdog timer logic
	if (channel_id.protocol == "qq")
		sd_notify(0, "WATCHDOG=1");
#endif

	// 开始处理本频道消息
    for (auto room : m_rooms)
    {
        // 避免重复发送给自己
        if (room == channel_id)
            continue;

        send_avbot_message(room, msg, yield_context);
    }

    auto send_avchannel_message = [this, send_avbot_message](avbotmsg msg, boost::asio::yield_context yield_context)
	{
		broadcast_message(msg, send_avbot_message, yield_context);
	};

    handle_extra_message(channel_id, msg, send_avchannel_message, yield_context);
}

void avchannel::broadcast_message(avbotmsg msg, send_avbot_message_t send_avbot_message, boost::asio::yield_context yield_context)
{
    // 开始处理消息
    for (auto room : m_rooms)
    {
        send_avbot_message(room, msg, yield_context);
    }
}

boost::property_tree::ptree av_msg_make_json(channel_identifier ci, avbotmsg msg)
{
    boost::property_tree::ptree ptree;

	// 将 avbotmsg 格式弄成 json 格式

	ptree.put("protocol", ci.protocol);
	ptree.put("room", ci.room);

	ptree.put("message.text", msg.to_plain_text());

    // TODO
    return ptree;
}

avbotmsg from_json_string(std::string json_string)
{
	std::wstringstream json_wstring;
	json_wstring << boost::locale::conv::utf_to_utf<wchar_t>(json_string);

	avbotmsg ret;

	// 鉴于 boost 的 json 代码的严重的巨大的 bug , 只能使用 wide 版本才能处理好中文.

	boost::property_tree::wptree json;
	boost::property_tree::json_parser::read_json(json_wstring, json);

	// 开始格式化!

	



	return ret;
}
