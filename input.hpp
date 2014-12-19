
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>

void start_stdinput(boost::asio::io_service &);
void connect_stdinput(boost::function<void(std::string)> cb);
