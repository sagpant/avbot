
#include "avbotui.hpp"
#if defined(WITH_QT_GUI)
#include "./src/qt-gui/qtgui.hpp"
#elif defined(WITH_WIN32_GUI)
#include "./src/wiin32-gui/avbotwin32ui.hpp"
#else
#include "./src/cli/avbotcli.hpp"
#endif

static std::shared_ptr<avbotui> static_avbot_ui_instance;

void ui_init(boost::asio::io_service& io_service, boost::logger& logger, int argc, char* argv[])
{
	// 好了, 根据需要返回

#if defined(WITH_QT_GUI)
	static_avbot_ui_instance.reset(new avbotqtui(io_service, logger, argc, argv));
#elif defined(WITH_WIN32_GUI)
	static_avbot_ui_instance.reset(new avbotwin32ui(io_service, logger, argc, argv));
#else
	static_avbot_ui_instance.reset(new avbotcliui(io_service, logger, argc, argv));
#endif
}

std::shared_ptr<avbotui> ui_get_instance()
{
	return static_avbot_ui_instance;
}

