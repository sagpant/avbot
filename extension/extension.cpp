
#include <boost/asio.hpp>
#ifdef HAVE_ZLIB
#include <zlib.h>
#else
#include "../third_party/qqwry/miniz.c"
#endif
#include "libavbot/avbot.hpp"
#include "extension.hpp"

#include "libavlog/avlog.hpp"

#ifdef ENABLE_LUA
#	include "luascript/luascript.hpp"
#endif

#include "urlpreview.hpp"
#include "joke.hpp"
#include "bulletin.hpp"
#include "metalprice.hpp"
#include "stockprice.hpp"
#include "exchangerate.hpp"
#include "iplocation.hpp"
#include "staticcontent.hpp"

#ifdef ENABLE_PYTHON
#include "pythonscriptengine.hpp"
#endif // ENABLE_PYTHON

#ifdef _WIN32
#include "dllextension.hpp"
#endif

// dummy file

extern avlog logfile;			// 用于记录日志文件.

static void sender(avbot & mybot, std::string channel_name, std::string txt, bool logtohtml)
{
	mybot.send_broadcast_message(channel_name, avbotmsg(txt));

	if (logtohtml){
		logfile.add_log(channel_name, avlog::html_escape(txt), 0);
	}
}

void new_channel_set_extension(avbot& mybot, avchannel& channel, std::string channel_name)
{
	auto & io_service = mybot.get_io_service();

	channel.handle_extra_message.connect(
		avbot_extension(
			channel_name,
			joke(
				io_service,
				io_service.wrap(std::bind(sender, std::ref(mybot), channel_name, std::placeholders::_1, 0)),
				channel_name,
				boost::posix_time::seconds(600)
			)
		)
	);

	channel.handle_extra_message.connect(
		avbot_extension(
			channel_name,
			urlpreview(
				io_service,
				io_service.wrap(std::bind(sender, std::ref(mybot), channel_name, std::placeholders::_1, 1))
			)
		)
	);
#ifdef ENABLE_LUA
	channel.handle_extra_message.connect(
		make_luascript(
			channel_name,
			io_service,
			io_service.wrap(std::bind(sender, std::ref(mybot), channel_name, std::placeholders::_1, 1))
		)
	);
#endif
	channel.handle_extra_message.connect(
		avbot_extension(
			channel_name,
			::bulletin(
				io_service,
				io_service.wrap(std::bind(sender, std::ref(mybot), channel_name, std::placeholders::_1, 1)),
				channel_name
			)
		)
	);
	channel.handle_extra_message.connect(
		avbot_extension(
			channel_name,
			make_metalprice(
				io_service,
				io_service.wrap(std::bind(sender, std::ref(mybot), channel_name, std::placeholders::_1, 1))
			)
		)
	);
	channel.handle_extra_message.connect(
		avbot_extension(
			channel_name,
			make_stockprice(
				io_service,
				io_service.wrap(std::bind(sender, std::ref(mybot), channel_name, std::placeholders::_1, 1))
			)
		)
	);
	channel.handle_extra_message.connect(
		avbot_extension(
			channel_name,
			::exchangerate(
				io_service,
				io_service.wrap(std::bind(sender, std::ref(mybot), channel_name, std::placeholders::_1, 1))
			)
		)
	);

	static std::shared_ptr<iplocationdetail::ipdb_mgr> ipdb_mgr;

	if (!ipdb_mgr)
	{
		// check for file "qqwry.dat"
		// if not exist, then download that file
		// after download that file, construct ipdb
		ipdb_mgr.reset(new  iplocationdetail::ipdb_mgr(mybot.get_io_service(), uncompress));
		ipdb_mgr->search_and_build_db();
	}

	channel.handle_extra_message.connect(
		avbot_extension(
			channel_name,
			make_iplocation(
				io_service,
				io_service.wrap(std::bind(sender, std::ref(mybot), channel_name, std::placeholders::_1, 1)),
				ipdb_mgr
			)
		)
	);

	channel.handle_extra_message.connect(
		make_static_content(io_service, channel_name)
	);

#ifdef ENABLE_PYTHON
	channel.handle_extra_message.connect(
		make_python_script_engine(
			io_service,
			channel_name,
			io_service.wrap(std::bind(sender, std::ref(mybot), channel_name, std::placeholders::_1, 1))
		)
	);
#endif

#ifdef _WIN32
	channel.handle_extra_message.connect(
		make_dllextention(
			io_service,
			channel_name,
			io_service.wrap(std::bind(sender, std::ref(mybot), channel_name, std::placeholders::_1, 1))
		)
	);
#endif
}
