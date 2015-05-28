#include "qtgui.hpp"
#include "avbotqtui_impl.hpp"

avbotqtui::~avbotqtui()
{
	delete impl;
}

avbotqtui::avbotqtui(boost::asio::io_service& io_service, boost::logger& logger, int& argc, char* argv[])
	: avbotui(io_service)
{
	app = new QApplication(argc, argv);
	app->setQuitOnLastWindowClosed(false);
	impl = new avbotqtui_impl(io_service, logger, argc, argv);
}

void avbotqtui::report_fatal_error(std::string text)
{
	impl->report_fatal_error(text);
}

void avbotqtui::run()
{
	impl->run();
}

void avbotqtui::show_vc_and_get_input_with_timeout(std::string imgdata, int timeout_sec, avbotui::vc_result_call_back cb)
{
	impl->show_vc_and_get_input_with_timeout(imgdata, timeout_sec, cb);
}

void avbotqtui::quit()
{
    impl->quit();
}

#ifdef _WIN32

#ifdef STATIC_QT5
#include <QtPlugin>
#include <windows.h>

Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);

Q_IMPORT_PLUGIN(QICOPlugin);

#ifdef _DEBUG
#pragma comment(lib, "Qt5PlatformSupportd.lib")
#pragma comment(lib, "qwindowsd.lib")
#pragma comment(lib, "qicod.lib")
#else
#pragma comment(lib, "Qt5PlatformSupport.lib")
#pragma comment(lib, "qwindows.lib")
#pragma comment(lib, "qico.lib")
#endif

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "Imm32.lib")
#pragma comment(lib, "winmm.lib")
#endif
#endif
