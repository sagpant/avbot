#include <string>
#include <algorithm>
#include <boost/date_time/time_duration.hpp>
#include <boost/algorithm/string.hpp>

#include "boost/stringencodings.hpp"

#include "urlpreview.hpp"
#include "html.hpp"
#include "html5.hpp"

namespace detail
{

static inline bool is_html( const std::string contenttype )
{
	if( contenttype.empty() )
		return false;

	if( contenttype == "text/html" )
		return true;

	if( contenttype.find( "text/html" ) != std::string::npos )
		return true;
	return false;
}

struct urlpreview
{
	boost::asio::io_service &io_service;
	boost::function<void ( std::string ) > m_sender;
	std::string m_speaker;
	std::string m_url;
	std::shared_ptr<avhttp::http_stream> m_httpstream;

	std::shared_ptr<boost::array<char, 512>> m_content;

	int m_redirect ;

	std::shared_ptr<html::dom> m_html_page;

	template<class MsgSender>
	urlpreview(boost::asio::io_service &_io_service, MsgSender sender,
		std::string speaker, std::string url, int redirectlevel = 0)
		: io_service( _io_service ), m_sender( sender )
		, m_speaker( speaker ), m_url( url )
		, m_httpstream( new avhttp::http_stream( io_service ) )
		, m_redirect(redirectlevel)
	{
		// 开启 avhttp 下载页面
		m_httpstream->check_certificate( false );
		try{
			// 防止 非法 url 导致错误
			m_httpstream->async_open( url, *this );
		}catch (...){}
	}

	// 打开在这里
	void operator()( const boost::system::error_code &ec )
	{
		if( ec )
		{
			// 报告出错。
			m_sender( boost::str( boost::format("@%s, 获取url有错 %s") % m_speaker % ec.message() ) );
			return;
		}

		// 根据 content_type 了， 如果不是 text/html 的就不要继续下去了.
		avhttp::response_opts opt = m_httpstream->response_options();

		if( ! is_html( opt.find( avhttp::http_options::content_type ) ) )
		{
			// 报告类型就可以
			m_sender( boost::str( boost::format("%s 发的 ⇪ 类型是 %s ") % m_speaker % opt.find( avhttp::http_options::content_type ) ) );
			return;
		}

		m_content = std::make_shared<boost::array<char, 512>>();

		m_html_page = std::make_shared<html::dom>();

		m_httpstream->async_read_some(boost::asio::buffer(*m_content, 512), *this);
	}

	// boost::asio::async_read 回调.
	void operator()( const boost::system::error_code &ec, int bytes_transferred )
	{
		bool try_http_redirect = false;
		if( ec && ec != boost::asio::error::eof )
		{
			m_sender( boost::str( boost::format("@%s, 获取url有错 %s") % m_speaker % ec.message() ) );
			return;
		}

		m_html_page->append_partial_html(std::string( &(*m_content)[0], bytes_transferred));

		// 解析 <title>
		auto title = (*m_html_page)["title"].to_plain_text();

		if (title.empty() && !ec)
		{
			m_httpstream->async_read_some(boost::asio::buffer(*m_content, 512), *this);
			return;
		}else if (ec ==  boost::asio::error::eof)
		{
			try_http_redirect = true;
			// 还是没有 title ?
			m_sender( boost::str( boost::format("@%s, 获取url有错 %s") % m_speaker % ec.message() ) );
			return;
		}

		// 获取charset
		auto charset = (*m_html_page).charset();

		if(!try_http_redirect)
		{
			try
			{
				if( charset != "utf8" && charset != "utf" && charset != "utf-8" )
				{
					title = boost::locale::conv::between( title, "UTF-8", charset );
				}

				boost::trim( title );

				title = html_unescape(title);
				// 将 &bnp 这种反格式化.
				m_sender( boost::str( boost::format("@%s ⇪ 标题： %s ") % m_speaker % title ) );
			}
			catch( const std::runtime_error & )
			{
				m_sender( boost::str( boost::format("@%s ⇪ 解码网页发生错误 ") % m_speaker ) );
			}
		}
		else
		{
			// 解析是不是 html 重定向

			auto dom_page = (*m_html_page)["meta [http-equiv][content][url]"];

			auto cd = dom_page.get_children();

			if (!cd.empty())
			{
				auto url = cd[0]->get_attr("url");

				if (m_redirect < 10  && !url.empty())
				{
					urlpreview(io_service, m_sender, m_speaker, url, m_redirect + 1);
				}else
				{
					m_sender( boost::str( boost::format("@%s ⇪ url 无标题 ") % m_speaker ) );
				}
			}
		}
	}
};

}

void urlpreview::operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context)
{
	// 检查 URL
	std::string txt = msg.to_plain_text();
	std::string speaker = msg.sender.nick; // 发了 url 的人的 nick

	// 用正则表达式
	// http://.*
	// https://.*
	// 统一为
	// http[s]?://[^ ].*
	// 使用 boost_regex_search
	boost::regex ex( "https?://[\\w\\d\\.\\?\\$\\-\\+\\|&@#/%=~_!:,]*[\\w\\d\\+&@#/%=~_\\|\\$]" );
	boost::cmatch what;

	while(boost::regex_search( txt.c_str(), what, ex ))
	{
		std::string url = what[0];
		do_urlpreview(speaker, boost::trim_copy(url), boost::posix_time::from_time_t(std::time(NULL)));
		std::size_t pos = txt.find_first_of(url);
		txt.erase(0, pos + url.length());
	}
}

bool urlpreview::can_preview(std::string speaker, std::string urlstr)
{
	boost::regex ex;
	// 内置数据库, 然后是 ${qqlog}/blockurls.txt
	try{

		avhttp::url url(urlstr);


		if(url.host() == "web.qq.com" || url.host() =="web2.qq.com")
		{
			return false;
		}

		// 遍历 blockurls.txt, 每个都是正则表达式!
		try{

			std::ifstream blockurls("blockurls");
			while(blockurls.is_open() && !blockurls.eof())
			{
				std::string urlregex;
				std::getline(blockurls, urlregex);
				if(urlregex.empty())
					break;
				try{
					ex.set_expression(urlregex);
					if(boost::regex_match(urlstr.c_str(), ex))
						return false;
				}catch(const boost::regex_error&){}
			}

		}catch(const std::runtime_error&)
		{
		}
	}catch (...)
	{
		return false;
	}
	return true;
}


static bool find_url(std::string url, const std::pair<std::string, boost::posix_time::ptime> & item)
{
	return item.first == url;
}

void urlpreview::do_urlpreview(std::string speaker, std::string url, boost::posix_time::ptime current )
{
	// 先查看黑名单, 符合黑名单要求的要过滤掉
	if(!can_preview(speaker,url))
		return;
	boost::posix_time::time_duration jiange = boost::posix_time::minutes(99);
	// 到 urllist 里找一下是否有重复的.
	if (!urllist->empty()){
		urllist_type::iterator prev_url_iter = std::find_if(urllist->begin(), urllist->end(), boost::bind(find_url, url, _1));
		if (prev_url_iter != urllist->end()){
			// 有找到啊! 检查时间.
			jiange = current -  prev_url_iter->second;

			urllist->erase(prev_url_iter);
		}
	}
	urllist->push_back(std::make_pair(url, current));

	if (jiange.minutes() >= 5){
		// 超过5分钟了,  应该说了.
		// 把真正的工作交给真正的 urlpreview
		detail::urlpreview( io_service, m_sender, speaker, url);
	}
}
