#include <algorithm>
#include <boost/range/algorithm/remove_if.hpp>
#include <boost/regex.hpp>
#include <boost/throw_exception.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/scope_exit.hpp>
#include <fstream>

#include "boost/urlencode.hpp"
#include "boost/stringencodings.hpp"
#include "avbot.hpp"

#ifndef NO_WEBQQ
static std::string	preamble_formater(std::string preamble_qq_fmt, webqq::qqBuddy *buddy, std::string falbacknick, webqq::qqGroup * grpup = NULL )
{
	static webqq::qqBuddy _buddy("", "", "", 0, "");
	std::string preamble;
	// 格式化神器, 哦耶.
	// 获取格式化描述字符串.
	std::string preamblefmt = preamble_qq_fmt;
	// 支持的格式化类型有 %u UID,  %q QQ号, %n 昵称,  %c 群名片 %a 自动.

	preamble = preamblefmt;
	std::string autonick = "";

	if( !buddy ) {
		autonick = falbacknick;
		buddy = & _buddy;
	} else {
		autonick = buddy->card;

		if( autonick.empty() ) {
			autonick = buddy->nick;
		}

		if( autonick.empty() ) {
			autonick = buddy->qqnum;
		}

		if( autonick.empty() ) {
			autonick = buddy->uin;
		}
	}

	boost::replace_all( preamble, "%a", autonick );
	boost::replace_all( preamble, "%n", buddy->nick );
	boost::replace_all( preamble, "%u", buddy->uin );
	boost::replace_all( preamble, "%q", buddy->qqnum );
	boost::replace_all( preamble, "%c", buddy->card );

	if( grpup ){
		boost::replace_all( preamble, "%r", grpup->qqnum );
		boost::replace_all( preamble, "%R", grpup->name );
	}
	return preamble;
}
#endif

static std::string	preamble_formater(std::string preamble_irc_fmt, irc::irc_msg pmsg )
{
	// 格式化神器, 哦耶.
	// 获取格式化描述字符串.
	std::string preamble = preamble_irc_fmt;

	// 支持的格式化类型有 %u UID,  %q QQ号, %n 昵称,  %c 群名片 %a 自动 %r irc 房间.
	// 默认为 qq(%a) 说:.
	boost::replace_all( preamble, "%a", pmsg.whom );
	boost::replace_all( preamble, "%r", pmsg.from );
	boost::replace_all( preamble, "%n", pmsg.whom );
	return preamble;
}

static std::string	preamble_formater(std::string preamble_xmpp_fmt,  std::string who, std::string room )
{
	// 格式化神器, 哦耶.
	// 获取格式化描述字符串.
	std::string preamble = preamble_xmpp_fmt;
	// 支持的格式化类型有 %u UID,  %q QQ号, %n 昵称,  %c 群名片 %a 自动 %r irc 房间.

	boost::replace_all( preamble, "%a", who );
	boost::replace_all( preamble, "%r", room );
	boost::replace_all( preamble, "%n", who );
	return preamble;
}

avbot::avbot(boost::asio::io_service& io_service, boost::logger& _logger)
	: m_io_service(io_service)
	, logger(_logger)
	, m_quit(std::make_shared<std::atomic<bool>>(false))
{
	preamble_irc_fmt = "%a 说：";
	preamble_qq_fmt = "qq(%a)说：";
	preamble_xmpp_fmt = "(%a)说：";

 	on_message.connect(std::bind(&avbot::forward_message, this, std::placeholders::_1, std::placeholders::_2));
}

avbot::~avbot()
{
	*m_quit = true;
	// then all coroutine will go die
}

boost::asio::io_service& avbot::get_io_service()
{
    return m_io_service;
}

void avbot::add_channel(std::string name, std::shared_ptr< avchannel > c)
{
    m_avchannels.insert(std::make_pair(name, c));
}

std::shared_ptr<avchannel> avbot::get_channel(channel_identifier cid)
{
	for (auto c_p : m_avchannels)
	{
		if(c_p.second->can_handle(cid))
			return c_p.second;
	}
	return std::shared_ptr<avchannel>();
}

void avbot::callback_on_irc_message(std::weak_ptr<irc::client>, irc::irc_msg pMsg )
{
	channel_identifier channdl_id;
	avbotmsg message;

	// formate irc message to JSON and call on_message

	channdl_id.protocol = "irc";
	channdl_id.room = pMsg.from;

	message.sender.nick = pMsg.whom;

	message.sender.preamble = preamble_formater(preamble_irc_fmt, pMsg);

	// TODO 将 识别的 URL 进行转化.

	avbotmsg_segment text_msg;
	text_msg.type = "text";
	text_msg.content = pMsg.msg;

	message.msgs.push_back(text_msg);

	on_message(channdl_id, message);
}

#ifndef NO_WEBQQ
void avbot::callback_on_qq_group_message(std::weak_ptr< webqq::webqq > qq_account, std::string group_code, std::string who, std::vector< webqq::qqMsg > msg, boost::asio::yield_context yield_context)
{
	channel_identifier channel_id;
	avbotmsg message;

	channel_id.protocol = "qq";
	channel_id.room = group_code;

	webqq::qqGroup_ptr group = qq_account.lock()->get_Group_by_gid( group_code );
	std::string groupname = group_code;

	if( group ){
		groupname = group->name;
		      channel_id.room  = group->qqnum;
		      channel_id.room_name = group->name;
	}

	message.sender.nick = who;

	webqq::qqBuddy_ptr buddy;

	if (group)
		buddy = group->get_Buddy_by_uin(who);

	if (buddy){
		message.sender.nick = buddy->nick.empty()? buddy->uin : buddy->nick ;
		message.sender.card_name = buddy->card;
		message.sender.qq_number = buddy->qqnum;

		if( ( buddy->mflag & 1 ) == 1 || buddy->uin == group->owner )
			message.sender.is_op = true;
		else
			message.sender.is_op = false;
	}

	message.sender.preamble = preamble_formater(preamble_qq_fmt, buddy.get(), who, group.get());

	// 解析 qqMsg
	for (webqq::qqMsg qqmsg : msg)
	{
		std::string buf;

		switch(qqmsg.type)
		{
			case webqq::qqMsg::LWQQ_MSG_FONT:
			break;
			case webqq::qqMsg::LWQQ_MSG_TEXT:
			{
				avbotmsg_segment text;
				text.type = "text";
				text.content = qqmsg.text;
				message.msgs.push_back(text);
			}
			break;
			case webqq::qqMsg::LWQQ_MSG_CFACE:
			{
				// save to disk
				// 先检查这样的图片本地有没有，有你fetch的P啊.

				std::string img_data;

				if (m_image_cacher)
					img_data = m_image_cacher(qqmsg.cface.name);

				// the image has to be feteched
				if (img_data.empty())
				{
					boost::system::error_code ec;
					boost::asio::streambuf buf;

 					webqq::webqq::async_fetch_cface(m_io_service, qqmsg.cface, buf, yield_context[ec]);

					if (!ec)
					{
						// 把 buf 的数据写入 img_data
						img_data.resize(buf.size());
						buf.sgetn(&img_data[0], buf.size());

						// 写入图片
						if (m_image_saver)
							m_image_saver(qqmsg.cface.name, img_data);
					}
				}

				avbotmsg_image_segment img_content;

				img_content.cname = qqmsg.cface.name;
				img_content.image_data = img_data;

				avbotmsg_segment msg_seg;
				msg_seg.type = "image";
				msg_seg.content = img_content;

				message.msgs.push_back(msg_seg);
			}
			break;
			case webqq::qqMsg::LWQQ_MSG_FACE:
			{

				avbotmsg_emoji_segment emoji_content;

				emoji_content.id = std::to_string(qqmsg.face);
				emoji_content.emoji_url  = boost::str( boost::format("http://0.web.qstatic.com/webqqpic/style/face/%d.gif" ) % qqmsg.face );

				avbotmsg_segment msg_seg;
				msg_seg.type = "emoji";
				msg_seg.content = emoji_content;
				message.msgs.push_back(msg_seg);
			} break;
		}
	}

	//

	on_message(channel_id, message);
}
#endif

void avbot::callback_on_xmpp_group_message(std::weak_ptr< xmpp >, std::string xmpproom, std::string who, std::string msg )
{
	channel_identifier channel_id;
	avbotmsg message;

	channel_id.protocol = "xmpp";
	channel_id.room = xmpproom;

	message.sender.nick = who;
	message.sender.preamble = preamble_formater( preamble_xmpp_fmt, who, xmpproom);

	avbotmsg_segment msg_seg;
	msg_seg.type = "text";
	msg_seg.content = msg;

	message.msgs.push_back(msg_seg);
	on_message(channel_id, message);
}

void avbot::callback_on_mail(mailcontent mail, mx::pop3::call_to_continue_function call_to_contiune )
{
	channel_identifier channel_id;
	avbotmsg message;
	channel_id.protocol = "mail";
	channel_id.room = mail.to;

	message.sender.nick = mail.from;

	avbotmsg_segment msg_seg;
	msg_seg.type = "text";
	msg_seg.content = mail.content;

	message.msgs.push_back(msg_seg);
	on_message(channel_id, message);

 	m_io_service.post(boost::asio::detail::bind_handler(call_to_contiune, 1));
}

void avbot::callback_on_avim(std::string reciver, std::string sender, std::vector<avim_msg> msg)
{
	channel_identifier channel_id;
	avbotmsg message;
	channel_id.protocol = "avim";
	channel_id.room = reciver;

	message.sender.nick = sender;

	for(avim_msg m : msg)
	{
		if (!m.text.empty())
		{
			avbotmsg_segment msg_seg;
			msg_seg.type = "text";
			msg_seg.content = m.text;
			message.msgs.push_back(msg_seg);
		}
		else if (!m.image.empty())
		{
			avbotmsg_image_segment img_seg;

			img_seg.image_data = m.image;

			avbotmsg_segment msg_seg;
			msg_seg.type = "image";
			msg_seg.content = img_seg;
			message.msgs.push_back(msg_seg);
		}
	}
	on_message(channel_id, message);
}

void avbot::callback_on_std_account(std::weak_ptr<avaccount> std_account, channel_identifier i, const avbotmsg& m)
{
	on_message(i,m);
}

#ifndef NO_WEBQQ
void avbot::callback_on_qq_group_found(std::weak_ptr<webqq::webqq> qq_account, webqq::qqGroup_ptr group)
{
	channel_identifier channel_id;
	channel_id.protocol = "qq";
	channel_id.room = group->qqnum;
	auto r = qq_account.lock();
	if (!r)
		return;

	m_account_mapping.push_back(std::make_pair(channel_id, accounts_t(r)));
}

void avbot::callback_on_qq_group_newbee(std::weak_ptr<webqq::webqq>, webqq::qqGroup_ptr group, webqq::qqBuddy_ptr buddy)
{
	channel_identifier channel_id;
	avbotmsg message;

	// 新人入群咯.

	channel_id.protocol = "qq";
	channel_id.room = group->qqnum;

	message.sender.preamble = "system message";
	message.sender.nick = "name unknow";

	// 构造 json 消息,  格式同 QQ 消息, 就是多了个 newbee 字段
	if (buddy){
		message.sender.nick = buddy->nick;
	}else{
		// 新人入群,  可是 webqq 暂时无法获取新人昵称.
		return;
	}

	if (buddy){
		message.msgs.push_back(avbotmsg_segment("text", "新人入群"));
	}

	on_message(channel_id, message);
}
#endif

void avbot::callback_on_xmpp_room_joined(std::weak_ptr<xmpp> xmpp_account, std::string roomname)
{
	channel_identifier channel_id;
	channel_id.protocol = "xmpp";
	channel_id.room = roomname;

	auto r = xmpp_account.lock();
	if (!r)
		return;
	m_account_mapping.push_back(std::make_pair(channel_id, r));
}

void avbot::callback_on_irc_room_joined(std::weak_ptr<irc::client> irc_account, std::string roomname)
{
	channel_identifier channel_id("irc", roomname);

	auto r = irc_account.lock();
	if (!r)
		return;
	m_account_mapping.push_back(std::make_pair(channel_id, accounts_t(r)));
}

void avbot::callback_on_avim_room_created(std::weak_ptr<avim> avim_account, std::string n)
{
	channel_identifier channel_id("avim", n);

	auto r = avim_account.lock();
	if (!r)
		return;
	m_account_mapping.push_back(std::make_pair(channel_id, accounts_t(r)));
}

std::shared_ptr<webqq::webqq> avbot::add_qq_account(std::string qqnumber, std::string password, avbot::need_verify_image cb, bool no_persistent_db)
{
#ifndef NO_WEBQQ
	auto qq_account = std::make_shared<webqq::webqq>(std::ref(get_io_service()), std::ref(logger), qqnumber, password, no_persistent_db);
	qq_account->on_verify_code([this, qq_account, cb](std::string img)
	{
		m_I_need_vc = qq_account;
		cb(img);
	});

	std::weak_ptr<webqq::webqq> account = qq_account;

	qq_account->on_group_found(std::bind(&avbot::callback_on_qq_group_found, this, account, std::placeholders::_1));
	qq_account->on_group_newbee(std::bind(&avbot::callback_on_qq_group_newbee, this, account, std::placeholders::_1, std::placeholders::_2));

	qq_account->on_group_msg([this, account](std::string group_code, std::string who, const std::vector<webqq::qqMsg>& msg)
	{
		boost::asio::spawn(m_io_service, std::bind(&avbot::callback_on_qq_group_message, this, account.lock(), group_code, who, msg, std::placeholders::_1));
	});

	m_qq_accounts.push_back(qq_account);
	return qq_account;
#else
	return std::shared_ptr<webqq::webqq>();
#endif

}

void avbot::feed_login_verify_code(std::string vcode, std::function<void()> badvcreporter)
{
#ifndef NO_WEBQQ
	// 检查需要验证码的 account
	try{
		auto qq_account = boost::any_cast<std::shared_ptr<webqq::webqq>>(m_I_need_vc);
		if (!qq_account->is_online())
			qq_account->feed_vc(vcode, badvcreporter);
	}catch(const boost::bad_any_cast&)
	{}
#endif
}

std::shared_ptr<irc::client> avbot::add_irc_account( std::string nick, std::string password, std::string server, bool use_ssl)
{
	if (use_ssl){
		boost::throw_exception(std::invalid_argument("ssl is currently not supported"));
	}

	auto irc_account = std::make_shared<irc::client>(std::ref(m_io_service), std::ref(logger), nick, password, server);

	std::weak_ptr<irc::client> account = irc_account;

	irc_account->on_privmsg_message(std::bind(&avbot::callback_on_irc_message, this, account, std::placeholders::_1));
	irc_account->on_new_room(std::bind(&avbot::callback_on_irc_room_joined, this, account, std::placeholders::_1));

	m_irc_accounts.push_back(irc_account);
	return irc_account;
}

std::shared_ptr<xmpp> avbot::add_xmpp_account( std::string user, std::string password, std::string nick, std::string server )
{
	auto xmpp_account = std::make_shared<xmpp>(std::ref(m_io_service), user, password, server, nick);

	std::weak_ptr<xmpp> account = xmpp_account;

	xmpp_account->on_room_message(std::bind(&avbot::callback_on_xmpp_group_message, this,account, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	xmpp_account->on_room_joined(std::bind(&avbot::callback_on_xmpp_room_joined, this, account, std::placeholders::_1));
	m_xmpp_accounts.push_back(xmpp_account);
	return xmpp_account;
}

void avbot::set_mail_account( std::string mailaddr, std::string password, std::string pop3server, std::string smtpserver )
{
	// 开启 pop3 收邮件.
	m_mail_account.reset(new mx::mx(m_io_service, mailaddr, password, pop3server, smtpserver));

	m_mail_account->async_fetch_mail(boost::bind(&avbot::callback_on_mail, this, _1, _2));
}

std::shared_ptr<avim> avbot::add_avim_account(std::string key, std::string cert, std::string groupdeffile)
{
	auto avim_account = std::make_shared<avim>(std::ref(m_io_service), key, cert, groupdeffile);
	std::weak_ptr<avim> account = avim_account;

	avim_account->on_message(std::bind(&avbot::callback_on_avim, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	avim_account->on_group_created(std::bind(&avbot::callback_on_avim_room_created, this, account, std::placeholders::_1));

	m_avim_accounts.push_back(avim_account);
	return avim_account;
}

void avbot::add_std_account(std::shared_ptr<avaccount> std_account)
{
	m_std_accounts.push_back(std_account);

	std::weak_ptr<avaccount> weak_account = std_account;

	std_account->on_message.connect(std::bind(&avbot::callback_on_std_account, this, weak_account, std::placeholders::_1, std::placeholders::_2));
}

void avbot::forward_message( const channel_identifier& i, const avbotmsg& m)
{
	send_avbot_message_t message_sender = std::bind(&avbot::send_avbot_message, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

	bool processed = false;
	for (auto& pair : m_avchannels)
	{
		std::shared_ptr<avchannel> & c = pair.second;
		if (c->can_handle(i))
		{
			processed = true;
			boost::asio::spawn(get_io_service(),
				std::bind(&avchannel::handle_message, c, i, m, message_sender, std::placeholders::_1));
		}
	}

	if (!processed)
	{
		// 居然有消息没有被任何频道接收??? ???
		// TODO 想点办法
	}
}

template<typename Handler>
struct send_avbot_message_visitor : public boost::static_visitor<>
{
	template<class T>
	void operator()(T &) const
	{
		boost::system::error_code ec;
		_yield_context(ec);
	}
#ifndef NO_WEBQQ
	void operator()(std::shared_ptr<webqq::webqq>& qq) const
	{
		// 使用 qq 的模式发送消息
		std::string text_msg = _bot.format_message_for_qq(_msg);
		webqq::qqGroup_ptr group = qq->get_Group_by_qq(_id.room);
		if (group)
		{
			qq->send_group_message(*group, text_msg, _yield_context);
		}
		else
		{
			boost::system::error_code ec;
			_bot.get_io_service().post(std::bind(_yield_context, ec));
		}
	}
#endif

	void operator()(std::shared_ptr<irc::client>& irc_account) const
	{
		// 使用 qq 的模式发送消息
		std::string text_msg = _bot.format_message_for_textIM(_msg);

		irc_account->chat(_id.room, text_msg);

		boost::system::error_code ec;
		_bot.get_io_service().post(std::bind(_yield_context, ec));
	}

	void operator()(std::shared_ptr<xmpp>& xmpp_account) const
	{
		// 使用 qq 的模式发送消息
		std::string text_msg = _bot.format_message_for_textIM(_msg);

		xmpp_account->send_room_message(_id.room, text_msg);
		boost::system::error_code ec;
		_bot.get_io_service().post(std::bind(_yield_context, ec));
	}

	void operator()(std::shared_ptr<avim>& avim_account) const
	{
		std::vector<avim_msg> avmsg;
		{
			avim_msg text_msg;
			text_msg.text = _msg.sender.preamble;
			avmsg.push_back(text_msg);
		}

		for (const avbotmsg_segment& seg : _msg.msgs)
		{
			if (seg.type == "text")
			{
				avim_msg text_msg;
				text_msg.text = boost::any_cast<std::string>(seg.content);
				avmsg.push_back(text_msg);
			}
			else if(seg.type == "image")
			{
				avim_msg img_msg;

				avbotmsg_image_segment img_seg = boost::any_cast<avbotmsg_image_segment>(seg.content);
 				img_msg.image = img_seg.image_data;
				img_msg.image_cname = img_seg.cname;
				avmsg.push_back(img_msg);
			}
		}

		avim_account->send_group_message(_msg.sender.nick, avmsg);

		boost::system::error_code ec;
		_bot.get_io_service().post(std::bind(_yield_context, ec));
	}

	send_avbot_message_visitor(avbot& bot, channel_identifier& id, avbotmsg& msg, Handler& yield_context)
		: _bot(bot), _id(id), _msg(msg), _yield_context(yield_context)
	{}

	avbot& _bot;
	channel_identifier& _id;
	avbotmsg& _msg;
	Handler& _yield_context;
};

void avbot::send_avbot_message(channel_identifier id, avbotmsg msg, boost::asio::yield_context yield_context)
{
	using namespace boost::asio;

	boost::asio::detail::async_result_init<boost::asio::yield_context, void(boost::system::error_code)>
		init((boost::asio::yield_context&&)yield_context);

	auto comp = [&id](const std::pair<channel_identifier, accounts_t>& a) -> bool
	{
		return a.first == id;
	};

	auto it = std::find_if(m_account_mapping.begin(), m_account_mapping.end(), comp);
	if ( it != m_account_mapping.end())
	{
		send_avbot_message_visitor<BOOST_ASIO_HANDLER_TYPE(boost::asio::yield_context, void(boost::system::error_code))>
			visitor(*this, id, msg, init.handler);
		boost::apply_visitor(visitor, it->second);
	}
	else
	{
		boost::system::error_code ec;
		m_io_service.post(std::bind(init.handler, ec));
	}
	return init.result.get();
}

void avbot::send_broadcast_message(std::string channel_name, avbotmsg msg)
{
	send_avbot_message_t message_sender = std::bind(&avbot::send_avbot_message, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

	auto channel_iter = m_avchannels.find(channel_name);
	if (channel_iter!= m_avchannels.end())
	{
		boost::asio::spawn(get_io_service(), [channel_iter, msg, message_sender](boost::asio::yield_context yield_context)
		{
			channel_iter->second->broadcast_message(msg, message_sender, yield_context);
		});
	}
}

void avbot::broadcast_message_to_all_channels(avbotmsg msg)
{
	for (auto c_p : m_avchannels)
	{
		send_broadcast_message(c_p.first, msg);
	}
}

void avbot::broadcast_textmessage_to_all_channels(std::string text)
{
	broadcast_message_to_all_channels(avbotmsg(text));
}

std::string avbot::format_message_for_qq(const avbotmsg& message )
{
	std::string linermessage;
	// 这个消息是要格式化后给 qq 发送的, 因此 ...

	linermessage += message.sender.preamble;

	linermessage += " ";

	for (const avbotmsg_segment& seg : message.msgs)
	{
		if (seg.type == "text")
		{
			linermessage += boost::any_cast<std::string>(seg.content);
		}
		else if(seg.type == "image")
		{
			auto img_seg = boost::any_cast<avbotmsg_image_segment>(seg.content);
			linermessage += " ";
			linermessage += " 发送了个图片, 但是 bot 暂不支持转发到qq......";
			linermessage += " ";
		}
	}

// 	// 首先是根据 nick 格式化
// 	if ( message.get<std::string>("protocol") != "mail")
// 	{
// 		linermessage += message.get<std::string>("preamble", "");
//
// 		BOOST_FOREACH(const auto & v, message.get_child("message"))
// 		{
// 			if (v.first == "text")
// 			{
// 				linermessage += v.second.data();
// 			}else if (v.first == "url"||v.first == "img")
// 			{
// 				linermessage += " ";
// 				linermessage += v.second.data();
// 				linermessage += " ";
// 			}else if (v.first == "cface"){
// 				// 老版本是显示 执行 HTTP 访问获得 302 跳转后的 URL.
// 				// 但是新的webqq已经把无cookie的访问河了蟹了。于是需要显示的是avbot vps上下载后的地址。不过这个需要呵呵了。
// 				if (m_urlformater)
// 					linermessage += m_urlformater(v.second);
// 				else
// 					linermessage += v.second.get<std::string>("gchatpicurl");
// 				linermessage += " ";
// 			}
// 		}
// 	}else{
//
// 		linermessage  = boost::str(
// 			boost::format("[QQ邮件]\n发件人:%s\n收件人:%s\n主题:%s\n\n%s")
// 			% message.get<std::string>("from") % message.get<std::string>("to") % message.get<std::string>("subject")
// 			% message.get_child("message").data()
// 		);
//  	}
//
	return linermessage;
}

std::string avbot::format_message_for_textIM(const avbotmsg& message)
{
	std::string linermessage;
	// 这个消息是要格式化后给 qq 发送的, 因此 ...

	linermessage += message.sender.preamble;

	linermessage += " ";

	for (const avbotmsg_segment& seg : message.msgs)
	{
		if (seg.type == "text")
		{
			linermessage += boost::any_cast<std::string>(seg.content);
		}
		else if(seg.type == "image")
		{
			auto img_seg = boost::any_cast<avbotmsg_image_segment>(seg.content);

			if (!img_seg.cname.empty())
			{
				if (m_urlformater)
					linermessage += m_urlformater(img_seg.cname);
				else
					linermessage += "发送了个图片, 但是没配置 http 服务, 所以没法转 URL 啦!";
			}
			else
			{
				linermessage += " ";
				linermessage += " 发送了个图片, 但是 bot 暂不支持......";
				linermessage += " ";
			}
		}
	}
	return linermessage;
}

std::vector<avim_msg> avbot::format_message_for_avim(const avbotmsg& message)
{
	std::vector<avim_msg> ret;

	avim_msg msg_segment;

	msg_segment.text = message.sender.preamble;

	ret.push_back(std::move(msg_segment));


	for (const avbotmsg_segment& seg : message.msgs)
	{
		if (seg.type == "text")
		{
			msg_segment.text = boost::any_cast<std::string>(seg.content);
			ret.push_back(std::move(msg_segment));
		}
		else if(seg.type == "image")
		{
			auto img_seg = boost::any_cast<avbotmsg_image_segment>(seg.content);

			if (img_seg.image_data.empty())
			{
				msg_segment.image = img_seg.image_data;
			}
			else
			{
				msg_segment.text = "<发了个图片, 但是没获取到>";
			}
			ret.push_back(std::move(msg_segment));
		}
	}
	return ret;
}

std::string avbot::image_subdir_name( std::string cface )
{
	boost::replace_all( cface, "{", "" );
	boost::replace_all( cface, "}", "" );
	boost::replace_all( cface, "-", "" );
	return cface.substr(0, 2);
}
