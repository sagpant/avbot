
#include <ctime>
#include <boost/date_time.hpp>
#include "boost/date_time/c_local_time_adjustor.hpp"
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/timedcall.hpp>

#include "bulletin.hpp"
#include <libavlog/avlog.hpp>

void bulletin::load_settings()
{
	boost::filesystem::path settingsfile = boost::filesystem::current_path() / m_channel_name / "bulletin_setting" ;

	m_settings->clear();

	if( boost::filesystem::exists( settingsfile ) )
	{
		std::ifstream bulletin_setting( settingsfile.string().c_str() );

		std::string line;

		while( std::getline( bulletin_setting, line ), !bulletin_setting.eof() )
		{
			m_settings->push_back( line );
		}

	}
}

static bool is_match(std::string cronline_field, int field)
{
	// 计划通,  呵呵,  检查 "*"
	if (cronline_field == "*")
		return true;
	return (boost::lexical_cast<int>(cronline_field) == field);
}

// 在这里调度一下下一次启动.
void bulletin::schedule_next() const
{
	// 获取当前时间,  以 分为单位步进
	// 直到获得一个能满足要求的时间值
	// 或则超过 24h
	// 如果这样就直接设定一天以后再重新跑
	// 避免 cpu 无限下去.
	boost::posix_time::ptime now =  boost::posix_time::second_clock::local_time();

	boost::posix_time::ptime end = now  + boost::posix_time::hours(2);

	// 比对 年月日.
	boost::posix_time::ptime::date_type date = now.date();

	boost::regex ex1("([0-9\\*]+)\\-([0-9\\*]+)\\-([0-9\\*]+)\\-([0-9\\*]+)\\-([0-9\\*]+)[ \t]+(.*)");
	boost::regex ex2("([0-9\\*]+)\\-([0-9\\*]+)\\-([0-9\\*]+)\\-([0-9\\*]+)\\-([0-9\\*]+)");
	boost::cmatch what;

	// 以一分钟为步进.
	for (	boost::posix_time::time_iterator titr(now + boost::posix_time::seconds(45), boost::posix_time::minutes(1));
			titr < end;
			++ titr )
	{
		BOOST_FOREACH(std::string cronline, *m_settings)
		{
			// 使用 regex 提取 *-*-*-*-* 的各个数字.
			if (boost::regex_match(cronline.c_str(), what, ex1))
			{
				// 然后为各个 filed 执行比较吧.
				if( is_match( what[1], titr->date().year())
					&& is_match( what[2], titr->date().month())
					&& is_match( what[3], titr->date().day())
					&& is_match( what[4], titr->time_of_day().hours())
					&& is_match( what[5], titr->time_of_day().minutes())
				)
				{
					// 设定 expires
					m_timer->expires_from_now(*titr - now + boost::posix_time::seconds( std::rand() % 30 +15 ));
					m_timer->async_wait(boost::bind<void>(*this, _1, std::string(what[6])));
					return;
				}
			}else if (boost::regex_match(cronline.c_str(), what, ex2)){
				if( is_match( what[1], titr->date().year())
					&& is_match( what[2], titr->date().month())
					&& is_match( what[3], titr->date().day())
					&& is_match( what[4], titr->time_of_day().hours())
					&& is_match( what[5], titr->time_of_day().minutes())
				)
				{
					// 设定 expires
					m_timer->expires_from_now(*titr - now + boost::posix_time::seconds( std::rand() % 30 + 15 ));
					m_timer->async_wait(boost::bind<void>(*this, _1, std::string("bulletin.txt")));
					return;
				}
			}
		}
	}

	//

	// 设定下次的 expires

	// 设定 expires
	m_timer->expires_from_now(boost::posix_time::hours(1));
	m_timer->async_wait(*this);
	return;
}

void bulletin::operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context) const
{
	// 其实主要是为了响应 .qqbot bulletin 命令.
}

void bulletin::operator()( boost::system::error_code ec, std::string msgfile )
{
	if (ec)
		return;
	load_settings();
	schedule_next();

	// 打开 bulletin 文件然后发送文件内容.
	// 就是这么回事.
	send_msg_file(msgfile);
}

void bulletin::operator()( boost::system::error_code)
{
	load_settings();
	schedule_next();
}

void bulletin::send_msg_file( std::string msgfile ) const
{
	std::string bulletinmsg;
	fs::path msgfilepath =  fs::current_path() / m_channel_name / msgfile ;

	try
	{
		boost::uintmax_t fsize = fs::file_size(msgfilepath);
		std::ifstream msgstream( msgfilepath.string().c_str() );
		bulletinmsg.resize(fsize);
		msgstream.read(&bulletinmsg[0], fsize);
	}
	catch( std::runtime_error )
	{
		// 文件无法打开 ...
		bulletinmsg = "无法打开 [logdir]/";
		bulletinmsg += "/";
		bulletinmsg += m_channel_name;
		bulletinmsg += "/";
		bulletinmsg += msgfile;
		bulletinmsg += " 请检查文件是否存在.";
	}

	m_sender(bulletinmsg);
}
