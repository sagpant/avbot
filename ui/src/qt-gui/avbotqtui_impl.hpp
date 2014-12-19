
#pragma once

#include <boost/thread.hpp>
#include <QThread>
#include <QMessageBox>
#include <QMainWindow>
#include <QApplication>
#include <QSystemTrayIcon>

#include <boost/avloop.hpp>
#include <boost/logger.hpp>

#include <avbotui.hpp>

class avbotqtui_impl : public QObject
{
	Q_OBJECT
public:

	avbotqtui_impl(boost::asio::io_service& io_service, boost::logger& _logger, int argc, char* argv[]);

	~avbotqtui_impl();

	void report_fatal_error(std::string text)
	{
		QMessageBox mbox;
		mbox.moveToThread(app->thread());

		mbox.setWindowIcon(app->windowIcon());
		mbox.setIcon(QMessageBox::Critical);
		mbox.setText(text.c_str());

		mbox.exec();
		::exit(1);
	}

	void show_vc_and_get_input_with_timeout(std::string imgdata, int timeout_sec, avbotui::vc_result_call_back cb);

	template<typename Handler>
 	void io_post(Handler handler)
	{
		m_io_service.post(handler);
	}

	template<typename Handler>
	void ui_post(Handler handler)
	{
		Q_EMIT post_event(handler);
	}

	void run();

	void quit()
	{
		app->quit();
	}

Q_SIGNALS:
	void post_event(std::function<void()> func);

private Q_SLOTS:
	void execute_post(std::function<void()> func)
	{
		func();
	}

	void write_log(std::string, std::string);

private:
	boost::asio::io_service& m_io_service;
	boost::logger& logger;
	boost::thread m_io_thread;

	QScopedPointer<QApplication> app;
	QScopedPointer<QSystemTrayIcon> m_tray;
};

