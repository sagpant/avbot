
#include "avbotui.hpp"

#include <boost/avloop.hpp>

avbotui::avbotui(boost::asio::io_service& io_service)
	: m_io_service(io_service)
{
}

avbotui::~avbotui()
{

}

void avbotui::append_log(int logleve, const char* msg)
{
}

void avbotui::report_fatal_error(std::string text)
{
	exit(1);
}

void avbotui::show_vc_and_get_input_with_timeout(std::string imgdata, int timeout_sec, vc_result_call_back cb)
{
	m_io_service.post(std::bind(cb, std::string()));
}

void avbotui::run()
{
	avloop_run_gui(m_io_service);
}

void avbotui::quit()
{
}
