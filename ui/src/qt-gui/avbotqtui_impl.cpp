
#include <QString>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QScreen>

#include "qtgui.hpp"
#include "avbotqtui_impl.hpp"
#include "vc_input_dialog.hpp"

void load_qrc(){Q_INIT_RESOURCE(avbot);}

#ifdef _WIN32
#include "resource.h"

extern QPixmap qt_pixmapFromWinHICON(HICON);

static QIcon load_win32_icon()
{
	QIcon ico;
	HICON hicon = (HICON)::LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
	ico = QIcon(qt_pixmapFromWinHICON(hicon));
	::DestroyIcon(hicon);
	return ico;
}
#endif

static QIcon load_icon()
{
#ifdef _WIN32
	return load_win32_icon();
#else
	return QIcon(":/avbot/avbot.svg");
#endif
}

avbotqtui_impl::avbotqtui_impl(boost::asio::io_service& io_service, boost::logger& _logger, int argc, char* argv[])
    : m_io_service(io_service)
    , logger(_logger)
{
    qRegisterMetaType<std::function<void()>>("std::function<void()>");
    qRegisterMetaType<std::string>("std::string");
    QObject::connect(this, &avbotqtui_impl::post_event, this, &avbotqtui_impl::execute_post, Qt::QueuedConnection);

	qApp->setWindowIcon(load_icon());
    // 创建 IO 线程
    m_io_thread = boost::thread([this]()
	{
		boost::asio::io_service::work work(m_io_service);
		avloop_run_gui(m_io_service);
	});

	auto mainwindow = new QMainWindow;

	mainwindow->setAttribute(Qt::WA_DeleteOnClose);

	auto log_browser = new QTextBrowser;

	mainwindow->setCentralWidget(log_browser);

	mainwindow->resize(qApp->primaryScreen()->availableSize() / 3);

	mainwindow->show();

	// 把日志输出给转到 qt 界面来
	logger.write_log.connect(std::bind(&avbotqtui_impl::write_log, this, std::placeholders::_1, std::placeholders::_2));


	connect(this, &avbotqtui_impl::append_log, mainwindow, [log_browser](std::string l, std::string m)
	{
		log_browser->append(QString::fromStdString(m));
	}, Qt::QueuedConnection);
}

void avbotqtui_impl::show_vc_and_get_input_with_timeout(std::string imgdata, int timeout_sec, avbotui::vc_result_call_back cb)
{
	ui_post([this, imgdata, timeout_sec, cb]()
	{
		auto input = new vc_input_dialog(this, imgdata, timeout_sec, cb);
		input->show();
	});
}

avbotqtui_impl::~avbotqtui_impl()
{
    // 发送退出信号, 然后等待正常退出
    if (!m_io_service.stopped())
        m_io_service.stop();
    m_io_thread.join();
}

void avbotqtui_impl::run()
{
	ui_post([this](){
		m_tray.reset(new QSystemTrayIcon(load_icon()));
		m_tray->setToolTip("avbot is running");
		m_tray->show();
	});
    qApp->exec();
}

void avbotqtui_impl::write_log(std::string l, std::string m)
{
	append_log(l,m);
	// 将 log 通知到 GUI 上.

	if(m_tray)
	{
		ui_post([this, l, m]()
		{
		//	m_tray->setToolTip(QString::fromStdString(m));
			if (l == boost::LOGGER_WARN_STR)
				m_tray->showMessage(QStringLiteral("avbot警告"), QString::fromStdString(m), QSystemTrayIcon::Warning);
			else if (l == boost::LOGGER_ERR_STR)
			{
				m_tray->showMessage(QStringLiteral("错误"), QString::fromStdString(m), QSystemTrayIcon::Critical);
			}else if (l == boost::LOGGER_INFO_STR)
			{
				m_tray->showMessage(QStringLiteral("info"), QString::fromStdString(m), QSystemTrayIcon::Information);
			}
		});
	}
}
