/**
 * @file   main.cpp
 * @author microcai <microcaicai@gmail.com>
 * @origal_author mathslinux <riegamaths@gmail.com>
 *
 */

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/program_options.hpp>
namespace po = boost::program_options;
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/noncopyable.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>

#include "xmpp.h"

static std::string progname;

fs::path configfilepath()
{
	if( fs::exists( fs::path( progname ) / "qqbotrc" ) )
		return fs::path( progname ) / "qqbotrc";

	if( getenv( "USERPROFILE" ) ) {
		if( fs::exists( fs::path( getenv( "USERPROFILE" ) ) / ".qqbotrc" ) )
			return fs::path( getenv( "USERPROFILE" ) ) / ".qqbotrc";
	}

	if( getenv( "HOME" ) ) {
		if( fs::exists( fs::path( getenv( "HOME" ) ) / ".qqbotrc" ) )
			return fs::path( getenv( "HOME" ) ) / ".qqbotrc";
	}

	if( fs::exists( "./qqbotrc/.qqbotrc" ) )
		return fs::path( "./qqbotrc/.qqbotrc" );

	if( fs::exists( "/etc/qqbotrc" ) )
		return fs::path( "/etc/qqbotrc" );

	throw "not configfileexit";
}

int main( int argc, char *argv[] )
{
	std::string qqnumber, password;
	std::string ircnick, ircroom;
	std::string cfgfile;
	std::string logdir;
	std::string chanelmap;

	bool isdaemon = false;

	progname = fs::basename( argv[0] );

	std::setlocale( LC_ALL, "" );

	po::options_description desc( "qqbot options" );
	desc.add_options()
	( "version,v", "output version" )
	( "help,h", "produce help message" )
	( "user,u", po::value<std::string>( &qqnumber ), "QQ 号" )
	( "pwd,p", po::value<std::string>( &password ), "password" )
	( "logdir", po::value<std::string>( &logdir ), "dir for logfile" )
	( "daemon,d", po::value<bool>( &isdaemon ), "go to background" )
	( "nick", po::value<std::string>( &ircnick ), "irc nick" )
	( "room", po::value<std::string>( &ircroom ), "irc room" )
	( "map", po::value<std::string>( &chanelmap ), "map qqgroup to irc channel. eg: --map:qq:12345,irc:avplayer;qq:56789,irc:ubuntu-cn" )
	;

	po::variables_map vm;
	po::store( po::parse_command_line( argc, argv, desc ), vm );
	po::notify( vm );

	if( vm.count( "help" ) ) {
		std::cerr <<  desc <<  std::endl;
		return 1;
	}

	if( vm.size() == 0 ) {
		po::notify( vm );
	}

	if( vm.count( "version" ) ) {
		printf( "qqbot version %s \n", "0.1" );
	}


	boost::asio::io_service asio;
	boost::asio::io_service::work work( asio );

	xmpp xl( asio, "qqbot@linuxapp.org", "qqbot2012" );
	xl.join( "avplayer@im.linuxapp.org" );
	asio.run();
	return 0;
}
