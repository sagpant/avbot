#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <fstream>

#include "pop3.hpp"
#include "boost/base64.hpp"

#include "fsconfig.ipp"

static void on_mail( mailcontent mail, mx::pop3::call_to_continue_function call_to_contiune )
{
	call_to_contiune( 0 );
}

int main( int argc, char * argv[] )
{
	std::string qqnumber, qqpwd;
	std::string ircnick, ircroom, ircpwd;
	std::string xmppuser, xmppserver, xmpppwd, xmpproom;
	std::string cfgfile;
	std::string logdir;
	std::string chanelmap;
	std::string mailaddr, mailpasswd, mailserver;

	setlocale( LC_ALL, "" );

	po::options_description desc( "qqbot options" );
	desc.add_options()
	( "version,v",										"output version" )
	( "help,h",											"produce help message" )
	( "daemon,d",										"go to background" )
	( "mail",		po::value<std::string>( &mailaddr ),	"fetch mail from this address" )
	( "mailpasswd",	po::value<std::string>( &mailpasswd ), "password of mail" )
	( "mailserver",	po::value<std::string>( &mailserver ), "password of mail" )
	;

	po::variables_map vm;
	po::store( po::parse_command_line( argc, argv, desc ), vm );
	po::notify( vm );

	if( vm.count( "help" ) ) {
		std::cerr <<  desc <<  std::endl;
		return 1;
	}

	if( vm.size() == 0 ) {
		try {
			fs::path p = configfilepath();
			po::store( po::parse_config_file<char>( p.string().c_str(), desc ), vm );
			po::notify( vm );
		} catch( char* e ) {
			std::cerr << e << std::endl;
		}
	}

	boost::asio::io_service asio;
	boost::asio::io_service::work work( asio );

	mx::pop3 p( asio, mailaddr, mailpasswd, mailserver );
	p.async_fetch_mail( on_mail );
	asio.run();
}
