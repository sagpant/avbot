#define ASIO_DISABLE_THREADS
#include "irc.hpp"
#include <string>
#include <iostream>

using namespace std;
using namespace irc;

// int n2u(const char *in, size_t in_len, char *out, size_t out_len)
// {
//     std::vector<wchar_t> wbuf(in_len+1);
//     int wlen = MultiByteToWideChar(CP_ACP, 0, in, (int)in_len, &wbuf[0], (int)in_len);
//     if( wlen < 0) return -1;
//     wbuf[wlen] = 0;
//
//     int len = WideCharToMultiByte(CP_UTF8, 0, &wbuf[0], (int)wlen, out, (int)out_len, NULL, FALSE);
//     if(len < 0)   return -1;
//     out[len] = 0;
//
//     return len;
// };


void my_cb( const irc_msg pMsg )
{
	std::cout <<  pMsg.msg << std::endl;
}

int
main( int argc, char **argv )
{
	boost::asio::io_service io_service;

	client irc_client( io_service, "testbot1234", "" );
	irc_client.on_privmsg_message( my_cb );
	irc_client.join( "#avplayer" );

	io_service.run();

	return 0;
}
