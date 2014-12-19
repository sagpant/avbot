#pragma once
#include <avbotui.hpp>

class avbotcliui : public avbotui
{
public:
    avbotcliui(boost::asio::io_service&, boost::logger& logger, int argc, char* argv[]);

    virtual void show_vc_and_get_input_with_timeout(std::string imgdata, int timeout_sec, vc_result_call_back cb);

    virtual void report_fatal_error(std::string text);
};
