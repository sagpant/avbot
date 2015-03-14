
/**
 * @file   main.cpp
 * @author microcai <microcaicai@gmail.com>
 *
 */

#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif
#ifdef _WIN32
#include <Windows.h>
#include <commctrl.h>
#include <mmsystem.h>
#endif
#include <string>
#include <algorithm>
#include <vector>
#include <signal.h>
#include <fstream>
#include <iterator>

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
namespace fs = boost::filesystem;
#include <boost/program_options.hpp>
namespace po = boost::program_options;
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/locale.hpp>
#include <boost/signals2.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/preprocessor.hpp>
#include <locale.h>
#include <cstring>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

#include <QtCore>

#include <soci-sqlite3.h>
#include <boost-optional.h>
#include <boost-tuple.h>
#include <boost-gregorian-date.h>
#include <soci.h>

#include <avhttp.hpp>

#include "boost/stringencodings.hpp"
#include "boost/avloop.hpp"
#include <boost/invoke_wrapper.hpp>

#include "libavbot/avbot.hpp"
#include "libavlog/avlog.hpp"

#include "botctl.hpp"
#include "input.hpp"
#include "rpc/server.hpp"

#include "extension/extension.hpp"
#include "deCAPTCHA/decaptcha.hpp"
#include "deCAPTCHA/deathbycaptcha_decoder.hpp"
#include "deCAPTCHA/anticaptcha_decoder.hpp"
#include "deCAPTCHA/avplayer_free_decoder.hpp"
#include "deCAPTCHA/jsdati_decoder.hpp"
#include "deCAPTCHA/hydati_decoder.hpp"
#include "ui_decoder.hpp"

#include "avbotui.hpp"

extern "C" void avbot_setup_seghandler();
extern "C" const char * avbot_version();
extern "C" const char * avbot_version_build_time();

extern pt::ptree parse_cfg(std::string filecontent);

char * execpath;
avlog logfile;			// 用于记录日志文件.

static std::string progname;
static bool need_vc = false;

std::string preamble_qq_fmt, preamble_irc_fmt, preamble_xmpp_fmt;

static void vc_code_decoded(boost::system::error_code ec, std::string provider,
	std::string vccode, boost::function<void()> reportbadvc, avbot & mybot, boost::logger& logger)
{
	set_do_vc();
	need_vc = false;

	// 关闭 settings 对话框 ，如果还没关闭的话

	if (ec)
	{
		printf("\r");
		fflush(stdout);
		logger.err() << literal_to_localstr("解码出错，重登录 ...");
		mybot.broadcast_message_to_all_channels("验证码有错，重新登录QQ");
// 	//	mybot.relogin_qq_account();
		return;
	}

	logger.info() << "使用" 	<< provider	<< " 成功解码验证码!";

	mybot.feed_login_verify_code(vccode, reportbadvc);
}

static void on_verify_code(std::string imgbuf, avbot & mybot, decaptcha::deCAPTCHA & decaptcha, boost::logger& logger)
{
	logger.info() << "got vercode from TX, now try to auto resovle it ... ...";

	need_vc = true;
	// 保存文件.
	std::ofstream img("vercode.jpeg",
		std::ofstream::openmode(std::ofstream::binary | std::ofstream::out)
	);

	img.write(&imgbuf[0], imgbuf.length());
	img.close();

	decaptcha.async_decaptcha(
		imgbuf,
		std::bind(&vc_code_decoded, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::ref(mybot), std::ref(logger))
	);
}

struct build_group_has_qq
{
	bool operator()(const std::string & str)
	{
		return str.substr(0, 3) == "qq:";
	}
};

std::string img_cacher(std::string cface)
{
	std::string ret;
	fs::path image_dir =  fs::path("images") / cface.substr(1,2);

	fs::path image_file = image_dir / cface;

	if (fs::exists(image_file) && fs::is_regular_file(image_file))
	{
		std::string imgfilename = image_file.string();

		auto reserved = fs::file_size(image_file);
		ret.resize(reserved);

		std::ifstream cfaceimg(imgfilename.c_str(), std::ofstream::binary|std::ofstream::in);
		cfaceimg.read(&ret[0], ret.size());
	}
	return ret;
}

static void img_saver(std::string cface, std::string data)
{
	fs::path image_dir =  fs::path("images") / cface.substr(1,2);

	if (!fs::exists(image_dir))
	{
		fs::create_directories(image_dir);
	}

	std::string imgfilename = (image_dir/cface).string();
	std::ofstream cfaceimg(imgfilename.c_str(), std::ofstream::binary|std::ofstream::out);
	cfaceimg.write(&data[0], data.size());
}

static std::string imgurlformater(std::string cface, std::string baseurl)
{
	return avhttp::detail::escape_path(
		boost::str(
			boost::format("%s/images/%s/%s")
			% baseurl
			% avbot::image_subdir_name(cface)
			% cface
		)
	);
}

static void avbot_log(std::string channel_name, channel_identifier id, avbotmsg message, soci::session & db)
{
	std::string linemessage;

	std::string curtime, protocol, nick;

	curtime = avlog::current_time();

	protocol = id.protocol;
	// 首先是根据 nick 格式化
	if ( protocol != "mail")
	{
		std::string textonly;
		linemessage += message.sender.preamble;

		for (avbotmsg_segment & msg_seg : message.msgs)
		{
			if (msg_seg.type  == "text")
			{
				textonly += boost::any_cast<std::string>(msg_seg.content);
				linemessage += avlog::html_escape(textonly);
			}
			else if (msg_seg.type == "url")
			{
				linemessage += boost::str(
					boost::format("<a href=\"%s\">%s</a>")
					% boost::any_cast<std::string>(msg_seg.content)
					% boost::any_cast<std::string>(msg_seg.content)
				);
			}
			else if (msg_seg.type == "image")
			{
				auto img_seg = boost::any_cast<avbotmsg_image_segment>(msg_seg.content);
				std::string cface = img_seg.cname;

				linemessage += boost::str(
					boost::format("<img src=\"../images/%s/%s\" />")
					% avbot::image_subdir_name(cface)
					% cface
				);
			}
			else if (msg_seg.type == "emoji")
			{
				auto emoji_seg = boost::any_cast<avbotmsg_emoji_segment>(msg_seg.content);
				linemessage += boost::str(
					boost::format("\t\t<img src=\"%s\" />\r\n")
					% emoji_seg.emoji_url
				);
			}
		}

		if(protocol != "rpc")
			nick = message.sender.nick;

		long rowid = 0;
		{
		// log to database
		db << "insert into avlog (date, protocol, channel, nick, message)"
			" values (:date, :protocol, :channel, :nick, :message)"
			, soci::use(curtime)
			, soci::use(protocol)
			, soci::use(channel_name)
			, soci::use(nick)
			, soci::use(textonly);
		}

		if (db.get_backend_name() == "sqlite3")
		rowid =  sqlite_api::sqlite3_last_insert_rowid(
			dynamic_cast<soci::sqlite3_session_backend*>(db.get_backend())->conn_
		);
		try{
		logfile.add_log(channel_name, linemessage, rowid);
		}catch(const std::runtime_error&)
		{}
	}
	else
	{
		// TODO ,  暂时不记录邮件吧！

// 		mybot.get_channel_name("room");
//
// 		linemessage  = boost::str(
// 			boost::format( "[QQ邮件]\n发件人:%s\n收件人:%s\n主题:%s\n\n%s" )
// 			% message.get<std::string>("from") % message.get<std::string>("to")
//			% message.get<std::string>("subject")
// 			% message.get_child("message").data()
// 		);
	}
}

static void init_database(soci::session & db)
{
	db.open(soci::sqlite3, "avlog.db");

	db <<
	"create table if not exists avlog ("
		"`date` TEXT not null, "
		"`protocol` TEXT not null default \"/\", "
		"`channel` TEXT not null, "
		"`nick` TEXT not null default \"\", "
		"`message` TEXT not null default \" \""
	");";
	db.close();
	db.open(soci::sqlite3, "avlog.db");
}

void sighandler(boost::asio::io_service & io)
{
	io.stop();
	std::cout << "Quitting..." << std::endl;
	ui_get_instance()->quit();
}

static fs::path get_home_dir();

static void setup_initail_channels(fs::path run_root, avbot& mybot, soci::session& avlogdb)
{
	for (auto subdir_it = fs::directory_iterator(run_root); subdir_it != fs::directory_iterator(); ++subdir_it)
	{
		fs::path subdirs = * subdir_it;
		if (fs::is_directory(subdirs) && (!fs::is_symlink(subdirs)))
		{
			// 检查 [channelname]/map.txt
			if (fs::exists(subdirs / "map.txt"))
			{
				std::cout << "processing: " << subdirs << std::endl;
				// 好, 配置 avchannel !

				std::string channel_name = subdirs.filename().string();

				auto channel = std::make_shared<avchannel>(channel_name);

				// 读取 map.txt 文件
				std::ifstream map((subdirs / "map.txt").string().c_str());

				for (; !map.eof();)
				{
					std::string mapline;
					std::getline(map, mapline);
					std::vector<std::string> tokens;

					boost::split(tokens, mapline, boost::is_any_of(":"));

					if( tokens.size() ==2)
					{
						std::cout << "  add room " << mapline << std::endl;
						channel->add_room(tokens[0], tokens[1]);
                    }
                    else
						break;
                }

				mybot.add_channel(channel_name, channel);

				// 记录到日志.
				channel->handle_extra_message.connect(
					std::bind(avbot_log, channel_name, std::placeholders::_1, std::placeholders::_2, std::ref(avlogdb))
				);

				// 开启 extension
				new_channel_set_extension(mybot, *channel, channel_name);

				// 开启 bot 控制.
				channel->handle_extra_message.connect(
					std::bind(on_bot_command, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::ref(mybot), std::ref(*channel))
				);
			}
		}
	}
}


int main(int argc, char * argv[])
{
#ifdef _WIN32
	::InitCommonControls();
#endif
	Q_INIT_RESOURCE(avbot);

	boost::asio::io_service io_service;
	boost::logger mylog;

	ui_init(io_service, mylog, 1, argv);

	std::string jsdati_username, jsdati_password;
	std::string hydati_key;
	std::string deathbycaptcha_username, deathbycaptcha_password;
	//http://api.dbcapi.me/in.php
	//http://antigate.com/in.php
	std::string antigate_key, antigate_host;
	bool use_avplayer_free_vercode_decoder(false);
	bool no_persistent_db(false);
	std::string weblogbaseurl;

	fs::path run_root; // 运行时的根
	fs::path account_settings;

	unsigned rpcport;

	avbot mybot(io_service, mylog);

	progname = fs::basename(argv[0]);

#ifdef _WIN32
	setlocale(LC_ALL, "zh");
#else
	setlocale(LC_ALL, "");
#endif

	po::variables_map vm;
	po::options_description desc("qqbot options");
	desc.add_options()
	("version,v", 	"output version")
	("help,h", 	"produce help message")
	("daemon,d", 	"go to background")
	("nostdin", 	"don't read from stdin (systemd daemon mode)")
#if defined(WIN32) || defined(WITH_QT_GUI)
	("gui,g",	 	"pop up settings dialog")
#endif

	("config,c", po::value<fs::path>(&account_settings)->default_value("/etc/avbot.conf"),
		"path to account file")
	("logdir,d", po::value<fs::path>(&run_root)->default_value("/run/avbot"),
		"path to logs")
	("nopersistent,s", po::value<bool>(&no_persistent_db),
		"do not use persistent database file to increase security.")

	("jsdati_username", po::value<std::string>(&jsdati_username),
		literal_to_localstr("联众打码服务账户").c_str())
	("jsdati_password", po::value<std::string>(&jsdati_password),
		literal_to_localstr("联众打码服务密码").c_str())

	("hydati_key", po::value<std::string>(&hydati_key),
		literal_to_localstr("慧眼答题服务key").c_str())

	("deathbycaptcha_username", po::value<std::string>(&deathbycaptcha_username),
		literal_to_localstr("阿三解码服务账户").c_str())
	("deathbycaptcha_password", po::value<std::string>(&deathbycaptcha_password),
		literal_to_localstr("阿三解码服务密码").c_str())

	("antigate_key", po::value<std::string>(&antigate_key),
		literal_to_localstr("antigate解码服务key").c_str())
	("antigate_host", po::value<std::string>(&antigate_host)->default_value("http://antigate.com/"),
		literal_to_localstr("antigate解码服务器地址").c_str())

	("use_avplayer_free_vercode_decoder", po::value<bool>(&use_avplayer_free_vercode_decoder),
		"ask microcai for permission")

	("weblogbaseurl", po::value<std::string>(&(weblogbaseurl)),
		"base url for weblog serving")
	("rpcport",	po::value<unsigned>(&rpcport)->default_value(6176),
		"run rpc server on port 6176")

	("preambleqq", po::value<std::string>(&preamble_qq_fmt)->default_value(literal_to_localstr("qq(%a): ")),
		literal_to_localstr("为QQ设置的发言前缀, 默认是 qq(%a): ").c_str())
	("preambleirc", po::value<std::string>(&preamble_irc_fmt)->default_value(literal_to_localstr("%a 说: ")),
		literal_to_localstr("为IRC设置的发言前缀, 默认是 %a 说: ").c_str())
	("preamblexmpp", po::value<std::string>(&preamble_xmpp_fmt)->default_value(literal_to_localstr("(%a): ")),
		literal_to_localstr(
			"为XMPP设置的发言前缀, 默认是 (%a): \n\n "
			"前缀里的含义 \n"
			"\t %a 为自动选择\n\t %q 为QQ号码\n\t %n 为昵称\n\t %c 为群名片 \n"
			"\t %r为房间名(群号, XMPP房名, IRC频道名) \n"
			"可以包含多个, 例如想记录QQ号码的可以使用 qq(%a, %q)说: \n"
			"注意在shell下可能需要使用\\(来转义(\n配置文件无此问题 \n\n").c_str())
	;

	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		std::cerr <<  desc <<  std::endl;
		return 1;
	}

	if (vm.count("version"))
	{
		printf("qqbot version %s (%s %s) \n", avbot_version() , __DATE__, __TIME__);
		exit(EXIT_SUCCESS);
	}

#ifdef WIN32
	// 从 windows 控制台输入的能有啥好编码，转到utf8吧.
	preamble_qq_fmt = ansi_utf8(preamble_qq_fmt);
	preamble_irc_fmt = ansi_utf8(preamble_irc_fmt);
	preamble_xmpp_fmt = ansi_utf8(preamble_xmpp_fmt);
#endif

#ifndef _WIN32
	// 设置到中国的时区，否则 qq 消息时间不对啊.
	setenv("TZ", "Asia/Shanghai", 1);
# endif

	if (vm.count("config") == 1 && account_settings == "/etc/avbot.conf")
	{
		// 查找合适的配置文件. 优先选择 当前目录, 然后是 $HOME/.avbot.conf 最后是 /etc/avbot.conf
		// 都没有的话, 还是使用 avbot.conf
		if (fs::exists("avbot.conf"))
		{
			account_settings = fs::current_path() / "avbot.conf";
		}
		else if (fs::exists( get_home_dir() / ".avbot.conf" ))
		{
			account_settings = get_home_dir() / ".avbot.conf" ;
		}
		else if (fs::exists("/etc/avbot.conf"))
			account_settings = "/etc/avbot.conf";
		else
		{
			account_settings = fs::current_path() / "avbot.conf";
		}
	}

	{
#ifndef _WIN32
		std::string env_oldpwd = fs::complete(fs::current_path()).string();
		setenv("O_PWD", env_oldpwd.c_str(), 1);
# else
		std::string env_oldpwd = std::string("O_PWD=") + fs::complete(fs::current_path()).string();
		putenv((char *)env_oldpwd.c_str());
#endif

	}

	if (!fs::exists(account_settings))
	{
		// 提示用户, 木有配置文件
		report_fatal_error("木配置文件");
	}

	// 解析 accounts 文件, 设置帐号
	pt::ptree accounts_settings;
	{
		std::ifstream accounts_file_stream;
		std::string accounts_file_content;
		accounts_file_content.resize(fs::file_size(account_settings));
		accounts_file_stream.open(account_settings.string().c_str(), std::ifstream::in);
		accounts_file_stream.read(&accounts_file_content[0], accounts_file_content.size());
		// 去掉 BOM ef bb bf
		if (accounts_file_content[0] == '\357' && accounts_file_content[1]=='\273' && accounts_file_content[2]=='\277')
		{
			accounts_file_content = accounts_file_content.substr(3);
		}
		accounts_settings = parse_cfg(accounts_file_content);
	}

	{
		auto globle_settings = accounts_settings.get_child("global");

		run_root = globle_settings.get<std::string>("logdir");

		if (!globle_settings.get<std::string>("antigate_key","").empty())
			antigate_key = globle_settings.get<std::string>("antigate_key");

		if (!globle_settings.get<std::string>("antigate_host","").empty())
			antigate_host = globle_settings.get<std::string>("antigate_host");

		if (!globle_settings.get<std::string>("weblogbaseurl","").empty())
			weblogbaseurl = globle_settings.get<std::string>("weblogbaseurl");
	}

	// 设置日志自动记录目录.
	if (!run_root.empty())
	{
		logfile.log_path(run_root.string());
#ifdef _WIN32
		SetCurrentDirectoryW(avhttp::detail::utf8_wide(run_root.string()).c_str());
#else
		chdir(run_root.c_str());
#endif // _WIN32
	}

	soci::session avlogdb;

	init_database(avlogdb);

	decaptcha::deCAPTCHA decaptcha_agent(io_service);

	if (!hydati_key.empty())
	{
		decaptcha_agent.add_decoder(
			decaptcha::decoder::hydati_decoder(
				io_service, hydati_key
			)
		);
	}

	if (!jsdati_username.empty() && !jsdati_password.empty())
	{
		decaptcha_agent.add_decoder(
			decaptcha::decoder::jsdati_decoder(
				io_service, jsdati_username, jsdati_password
			)
		);
	}

	if (!deathbycaptcha_username.empty() && !deathbycaptcha_password.empty())
	{
		decaptcha_agent.add_decoder(
			decaptcha::decoder::deathbycaptcha_decoder(
				io_service, deathbycaptcha_username, deathbycaptcha_password
			)
		);
	}

	if (!antigate_key.empty())
	{
		decaptcha_agent.add_decoder(
			decaptcha::decoder::anticaptcha_decoder(io_service, antigate_key, antigate_host)
		);
	}

	if (use_avplayer_free_vercode_decoder)
	{
		decaptcha_agent.add_decoder(
			decaptcha::decoder::avplayer_free_decoder(io_service)
		);
	}

	decaptcha_agent.add_decoder(
		decoder::ui_decoder(io_service)
	);

	mybot.preamble_irc_fmt = preamble_irc_fmt;
	mybot.preamble_qq_fmt = preamble_qq_fmt;
	mybot.preamble_xmpp_fmt = preamble_xmpp_fmt;

	// 解析 accounts_settings 然后设置帐号
 	for (pt::ptree::value_type account: accounts_settings)
	{
		try{
			if (account.first == "qq")
			{
				auto qqnumber = account.second.get<std::string>("qqnumber");
				auto qqpassword = account.second.get<std::string>("password");
				// 调用 add_qq_account
				mybot.add_qq_account(qqnumber, qqpassword,
					std::bind(on_verify_code, std::placeholders::_1, std::ref(mybot), std::ref(decaptcha_agent), std::ref(mylog)),
					no_persistent_db);
			}
			else if (account.first == "irc")
			{
				auto nick = account.second.get<std::string>("nick");
				auto password = account.second.get<std::string>("password","");

				auto irc_client = mybot.add_irc_account(nick, password);

				auto rooms = account.second.get_child("rooms");
				for (pt::ptree::value_type room : rooms)
				{
					std::string room_name = room.second.data();
					irc_client->join(room_name);
				}
			}
			else if (account.first == "xmpp")
			{
				auto nick = account.second.get<std::string>("name");
				auto password = account.second.get<std::string>("password");

				auto xmpp_client = mybot.add_xmpp_account(nick, password);

				auto rooms = account.second.get_child("rooms");
				for (pt::ptree::value_type room : rooms)
				{
					std::string room_name = room.first;
					xmpp_client->join(room_name);
				}
			}
			else if (account.first == "avim")
			{
				auto key = account.second.get<std::string>("keyfile");
				auto cert = account.second.get<std::string>("certfile");
				auto deffile = account.second.get<std::string>("groupdef");

				auto avim_client = mybot.add_avim_account(key, cert, deffile);

			}
		}catch(const boost::property_tree::ptree_error&)
		{}
	}

	// 遍历文件夹, 设置 channel
	setup_initail_channels(run_root, mybot, avlogdb);

	boost::asio::io_service::work work(io_service);

	if (!weblogbaseurl.empty())
	{
		mybot.m_urlformater = std::bind(&imgurlformater, std::placeholders::_1, weblogbaseurl);
	}

	mybot.m_image_saver = img_saver;
	mybot.m_image_cacher = img_cacher;

	if (rpcport > 0)
	{
		if (!avbot_start_rpc(io_service, mylog, rpcport, mybot, avlogdb))
		{
			mylog.warn() <<  "bind to port " <<  rpcport <<  " failed!";
			mylog.warn() <<  "Did you happened to already run an avbot? ";
			mylog.warn() <<  "Now avbot will run without RPC support. ";
		};
	}

	avhttp::http_stream s(io_service);
	s.async_open(
		"https://avlog.avplayer.org/cache/tj.php",
		boost::bind(&avhttp::http_stream::close, &s)
	);

	boost::asio::signal_set terminator_signal(io_service);
	terminator_signal.add(SIGINT);
	terminator_signal.add(SIGTERM);
#if defined(SIGQUIT)
	terminator_signal.add(SIGQUIT);
#endif // defined(SIGQUIT)
	terminator_signal.async_wait(boost::bind(&sighandler, boost::ref(io_service)));

#ifdef HAVE_SYSTEMD
	// watchdog timer logic

	// check WATCHDOG_USEC environment variable
	uint64_t watchdog_usec;
	if(sd_watchdog_enabled(1, &watchdog_usec))
	{
		mybot.on_message.connect([](channel_identifier, avbotmsg)
		{
			sd_notify(0, "WATCHDOG=1");
		});
	};

	avloop_idle_post(io_service, std::bind(&sd_notify,0, "READY=1"));
#endif
	avbot_setup_seghandler();
	ui_run_loop();
	return 0;
}

static fs::path get_home_dir()
{
#ifdef _WIN32
	if (getenv("USERPROFILE"))
	{
		return getenv("USERPROFILE");
	}
	else
	{
		return "C:/";
	}
#else
	if (getenv("HOME"))
	{
		return getenv("HOME");
	}
	else
	{
		return "/run";
	}
#endif
}
