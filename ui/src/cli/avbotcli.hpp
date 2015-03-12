
#pragma once

#include <memory>
#include <boost/thread.hpp>
#include <avbotui.hpp>

class QCoreApplication;
class avbotcliui : public avbotui
{
public:
    avbotcliui(boost::asio::io_service&, boost::logger& logger, int argc, char* argv[]);
	~avbotcliui();

    virtual void show_vc_and_get_input_with_timeout(std::string imgdata, int timeout_sec, vc_result_call_back cb);

    virtual void report_fatal_error(std::string text);

	virtual void run();

	QCoreApplication* app;
	boost::thread m_io_thread;
};
