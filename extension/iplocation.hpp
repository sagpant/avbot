
#ifndef iplocation_h__
#define iplocation_h__

#include <boost/regex.hpp>
#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <avhttp.hpp>
#include <avhttp/async_read_body.hpp>

#include "boost/stringencodings.hpp"
#include "qqwry/ipdb.hpp"
#include <libavbot/avchannel.hpp>

namespace iplocationdetail{

// download qqwy.dat file and decode it and return decoded data.
// so the user can decide to write to file or just pass it to the constructor
template<class uncompressfunc, class Handler>
struct download_qqwry_dat_op : boost::asio::coroutine
{
	boost::asio::io_service& m_io_service;

	uncompressfunc m_uncompress;
	Handler m_handler;

	std::shared_ptr<avhttp::http_stream> m_http_stream;
	std::shared_ptr<boost::asio::streambuf> m_buf_copywrite_rar;
	std::shared_ptr<boost::asio::streambuf> m_buf_qqwry_rar;

	download_qqwry_dat_op(boost::asio::io_service& _io_service, uncompressfunc uncompress, Handler handler)
		: m_io_service(_io_service)
		, m_uncompress(uncompress)
		, m_handler(handler)
	{
		// start downloading copyrite.rar
		m_http_stream.reset(new avhttp::http_stream(m_io_service));
		m_buf_copywrite_rar.reset(new boost::asio::streambuf);
		avhttp::async_read_body(*m_http_stream, "http://update.cz88.net/ip/copywrite.rar", *m_buf_copywrite_rar, *this);
	}

	void operator()(boost::system::error_code ec, std::size_t bytes_transfered)
	{
		BOOST_ASIO_CORO_REENTER(this)
		{
			// 然后下 qqwry.dat
			m_http_stream.reset(new avhttp::http_stream(m_io_service));
			m_buf_qqwry_rar.reset(new boost::asio::streambuf);

			BOOST_ASIO_CORO_YIELD avhttp::async_read_body(
				*m_http_stream,
				"http://update.cz88.net/ip/qqwry.rar",
				*m_buf_qqwry_rar,
				*this
			);

			// 解码
			{
				std::string copywrite, qqwry;
				copywrite.resize(boost::asio::buffer_size(m_buf_copywrite_rar->data()));
				m_buf_copywrite_rar->sgetn(&copywrite[0], copywrite.size());
				qqwry.resize(bytes_transfered);
				m_buf_qqwry_rar->sgetn(&qqwry[0], bytes_transfered);

				std::string m_decoded = QQWry::decodeQQWryDat(copywrite, qqwry, m_uncompress);

				// callback
				m_handler(m_decoded);
			}
		}
	}
};

template<class uncompressfunc, class Handler>
void download_qqwry_dat(boost::asio::io_service& io_service, uncompressfunc uncompress, Handler handler)
{
	download_qqwry_dat_op<uncompressfunc, Handler> op(io_service, uncompress, handler);
}

struct ipdb_mgr
{
private:
	boost::asio::io_service & m_io_service;
	boost::function<int(unsigned char *pDest, unsigned long *pDest_len, const unsigned char *pSource, unsigned long source_len)> m_uncompress;
	std::string decoded_data;
public:
	std::shared_ptr<QQWry::ipdb> db;
public:
	template<typename uncompressfunc>
	ipdb_mgr(boost::asio::io_service & _io_servcie, uncompressfunc uncompress_)
		: m_uncompress(uncompress_)
		, m_io_service(_io_servcie)
	{

	}

	bool search_and_build_db()
	{
		if (boost::filesystem::exists("/tmp/qqwry.dat"))
		{
			// good
			db.reset(new QQWry::ipdb("/tmp/qqwry.dat"));
			return true;
		}

		boost::filesystem::path p;
		p = "qqwry.dat";
		// find qqwry.dat
		if (boost::filesystem::exists(p))
		{
			db.reset(new QQWry::ipdb("qqwry.dat"));
			return true;
			// good
		}

#ifdef _WIN32
		// find pathof(avbot.exe)/qqwry.dat
		{char exePATH[4096];
		::GetModuleFileName(NULL, exePATH, sizeof(exePATH));
		p = exePATH; }

		p = p.parent_path() / "qqwry.dat";

		if (boost::filesystem::exists(p))
		{
			db.reset(new QQWry::ipdb(p.string().c_str()));
			return true;
			// good
		}			// construct
#endif // _WIN32
		// find /var/lib/qqwry.dat
		if (boost::filesystem::exists("/var/lib/qqwry.dat"))
		{
			db.reset(new QQWry::ipdb("/var/lib/qqwry.dat"));
			return true;
			// good
		}
		// 找不到，得找个机会去 .. 下载

		download_qqwry_dat(
			m_io_service,
			m_uncompress,
			boost::bind(&ipdb_mgr::qqwry_downloaded, this, _1)
		);
		return false;
	}

	void qqwry_downloaded(std::string decoded_qqwry_dat)
	{
		decoded_data = boost::move(decoded_qqwry_dat);
		// save to /tmp/qqwry.dat or qqwry.dat depend on the OS
		boost::filesystem::path savepath;
#ifndef _WIN32
		savepath = "/tmp/qqwry.dat";
#else
		savepath = "qqwry.dat";
#endif
		std::ofstream qqwrydatfile(savepath.string().c_str(), std::ios::binary);
		qqwrydatfile.write(decoded_data.data(), decoded_data.size());
		db.reset(new QQWry::ipdb(decoded_data.data(), decoded_data.length()));
	}

	bool is_ready() const
	{
		return !!db;
	}
};

} // namespace iplocationdetail

template<typename MsgSender>
class iplocation
{
public:
	void operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context)
	{
		std::string textmsg = msg.to_plain_text();

		in_addr ipaddr;

		boost::cmatch what;

		try{
			if (
				boost::regex_search(
					textmsg.c_str(),
					what,
					boost::regex(
						"((((\\d{1,2})|(1\\d{2})|(2[0-4]\\d)|(25[0-5]))\\.){3}((1\\d{2})|(2[0-4]\\d)|(25[0-5])|(\\d{1,2})))"
					),
					boost::match_default
				)
			)
			{
				if (!m_ipdb_mgr->is_ready())
				{
					m_sender(std::string("吼吼，还木有纯真数据库，暂时无法查询"));
					return;
				}

				std::string matchedip = what[1];
				ipaddr.s_addr = ::inet_addr(matchedip.c_str());
				// ip 地址是这样查询的 .qqbot locate 8.8.8.8
				// 或者直接聊天内容就是完整的一个 ip 地址
				QQWry::IPLocation l = m_ipdb_mgr->db->GetIPLocation(ipaddr);

				// 找到后，发给聊天窗口.

				m_sender(boost::str(
					boost::format("你在聊天中提到的 %s 地址，经过 avbot 仔细的考察，发现它在 %s %s")
					% matchedip
					% local_encode_to_utf8(l.country)
					% local_encode_to_utf8(l.area)
				));
			}
		}
		catch (const std::runtime_error&)
		{
		}
		// 或者通过其他方式激活

	}
public:
	iplocation(boost::asio::io_service & _io_service, const MsgSender & sender, std::shared_ptr<iplocationdetail::ipdb_mgr> _ipdb_mgr)
		: io_service( _io_service )
		, m_sender( sender )
		, m_ipdb_mgr(_ipdb_mgr)
	{
	}
private:
	// 这么多 extension 的 qqwry 数据库当然得共享啦！ 共享那肯定就是用的共享指针.
	std::shared_ptr<iplocationdetail::ipdb_mgr> m_ipdb_mgr;
	boost::asio::io_service &io_service;
	MsgSender m_sender;
};

template<typename MsgSender>
iplocation<typename boost::remove_reference<MsgSender>::type> make_iplocation(boost::asio::io_service & _io_service, const MsgSender & sender, std::shared_ptr<iplocationdetail::ipdb_mgr> _ipdb_mgr)
{
	return iplocation<typename boost::remove_reference<MsgSender>::type>(_io_service, sender, _ipdb_mgr);
}

#endif // iplocation_h__
