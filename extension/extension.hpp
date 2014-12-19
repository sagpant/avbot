
#pragma once

#include <string>
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>

#include "libavbot/avbot.hpp"
#include "boost/stringencodings.hpp"

class avbot_extension;

namespace detail{
class avbotexteison_interface
{
public:
	virtual void operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context) = 0;
};

template<class ExtensionType>
class avbotexteison_adapter : public avbotexteison_interface
{
	ExtensionType m_pextension;
	void operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context)
	{
		(m_pextension)(cid, msg, sender, yield_context);
	}

public:
	avbotexteison_adapter(const ExtensionType & obj)
		: m_pextension(obj)
	{
	}

	avbotexteison_adapter(ExtensionType && obj)
		: m_pextension(obj)
	{
	}

	avbotexteison_adapter(ExtensionType * obj)
		: m_pextension(*obj)
	{
	}
};

} // namespace detail

// type erasure for avbot extension
class avbot_extension
{
	std::shared_ptr<detail::avbotexteison_interface> m_exteison_obj;
	std::string m_channel_name;

public:

	template<class ExtensionType>
	avbot_extension(std::string channel_name, const ExtensionType & extensionobj)
		: m_channel_name( channel_name )
	{
		m_exteison_obj.reset(
			new detail::avbotexteison_adapter<
				typename boost::remove_reference<ExtensionType>::type
			>(extensionobj)
		);
	}

	template<class ExtensionType>
	avbot_extension & operator = (const ExtensionType & extensionobj)
	{
		m_exteison_obj.reset(
			new detail::avbotexteison_adapter<
				typename boost::remove_reference<ExtensionType>::type
			>(extensionobj)
		);
		return *this;
	}

	template<class ExtensionType>
	avbot_extension & operator = (ExtensionType * extensionobj)
	{
		m_exteison_obj.reset(
			new detail::avbotexteison_adapter<
			typename boost::remove_reference<ExtensionType>::type
			>(extensionobj)
		);
		return *this;
	}

	avbot_extension & operator = (const avbot_extension & rhs)
	{
		m_exteison_obj = rhs.m_exteison_obj;
		m_channel_name = rhs.m_channel_name;
		return *this;
	}

	void operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context)
	{
		// 调用实际的函数
		(*m_exteison_obj)(cid, msg, sender, yield_context);
	}
	typedef void result_type;
};

void new_channel_set_extension(avbot& mybot, avchannel& channel, std::string channel_name);
