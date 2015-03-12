#pragma once

#include <string>
#include <boost/logger.hpp>
#include "avbotui.hpp"

class QApplication;
class avbotqtui_impl;
class avbotqtui : public avbotui
{
public:
	avbotqtui(boost::asio::io_service& io_service, boost::logger& logger, int argc, char* argv[]);
	~avbotqtui();

    virtual void run();
    virtual void report_fatal_error(std::string text);
	virtual void show_vc_and_get_input_with_timeout(std::string imgdata, int timeout_sec, avbotui::vc_result_call_back cb);
    virtual void quit();

	avbotqtui_impl* impl;
	std::shared_ptr<QApplication> app;
};
