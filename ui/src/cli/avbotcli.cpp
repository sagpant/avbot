
#include <boost/thread/future.hpp>
#include <fstream>
#include "avbotcli.hpp"

avbotcliui::avbotcliui(boost::asio::io_service& io_service, boost::logger& logger, int argc, char* argv[])
	: avbotui(io_service)
{

}

std::string timedout_cin(int timeout_sec)
{
	// 如何实现超时的输入?
	std::string line;

	std::cin >> line;

	return line;
}

void avbotcliui::show_vc_and_get_input_with_timeout(std::string imgdata, int timeout_sec, avbotui::vc_result_call_back cb)
{
    // 在控制台输出提示.
	std::cout << "请查看log目录下的vercode.jpeg 然后输入验证码:" ;
	std::cout.flush();

	std::ofstream vercode("vercode.jpeg", std::ofstream::binary);
	vercode.write(imgdata.c_str(), imgdata.size());
	vercode.close();

	boost::async(boost::launch::async, [timeout_sec, cb]()
	{
		cb(timedout_cin(timeout_sec));
	});
}

void avbotcliui::report_fatal_error(std::string text)
{
    std::cerr  << text << std::endl;
	exit(1);
}
