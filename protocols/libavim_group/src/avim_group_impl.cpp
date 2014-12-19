#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <iostream>
#include <fstream>
#include "avim_group_impl.hpp"
#include <avproto/easyssl.hpp>

#include "group.pb.h"
#include "avproto/message.hpp"

static std::vector<std::string> get_lines(std::string filename)
{
	std::vector<std::string> ret;
	// 打开 groupdef 文件
	// 为每个 avim 执行一次 send
	if(fs::exists(filename) && fs::is_regular_file(filename))
	{
		std::ifstream groupdef(filename.c_str());
		for(;!groupdef.eof();)
		{
			std::string l;
			std::getline(groupdef, l);

			if(!l.empty())
				ret.push_back(l);
		}
	}
	return ret;
}

avim_group_impl::avim_group_impl(boost::asio::io_service& io, std::string key, std::string cert, std::string groupdeffile)
	: m_io_service(io)
	, m_core(io)
	, m_groupdef(groupdeffile)
{
	m_quitting = false;

	m_key = load_RSA_from_file(key);
	m_cert = load_X509_from_file(cert);

}

avim_group_impl::~avim_group_impl()
{
	m_quitting = true;
}

void avim_group_impl::start()
{
	recive_client_message.connect(std::bind(&avim_group_impl::forward_client_message, shared_from_this(), std::placeholders::_1));
	boost::asio::spawn(m_io_service, std::bind(&avim_group_impl::internal_loop_coroutine, shared_from_this(), std::placeholders::_1));

	start_login();
}

void avim_group_impl::start_login()
{
	if (!m_quitting)
		boost::asio::spawn(m_io_service, std::bind(&avim_group_impl::internal_login_coroutine, shared_from_this(), std::placeholders::_1));
}

void avim_group_impl::send_group_message(std::string sender, std::vector<avim_msg> m)
{
	// 格式化后发送

	message::message_packet av_msg;


	for (avim_msg segmemt: m)
	{
		if(!segmemt.text.empty())
		{
			av_msg.add_avim()->mutable_item_text()->set_text(segmemt.text);
		}

		if(!segmemt.image.empty())
		{
			av_msg.add_avim()->mutable_item_image()->set_image(segmemt.image);
		}
	}

	this->forward_client_message(encode_group_message(sender, "", 0, av_msg));
}

void avim_group_impl::internal_login_coroutine(boost::asio::yield_context yield_context)
{
	m_con.reset(new avjackif(m_io_service));
	m_con->set_pki(m_key, m_cert);

	try
	{
		if (!m_con->async_connect("avim.avplayer.org", "24950", yield_context))
			return start_login();

		if (!m_con->async_handshake(yield_context))
		{
			// stop loop
			std::cerr << "avim login failed!" << std::endl;
			return;
		}

		m_con->signal_notify_remove.connect([this]()
		{
			start_login();
		});

		m_core.add_interface(m_con);

		m_me_addr = av_address_to_string(*m_con->if_address());

		// 添加路由表, metric越大，优先级越低
		m_core.add_route(".+@.+", m_me_addr, m_con->get_ifname(), 100);

		on_group_created(m_me_addr);

	}catch(const std::exception&)
	{
		start_login();
	}
}

void avim_group_impl::internal_loop_coroutine(boost::asio::yield_context yield_context)
{

	for(;!m_quitting;)
	{
		std::string sender, data;

		m_core.async_recvfrom(sender, data, yield_context);

		if(is_group_message(data))
		{
			if (group_message_get_sender(data) == sender)
			{
				// 转发
				recive_client_message(data);

				// 看是否能解码 group 消息
				if (!is_encrypted_message(data))
				{
					auto im = decode_im_message(data);

					std::vector<avim_msg> avmsg;

					for (const message::avim_message& im_item : im.impkt.avim())
					{
						avim_msg item;
						if (im_item.has_item_text())
						{
							item.text = im_item.item_text().text();
						}
						if (im_item.has_item_image())
						{
							item.image = im_item.item_image().image();
						}
						avmsg.push_back(item);
					}
					on_message(m_me_addr, im.sender, avmsg);
				}
			}
		}
        else if (is_control_message(data))
		{
			// 无非就是获取群列表嘛!
			std::string _sender;
			auto bufmsg = decode_control_message(data, _sender);

			if (bufmsg->GetTypeName() == "proto.group.list_request")
			{
				auto list_request= reinterpret_cast<proto::group::list_request*>(bufmsg.get());

				proto::group::list_response list_response;
				list_response.set_result(proto::group::list_response::OK);

				auto avs =  get_lines(m_groupdef);

				for ( auto a : avs )
					list_response.add_list()->assign(a);

				m_core.async_sendto(sender, encode_control_message(list_response), yield_context);
			}
		}
	}
}


void avim_group_impl::forward_client_message(std::string data)
{
	auto avs = get_lines(m_groupdef);
	// 打开 groupdef 文件
	// 为每个 avim 执行一次 send
	for (auto avaddress : avs )
	{
		m_core.async_sendto(avaddress, data, [](boost::system::error_code){});
	}
}
