#pragma once

#include <string>
#include <memory>
#include <functional>
#include <boost/any.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>

struct avbotmsg_image_segment
{
	std::string cname;
	std::string image_data;
};

struct avbotmsg_emoji_segment
{
	std::string id;
	std::string image_data;
	std::string emoji_url;
};


struct avbotmsg_segment
{
	std::string type;

	boost::any content;

	avbotmsg_segment() = default;
	avbotmsg_segment(const avbotmsg_segment&) = default;

	avbotmsg_segment& operator =(const avbotmsg_segment&) = default;
#ifndef _MSC_VER
	avbotmsg_segment(avbotmsg_segment&&) = default;
	avbotmsg_segment& operator =(avbotmsg_segment&&) = default;
#endif
	template<class ANY>
	avbotmsg_segment(std::string _type, ANY any)
		: type(_type)
	{
		content = any;
	}
};

struct msg_sender
{
	std::string nick;
	std::string preamble;

	// additinal info
	std::string card_name; // name in group card
	std::string qq_number;

	bool is_op;
};

struct avbotmsg
{
	msg_sender sender;
	std::vector<avbotmsg_segment> msgs;

	avbotmsg() = default;
	avbotmsg(const avbotmsg&) = default;
	avbotmsg& operator =(const avbotmsg&) = default;
#ifndef _MSC_VER
	avbotmsg(avbotmsg&&) = default;
	avbotmsg& operator =(avbotmsg&&) = default;
#endif

	avbotmsg(std::string plain_text)
	{
		avbotmsg_segment seg;
		seg.type = "text";
		seg.content = plain_text;
		msgs.push_back(seg);
	}

	avbotmsg(const char* str)
		: avbotmsg(std::string(str))
	{
	}

	std::string to_plain_text() const
	{
		std::string ret;
		for (const avbotmsg_segment& s : msgs)
		{
			if (s.type == "text")
			{
				ret += boost::any_cast<std::string>(s.content);
			}
		}
		return ret;
	}
};

struct channel_identifier
{
	std::string protocol;
	std::string room;

	channel_identifier() = default;

	channel_identifier(const channel_identifier&) = default;
	channel_identifier& operator =(const channel_identifier&) = default;
#ifndef _MSC_VER
	channel_identifier(channel_identifier&&) = default;
	channel_identifier& operator =(channel_identifier&&) = default;
#endif

	channel_identifier(std::string p, std::string r)
		: protocol(p) , room(r) {}

	// addtionnal info for qq
	std::string room_name; // qq group number as room , so you can use this one to get group name
};

inline bool operator == (const channel_identifier& a, const channel_identifier& b)
{
	return a.protocol == b.protocol && a.room == b.room;
}

inline bool operator < (const channel_identifier& a, const channel_identifier& b)
{
	return a.protocol < b.protocol && a.room < b.room;
}

typedef std::function<void(channel_identifier, avbotmsg, boost::asio::yield_context)> send_avbot_message_t;
typedef std::function<void(avbotmsg, boost::asio::yield_context)> send_avchannel_message_t;

class avchannel // : std::enable_shared_from_this<avchannel>
{
public:
	typedef boost::signals2::signal<void(channel_identifier, avbotmsg, send_avchannel_message_t, boost::asio::yield_context)> handle_extra_message_type;

	avchannel(std::string);

	// 信号, 用于处理额外的消息!
	handle_extra_message_type handle_extra_message;

	// 每个频道会接收到所有的消息, 只是会过滤掉不是自己的消息
	bool can_handle(channel_identifier channel_id) const;

	void handle_message(channel_identifier channel_id, avbotmsg msg, send_avbot_message_t, boost::asio::yield_context);

	void broadcast_message(avbotmsg msg, send_avbot_message_t, boost::asio::yield_context);

	void add_room(std::string protocol, std::string room)
	{
		m_rooms.push_back(channel_identifier(protocol, room));
	}

	// get primary channel name,  aka, the QQ group number
	std::string get_primary() const
	{
		for (auto room : m_rooms)
		{
			if (room.protocol == "qq")
				return room.room;
		}

		// 没 QQ ? 用 irc 的吧...
		for (auto room : m_rooms)
		{
			if (room.protocol == "irc")
				return room.room.substr(1);
		}

		// 都没? 咋办? 用第一个的
		return m_rooms[0].room;
	}

	std::string name() const
	{
		return m_name;
	}

private:
	std::vector<channel_identifier> m_rooms;
	std::string m_name;
};

boost::property_tree::ptree av_msg_make_json(channel_identifier ci, avbotmsg msg);
