
extern "C" {
#include <luajit-2.0/luajit.h>
#include <luajit-2.0/lualib.h>
#include <luajit-2.0/lauxlib.h>
}

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/property_tree/json_parser.hpp>

#include "luabind/object.hpp"
#include "luabind/luabind.hpp"
#include "luabind/tag_function.hpp"
#include "luabind/function.hpp"

#include "luascript.hpp"

#include "boost/json_parser_write.hpp"
#include "boost/stringencodings.hpp"

#include <setjmp.h>

#ifdef _WIN32
#include <excpt.h>
#include <winerror.h>
#ifndef FACILITY_VISUALCPP
#define FACILITY_VISUALCPP  ((LONG)0x6d)
#endif
#include <delayimp.h>
#endif // _WIN32


struct lua_sender
{
	boost::function<void ( std::string ) > m_sender;

	template<class F>
	lua_sender( F f ): m_sender( f ) {}

	void operator()( const char * str ) const
	{
		m_sender( str );
	}
};

callluascript::callluascript(boost::asio::io_service &_io_service, std::string channel_name, std::function<void(std::string)> sender)
	: io_service(_io_service)
	, m_sender(sender)
	, channel_name_(channel_name)
{
}

callluascript::~callluascript()
{

}

void callluascript::load_lua() const
{
	fs::path luafile;
	char *old_pwd = getenv( "O_PWD" );

	if( old_pwd )
	{
		luafile = fs::path( old_pwd ) /  "main.lua" ;
	}
	else
	{
		luafile = fs::current_path() / "main.lua" ;
	}

	// 实时载入, 修改后就马上生效!
	if( fs::exists( luafile ) )
	{
		lua_State* L = luaL_newstate();
		m_lua_State.reset( L, lua_close );
		luaL_openlibs( L );

		luabind::open( L );

		// 准备调用 LUA 脚本.
		luabind::module( L )[
			luabind::def( "send_channel_message", luabind::tag_function<void( const char * )>( lua_sender( m_sender ) ) )
		];

		luaL_dofile( L, luafile.string().c_str() );
	}
}

void callluascript::call_lua( std::string jsondata ) const
{
	lua_State* L = m_lua_State.get();

	if( L )
	{
		try
		{
			luabind::call_function<void>( L, "channel_message", jsondata);
		}
		catch(luabind::error e)
		{
			std::cout << "\t" << e.what() << std::endl;
			std::cout<< lua_tostring(L, -1);
		}
	}
}

void callluascript::operator()(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context) const
{
	load_lua();
	std::stringstream jsondata;
	auto json_msg = av_msg_make_json(cid, msg);
	json_msg.put("channel", channel_name_);
	boost::property_tree::json_parser::write_json(jsondata, json_msg);

	call_lua(jsondata.str());
}


#ifdef _MSC_VER

LONG WINAPI DelayLoadExceptionFilter(DWORD exceptioncode)
{
	switch (exceptioncode)
	{
	case  VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND):
		return EXCEPTION_EXECUTE_HANDLER;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}


// 先测试 lua51.dll 的存在性，如果不存在，就别继续啦！
static bool test_lua51_dll()
{
	__try
	{
		LUAJIT_VERSION_SYM();
		return true;
	}
	__except (DelayLoadExceptionFilter(GetExceptionCode()))
	{
		return false;
	}
	return false;
}

#elif defined(_WIN32)

// 别的平台没有延迟加载技术
static bool test_lua51_dll()
{
	std::shared_ptr<void> res(
		LoadLibraryW(L"lua51.dll"),
		FreeLibrary
	);

	if (res.get() != NULL)
	{
		LUAJIT_VERSION_SYM();
		return true;
	}
	return false;
}

#else

static bool test_lua51_dll()
{
	return true;
}

#endif // _MSC_VER

static void dumy_func(channel_identifier cid, avbotmsg msg, send_avchannel_message_t sender, boost::asio::yield_context yield_context)
{
}

avbot_extension make_luascript(std::string channel_name, boost::asio::io_service &_io_service, std::function<void(std::string)> sender)
{
	if (test_lua51_dll())
	{
		std::cerr << literal_to_localstr("lua51.dll 找到啦！！ 脚本功能开启！") << std::endl;
		return avbot_extension(
			channel_name,
			callluascript(_io_service, channel_name, sender)
		);
	}
	else
	{
		std::cerr << literal_to_localstr("lua51.dll 加载失败，lua 脚本功能被禁止！！！") << std::endl;
		std::cerr << literal_to_localstr("如果希望使用lua脚本功能，请将 lua51.dll 和 avbot 放置于同一目录！") << std::endl;

		return avbot_extension(
			channel_name,
			&dumy_func
		);
	}
}
