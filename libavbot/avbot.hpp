
#pragma once

#include <atomic>
#include <vector>
#include <memory>
#include <boost/config.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/any.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/variant.hpp>
#include <boost/logger.hpp>

#include "avchannel.hpp"
#include "avaccount.hpp"

#include "libwebqq/webqq.hpp"
#include "irc.hpp"
#include "libxmpp/xmpp.hpp"
#include "mx.hpp"
#include "avim_group.hpp"

class BOOST_SYMBOL_VISIBLE avbot : boost::noncopyable
{
public:
	typedef boost::variant<
		std::shared_ptr<webqq::webqq>, std::shared_ptr<avim>, std::shared_ptr<irc::client>, std::shared_ptr<xmpp>
	> accounts_t;
	typedef std::function<void (std::string) > need_verify_image;
	typedef boost::signals2::signal<void(channel_identifier, avbotmsg) > on_message_type;

private:
	boost::asio::io_service & m_io_service;
	boost::logger& logger;

	boost::any m_I_need_vc;

	std::vector<std::shared_ptr<webqq::webqq>> m_qq_accounts;
	std::vector<std::shared_ptr<irc::client>> m_irc_accounts;
	std::vector<std::shared_ptr<xmpp>> m_xmpp_accounts;
	std::vector<std::shared_ptr<avim>> m_avim_accounts;
	std::vector<std::shared_ptr<avaccount>> m_std_accounts;
	std::shared_ptr<mx::mx> m_mail_account;

	std::map<std::string, std::shared_ptr<avchannel>> m_avchannels;

	// maps from protocol and room name to accounts
	std::vector<std::pair<channel_identifier, accounts_t>> m_account_mapping;

	std::shared_ptr<std::atomic<bool>> m_quit;

public:
	// 每当有消息的时候激发.
	on_message_type on_message;

public:
	avbot(boost::asio::io_service & io_service, boost::logger&);
	~avbot();

	boost::asio::io_service & get_io_service();

	// 添加一个 channel
	void add_channel(std::string name, std::shared_ptr<avchannel>);
    std::shared_ptr<avchannel> get_channel(channel_identifier cid);

public:
	void send_avbot_message(channel_identifier, avbotmsg, boost::asio::yield_context);
	void send_broadcast_message(std::string channel_name, avbotmsg);
    void broadcast_message_to_all_channels(avbotmsg);
    void broadcast_textmessage_to_all_channels(std::string);

public:
	// 这里是一些公开的成员变量.

	// 用了传入一个 url 生成器，这样把 qq 的消息里的图片地址转换为 vps 上跑的 http 服务的地址。
	std::function<std::string(std::string)> m_urlformater;

	// 调用此函数保存图片
	std::function<void(std::string digestname, std::string data)> m_image_saver;
	// 传入一个 image 缓存器, 如果返回了数据, 则 avbot 不会去下载图片
	std::function<std::string(std::string digestname)> m_image_cacher;

	std::string preamble_qq_fmt, preamble_irc_fmt, preamble_xmpp_fmt;

public:
	std::shared_ptr<mx::mx> get_mx(){return m_mail_account;}
public:

	// 调用这个添加 QQ 帐号. need_verify_image 会在需要登录验证码的时候调用，buffer 里包含了验证码图片.
	std::shared_ptr<webqq::webqq> add_qq_account(std::string qqnumber, std::string password, need_verify_image cb, bool no_persistent_db = false);

	// 如风发生了 需要验证码 这样的事情，就麻烦调用这个把识别后的验证码喂进去.
	void feed_login_verify_code(std::string vcode, std::function<void()> badvcreporter = std::function<void()>());

	// 调用这个添加 IRC 帐号.
	std::shared_ptr<irc::client> add_irc_account(std::string nick = autonick(), std::string password = "" , std::string server = "irc.freenode.net:6667", bool use_ssl = false);

	// 调用这个设置 XMPP 账户.
	std::shared_ptr<xmpp> add_xmpp_account(std::string user, std::string password, std::string nick="avbot", std::string server="");

	// 调用这个设置avim账户
	std::shared_ptr<avim> add_avim_account( std::string key, std::string cert, std::string groupdeffile );

	// 调用这个添加其他帐号!
	void add_std_account(std::shared_ptr<avaccount>);

	// 调用这个设置邮件账户.
	void set_mail_account(std::string mailaddr, std::string password, std::string pop3server = "", std::string smtpserver = "");

private:
	void callback_on_qq_group_message(std::weak_ptr<webqq::webqq> qq_account, std::string group_code, std::string who, std::vector<webqq::qqMsg> msg, boost::asio::yield_context);
	void callback_on_irc_message(std::weak_ptr<irc::client>, irc::irc_msg pMsg);
	void callback_on_xmpp_group_message(std::weak_ptr<xmpp>, std::string xmpproom, std::string who, std::string message);
	void callback_on_mail(mailcontent mail, mx::pop3::call_to_continue_function call_to_contiune);
	void callback_on_avim(std::string reciver, std::string sender, std::vector<avim_msg>);
	void callback_on_std_account(std::weak_ptr<avaccount>, channel_identifier, const avbotmsg&);
private:
	void callback_on_qq_group_found(std::weak_ptr<webqq::webqq>, webqq::qqGroup_ptr);
	void callback_on_qq_group_newbee(std::weak_ptr<webqq::webqq>, webqq::qqGroup_ptr, webqq::qqBuddy_ptr);

	void callback_on_xmpp_room_joined(std::weak_ptr<xmpp>, std::string);
	void callback_on_irc_room_joined(std::weak_ptr<irc::client>, std::string);
	void callback_on_avim_room_created(std::weak_ptr<avim>, std::string);

private:
	void forward_message(const channel_identifier& i, const avbotmsg& m);
public:
	// auto pick an nick name for IRC
	static std::string autonick();
	static std::string image_subdir_name(std::string cface);

	// used by internal visitor
	std::string format_message_for_qq(const avbotmsg& message);
	std::string format_message_for_textIM(const avbotmsg& message);
	std::vector<avim_msg> format_message_for_avim(const avbotmsg& message);
};
