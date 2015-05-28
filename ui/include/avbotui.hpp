
#pragma once

#include <memory>
#include <functional>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <boost/logger.hpp>
#include <boost/config.hpp>

#ifdef _WIN32
#include <windows.h>
#endif


/*
 * 创建一个对话框并以 imagedata 指示的数据显示验证码
 * 返回一个用来撤销他的闭包。
 */
std::function<void()> async_input_box_get_input_with_image(boost::asio::io_service & io_service, std::string imagedata, std::function<void(boost::system::error_code, std::string)> donecallback);

/**
 * 弹出窗口警告用户, 或者只是一个控制台消息
 */
void warn_user(std::string text);

// 创建一个该对象以使用 UI 功能. UI 包括基于 Qt的 GUI, ncurses 的 CLI 和 win32 GUI
// 还有一个空 UI 也就是没 UI
class avbotui : boost::noncopyable
{
public:
	avbotui(boost::asio::io_service& io_service);
	virtual ~avbotui();
public:
	virtual void append_log(int logleve, const char* msg);

	typedef std::function<void(std::string)> vc_result_call_back;

	// when timeout, will be empty string
	virtual void show_vc_and_get_input_with_timeout(std::string imgdata, int timeout_sec, avbotui::vc_result_call_back cb);

	// 用指定的 text 报告终极错误, 这个函数不会返回.
	virtual void report_fatal_error(std::string text);

	virtual void run();

	virtual void quit();

	boost::asio::io_service& m_io_service;
};

std::shared_ptr<avbotui> ui_get_instance();

// UI 初始化失败会直接退出程序.
void ui_init(boost::asio::io_service& io_service, boost::logger& logger, int& argc, char* argv[]);

inline void report_fatal_error(std::string text)
{
	ui_get_instance()->report_fatal_error(text);
}

template<class Handler>
void show_vc_and_get_input_with_timeout(std::string imgdata, int timeout_sec, Handler cb)
{
	ui_get_instance()->show_vc_and_get_input_with_timeout(imgdata, timeout_sec, cb);
}

inline void ui_run_loop()
{
	ui_get_instance()->run();
}
