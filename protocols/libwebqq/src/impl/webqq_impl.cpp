/*
 * Copyright (C) 2012 - 2013  微蔡 <microcai@fedoraproject.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <string.h>
#include <string>
#include <iostream>

#include <boost/random.hpp>
#include <boost/system/system_error.hpp>
#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/json_parser.hpp>
namespace js = boost::property_tree::json_parser;
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/assign.hpp>
#include <boost/scope_exit.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "boost/timedcall.hpp"
#include "boost/stringencodings.hpp"
#include "boost/urlencode.hpp"

#include "webqq_impl.hpp"
#include "constant.hpp"

#include "webqq_status.hpp"
#include "webqq_verify_image.hpp"
#include "webqq_keepalive.hpp"
#include "group_message_sender.hpp"
#include "webqq_loop.hpp"

#include "webqq_send_offfile.hpp"
#include "webqq_check_login.hpp"
#include "webqq_login.hpp"
#include "webqq_poll_message.hpp"

#ifdef WIN32

#include <stdarg.h>

// '_vsnprintf': This function or variable may be unsafe
#pragma warning(disable:4996)

inline int snprintf(char* buf, int len, char const* fmt, ...)
{
	va_list lp;
	va_start(lp, fmt);
	int ret = _vsnprintf(buf, len, fmt, lp);
	va_end(lp);
	if (ret < 0) { buf[len-1] = 0; ret = len-1; }
	return ret;
}

#endif // WIN32

namespace webqq{
namespace qqimpl{

static void dummy(){}

///low level special char mapping
static pt::wptree json_parse( const wchar_t * doc )
{
	pt::wptree jstree;
	std::wstringstream stream;
	stream <<  doc ;
	js::read_json( stream, jstree );
	return jstree;
}

// build webqq and setup defaults
WebQQ::WebQQ(boost::asio::io_service& _io_service, boost::logger& _logger,
	std::string _qqnum, std::string _passwd, bool no_persistent_db)
	: m_io_service( _io_service )
	, m_qqnum( _qqnum )
	, m_passwd( _passwd )
	, m_status( LWQQ_STATUS_OFFLINE )
	, m_cookie_mgr(no_persistent_db? ":memory:": "webqq_persistent")
	, m_buddy_mgr(no_persistent_db? ":memory:": "webqq_persistent")
	, m_vc_queue(_io_service, 1)
	, m_group_message_queue(_io_service, 20) // 最多保留最后的20条未发送消息.
	, m_group_refresh_queue(_io_service)
	, logger(_logger)
{
#ifndef _WIN32
	/* Set msg_id */
	timeval tv;
	long v;
	gettimeofday( &tv, NULL );
	v = tv.tv_usec;
	v = ( v - v % 1000 ) / 1000;
	v = v % 10000 * 10000;
	m_msg_id = v;
#else
	m_msg_id = std::rand();
#endif

	init_face_map();

	m_fetch_groups = true;
}

template<typename Handler>
class async_login_op
{
	Handler m_handler;
	std::shared_ptr<WebQQ> m_webqq;
	int m_counted_network_error;
public:

	async_login_op(std::shared_ptr<WebQQ> _webqq, Handler handler)
		: m_handler(handler), m_webqq(_webqq)
	{
		m_counted_network_error = 0;
	}

	void operator()(boost::system::error_code ec, std::string str)
	{
		m_webqq->logger.info()  << "libwebqq: use cached cookie to avoid login...";
		BOOST_ASIO_CORO_REENTER(this)
		{
			BOOST_ASIO_CORO_YIELD async_poll_message(m_webqq,
				std::bind<void>(*this, std::placeholders::_1, std::string())
			);

			if (!ec)
			{
				m_webqq->logger.info() << "libwebqq: GOOD NEWS! The cached cookies accepted by TX!";

				// login success!
				m_handler(ec);
			}

			// now, try do real login here
			// 判断消息处理结果

			if (ec == error::poll_failed_need_login || ec == error::poll_failed_user_kicked_off)
			{
				// 重新登录
				m_webqq->m_status = LWQQ_STATUS_OFFLINE;

				// 延时 60s  第一次的话不延时,  立即登录.
				BOOST_ASIO_CORO_YIELD boost::delayedcallms(m_webqq->get_ioservice(), 200,
					boost::asio::detail::bind_handler(*this, ec, str)
				);

				m_webqq->logger.info() << "libwebqq: failed with last cookies, doing full login";
			}
			else if ( ec == error::poll_failed_network_error )
			{
				// 网络错误计数 +1 遇到连续的多次错误就要重新登录.
				m_counted_network_error ++;

				if (m_counted_network_error >= 3)
				{
					m_webqq->m_status = LWQQ_STATUS_OFFLINE;

					m_webqq->logger.info() << "libwebqq: too many network errors, try relogin later...";
				}
				// 等待等待就好了，等待 12s
				BOOST_ASIO_CORO_YIELD boost::delayedcallsec(m_webqq->get_ioservice(), 12,
					boost::asio::detail::bind_handler(*this, ec, str)
				);
			}
			else if (ec == error::poll_failed_need_refresh)
			{
				// 重新刷新列表.

				// 就目前来说, 重新登录是最快的实现办法
				// TODO 使用更好的办法.
				m_webqq->m_status = LWQQ_STATUS_OFFLINE;

				m_webqq->logger.info() << "libwebqq: group uin changed, try relogin...";

				// 等待等待就好了，等待 15s
				BOOST_ASIO_CORO_YIELD boost::delayedcallsec(m_webqq->get_ioservice(), 12,
					boost::asio::detail::bind_handler(*this, ec, str)
				);
			}

		}
	}



};

void WebQQ::async_login(webqq::webqq_handler_t handler)
{
	// 读取 一些 cookie
	avhttp::cookies webqqcookie = m_cookie_mgr.get_cookie("http://psession.qq.com/");

	// load 缓存的 一些信息
	m_vfwebqq = webqqcookie["vfwebqq"];
	m_psessionid = webqqcookie["psessionid"];
	m_clientid = webqqcookie["clientid"];

	// 开启登录协程
	async_login_op<webqq::webqq_handler_t>(shared_from_this(), handler);
}

void WebQQ::start()
{
	webqq_loop_start(get_ioservice(), shared_from_this());

	group_message_sender(shared_from_this());

	webqq_keepalive(shared_from_this());
}

void WebQQ::stop()
{
	m_group_message_queue.cancele();
	m_group_refresh_queue.cancele();
	m_status = LWQQ_STATUS_QUITTING;
}

// last step of a login process
// and this will be callded every other minutes to prevent foce kick off.
void  WebQQ::change_status(LWQQ_STATUS status, std::function<void (boost::system::error_code) > handler)
{
	async_change_status(shared_from_this(), status, handler);
}

void WebQQ::send_group_message(const qqGroup& group, std::string msg, send_group_message_cb donecb )
{
	send_group_message( group.gid, msg, donecb );
}

void WebQQ::send_group_message( std::string group, std::string msg, send_group_message_cb donecb )
{
	m_group_message_queue.push(boost::make_tuple( group, msg, donecb ));
}

void WebQQ::update_group_member(std::shared_ptr<qqGroup> group, webqq::webqq_handler_t handler)
{
	m_group_refresh_queue.push(
		boost::make_tuple(handler, group->gid)
	);
}

void WebQQ::send_offline_file(std::string uin, std::string filename, webqq::webqq_handler_t handler)
{
	async_send_offline_file(shared_from_this(), uin, filename, handler);
}

class SYMBOL_HIDDEN buddy_uin_to_qqnumber {
public:
	// 将　qqBuddy 里的　uin 转化为　qq 号码.
	template<class Handler>
	buddy_uin_to_qqnumber( std::shared_ptr<WebQQ> _webqq, std::string uin, Handler handler )
		: _io_service(_webqq->get_ioservice())
		, m_webqq(_webqq)
	{
		std::string url = boost::str(
			boost::format( "%s/api/get_friend_uin2?tuin=%s&verifysession=&type=1&code=&vfwebqq=%s" )
			% "http://s.web2.qq.com"
			% uin
			% _webqq->m_vfwebqq
		);

		m_stream = std::make_shared<avhttp::http_stream>(boost::ref(_io_service));
		m_webqq->m_cookie_mgr.get_cookie(url, *m_stream);
		m_stream->request_options(
			avhttp::request_opts()
			( avhttp::http_options::http_version , "HTTP/1.0" )
			( avhttp::http_options::referer, "http://s.web2.qq.com/proxy.html?v=201304220930&callback=1&id=3" )
			( avhttp::http_options::content_type, "UTF-8" )
			( avhttp::http_options::connection, "close" )
		);

		std::shared_ptr<boost::asio::streambuf> buffer = std::make_shared<boost::asio::streambuf>();

		avhttp::async_read_body(
			*m_stream, url, *buffer,
			std::bind<void>( *this, std::placeholders::_1, buffer, handler)
		);
	}

	template <class Handler>
	void operator()( const boost::system::error_code& ec, std::shared_ptr<boost::asio::streambuf> buffer, Handler handler )
	{
		// 获得的返回代码类似
		// {"retcode":0,"result":{"uiuin":"","account":2664046919,"uin":721281587}}
		pt::ptree jsonobj;
		std::iostream resultjson( buffer.get() );

		try
		{
			// 处理.
			pt::json_parser::read_json( resultjson, jsonobj );
			int retcode = jsonobj.get<int>("retcode");
			if (retcode ==0)
			{
				std::string qqnum = jsonobj.get<std::string>( "result.account" );
				return _io_service.post(
					boost::asio::detail::bind_handler(handler, qqnum)
				);
			}
		}
		catch(const pt::ptree_error & badpath)
		{
			m_webqq->logger.err() <<  __FILE__ << " : " << __LINE__ << " : " <<  "bad path" <<  badpath.what();
		}
		// 判断是否执行了太久，太久的话，先刷一次消息，来更新下vfwebqq这个cookie
		_io_service.post( boost::asio::detail::bind_handler( handler, std::string( "" ) ) );
	}
private:
	boost::asio::io_service& _io_service;
	std::shared_ptr<WebQQ> m_webqq;
	read_streamptr m_stream;
};

class SYMBOL_HIDDEN update_group_member_qq_op : boost::asio::coroutine {
public:
	update_group_member_qq_op( std::shared_ptr<WebQQ>  _webqq, std::shared_ptr<qqGroup> _group )
		: group( _group ), m_webqq( _webqq )
	{
		m_uins = m_webqq->m_buddy_mgr.get_group_all_buddies_uin(_group->gid);

		m_webqq->get_ioservice().post(
			boost::asio::detail::bind_handler(*this, std::string())
		);
	}

	void operator()( std::string qqnum )
	{
		//我说了是一个一个的更新对吧，可不能一次发起　N 个连接同时更新，会被TX拉黑名单的.
		BOOST_ASIO_CORO_REENTER( this )
		{
			for( i = 0 ; i < m_uins.size() ; i++ )
			{
				if (!m_webqq->m_buddy_mgr.buddy_has_qqnum(m_uins[i]))
				{
					BOOST_ASIO_CORO_YIELD buddy_uin_to_qqnumber(m_webqq, m_uins[i], *this);
					if ( qqnum == "-1")
						return;
					else
						m_webqq->m_buddy_mgr.map_buddy_qqnum(m_uins[i], qqnum);
				}
			}
		}
	}
private:
	std::shared_ptr<qqGroup> group;
	std::shared_ptr<WebQQ> m_webqq;

	std::vector<std::string> m_uins;

	int i;
};

//　将组成员的 QQ 号码一个一个更新过来.
void WebQQ::update_group_member_qq(std::shared_ptr<qqGroup> group )
{
	update_group_member_qq_op op( shared_from_this(), group );
}

qqGroup_ptr WebQQ::get_Group_by_gid( std::string gid )
{
	return m_buddy_mgr.get_group_by_gid(gid);
}

qqGroup_ptr WebQQ::get_Group_by_qq( std::string qq )
{
	return m_buddy_mgr.get_group_by_qq(qq);
}

std::vector< qqBuddy_ptr > WebQQ::get_buddies()
{
	return m_buddy_mgr.get_buddies();
}

void WebQQ::get_verify_image( std::string vcimgid, webqq::webqq_handler_string_t handler)
{
	detail::get_verify_image_op op(shared_from_this(), vcimgid, handler);
}

void WebQQ::cb_newbee_group_join( qqGroup_ptr group,  std::string uid )
{
	if (group)
	// 报告新人入群.
	boost::delayedcallsec(get_ioservice(), 30,
		boost::bind(boost::ref(signewbuddy), group, group->get_Buddy_by_uin(uid))
	);
}

void WebQQ::cb_fetch_aid(const boost::system::error_code& ec, read_streamptr stream,  std::shared_ptr<boost::asio::streambuf> buf, std::function<void(const boost::system::error_code&, std::string)> handler)
{
	if (!ec)
	{
		// 获取到咯, 更新 verifysession
		m_cookie_mgr.save_cookie(*stream);

		handler(boost::system::error_code(), std::string(boost::asio::buffer_cast<const char*>(buf->data()), boost::asio::buffer_size(buf->data())));
		return;
	}
	handler(ec, std::string());
}

void WebQQ::fetch_aid(std::string arg, std::function<void(const boost::system::error_code&, std::string)> handler)
{
	std::string url = boost::str(
		boost::format("http://captcha.qq.com/getimage?%s") % arg
	);

	read_streamptr stream(new avhttp::http_stream(m_io_service));
	m_cookie_mgr.get_cookie(url, *stream);
	stream->request_options(
		avhttp::request_opts()
			(avhttp::http_options::referer, "http://web.qq.com/")
			(avhttp::http_options::connection, "close")
	);

	std::shared_ptr<boost::asio::streambuf> buffer = std::make_shared<boost::asio::streambuf>();
	avhttp::async_read_body(*stream, url,*buffer, std::bind(&WebQQ::cb_fetch_aid, shared_from_this(), std::placeholders::_1, stream, buffer, handler ));
}

static void cb_search_group_vcode(const boost::system::error_code& ec, std::string vcodedata, webqq::search_group_handler handler, qqGroup_ptr group)
{
	if (!ec){
		handler(group, 1, vcodedata);
	}else{
		group.reset();
		handler(group, 0, vcodedata);
	}
}

void WebQQ::cb_search_group(std::string groupqqnum, const boost::system::error_code& ec, read_streamptr stream,  std::shared_ptr<boost::asio::streambuf> buf, webqq::search_group_handler handler)
{
	pt::ptree	jsobj;
	std::istream jsondata(buf.get());
	qqGroup_ptr  group;

	if (!ec){
		// 读取 json 格式
		js::read_json(jsondata, jsobj);
		group.reset(new qqGroup);
		group->qqnum = groupqqnum;
		try{
			if(jsobj.get<int>("retcode") == 0){
				group->qqnum = jsobj.get<std::string>("result..GE");
				group->code = jsobj.get<std::string>("result..GEX");
			}else if(jsobj.get<int>("retcode") == 100110){
				// 需要验证码, 先获取验证码图片，然后回调
				fetch_aid(boost::str(boost::format("aid=1003901&%ld") % std::time(NULL)),
					std::bind(cb_search_group_vcode, std::placeholders::_1, std::placeholders::_2, handler, group));
				return;
			}else if (jsobj.get<int>("retcode")==100102){
				// 验证码错误
				group.reset();
			}
		}catch (...){
			group.reset();
		}
	}
	handler(group, 0, "");
}

void WebQQ::search_group(std::string groupqqnum, std::string vfcode, webqq::search_group_handler handler)
{
	// GET /keycgi/qqweb/group/search.do?pg=1&perpage=10&all=82069263&c1=0&c2=0&c3=0&st=0&vfcode=&type=1&vfwebqq=59b09b83f622d820cd9ee4e04d4f4e4664e6704ee7ac487ce00595f8c539476b49fdcc372e1d11ea&t=1365138435110 HTTP/1.1
	std::string url = boost::str(
		boost::format("%s/keycgi/qqweb/group/search.do?pg=1&perpage=10&all=%s&c1=0&c2=0&c3=0&st=0&vfcode=%s&type=1&vfwebqq=%s&t=%ld")
			%  "http://cgi.web2.qq.com" % groupqqnum % vfcode %  m_vfwebqq % std::time(NULL)
	);

	read_streamptr stream(new avhttp::http_stream(m_io_service));
	m_cookie_mgr.get_cookie(url, *stream);
	stream->request_options(avhttp::request_opts()
		(avhttp::http_options::content_type, "utf-8")
		(avhttp::http_options::referer, "http://cgi.web2.qq.com/proxy.html?v=201304220930&callback=1&id=2")
		(avhttp::http_options::connection, "close")
	);

	std::shared_ptr<boost::asio::streambuf> buffer = std::make_shared<boost::asio::streambuf>();
	avhttp::async_read_body(*stream, url, * buffer, std::bind(&WebQQ::cb_search_group, shared_from_this(), groupqqnum, std::placeholders::_1, stream, buffer, handler));
}

static void cb_join_group_vcode(const boost::system::error_code& ec, std::string vcodedata, webqq::join_group_handler handler, qqGroup_ptr group)
{
	if (!ec){
		handler(group, 1, vcodedata);
	}else{
		group.reset();
		handler(group, 0, vcodedata);
	}
}


void WebQQ::cb_join_group( qqGroup_ptr group, const boost::system::error_code& ec, read_streamptr stream, std::shared_ptr<boost::asio::streambuf> buf, webqq::join_group_handler handler )
{
	// 检查返回值是不是 retcode == 0
	pt::ptree	jsobj;
	std::istream jsondata(buf.get());
	try{
		js::read_json(jsondata, jsobj);
		js::write_json(std::cerr, jsobj);

		if(jsobj.get<int>("retcode") == 0){
			// 搞定！群加入咯. 等待管理员吧.
			handler(group, 0, "");
			// 获取群的其他信息
			// GET http://s.web2.qq.com/api/get_group_public_info2?gcode=3272859045&vfwebqq=f08e7a200fd0be375d753d3fedfd24e99f6ba0a8063005030bb95f9fa4b7e0c30415ae74e77709e3&t=1365161430494 HTTP/1.1
		}else if(jsobj.get<int>("retcode") == 100001){
			std::cout << "原因： " <<   jsobj.get<std::string>("tips") <<  std::endl;
			// 需要验证码, 先获取验证码图片，然后回调
			fetch_aid(boost::str(boost::format("aid=%s&_=%ld") % APPID % std::time(NULL)), std::bind(cb_join_group_vcode, std::placeholders::_1, std::placeholders::_2, handler, group) );
		}else{
			// 需要验证码, 先获取验证码图片，然后回调
			fetch_aid(boost::str(boost::format("aid=%s&_=%ld") % APPID % std::time(NULL)), std::bind(cb_join_group_vcode, std::placeholders::_1, std::placeholders::_2, handler, group) );
		}
	}catch (...){
		handler(qqGroup_ptr(), 0, "");
	}
}


void WebQQ::join_group(qqGroup_ptr group, std::string vfcode, webqq::join_group_handler handler )
{
	std::string url = "http://s.web2.qq.com/api/apply_join_group2";

	std::string postdata =	boost::str(
								boost::format(
									"{\"gcode\":%s,"
									"\"code\":\"%s\","
									"\"vfy\":\"%s\","
									"\"msg\":\"avbot\","
									"\"vfwebqq\":\"%s\"}" )
								% group->code
								% vfcode
								% m_cookie_mgr.get_cookie(url)["verifysession"]
								% m_vfwebqq
							);

	postdata = std::string("r=") + avhttp::detail::escape_string(postdata);

	read_streamptr stream(new avhttp::http_stream(m_io_service));
	m_cookie_mgr.get_cookie(url, *stream);

	stream->request_options(avhttp::request_opts()
		(avhttp::http_options::http_version, "HTTP/1.0")
		(avhttp::http_options::content_type, "application/x-www-form-urlencoded; charset=UTF-8")
		(avhttp::http_options::referer, "http://s.web2.qq.com/proxy.html?v=201304220930&callback=1&id=1")
		(avhttp::http_options::connection, "close")
		(avhttp::http_options::request_method, "POST")
		(avhttp::http_options::request_body, postdata)
		(avhttp::http_options::content_length, boost::lexical_cast<std::string>(postdata.length()))
	);

	std::shared_ptr<boost::asio::streambuf> buffer = std::make_shared<boost::asio::streambuf>();

	avhttp::async_read_body(*stream, url, * buffer, std::bind(&WebQQ::cb_join_group, shared_from_this(), group, std::placeholders::_1, stream, buffer, handler));
}

} // namespace qqimpl
} // namespace webqq
