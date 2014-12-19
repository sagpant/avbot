#pragma once

#include "avbotui.hpp"
#include "avbotqtui_impl.hpp"

#include "ui_vc_input_dialog.h"

class vc_input_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit vc_input_dialog(avbotqtui_impl* ui_impl, std::string imgdata, int timeout_sec, avbotui::vc_result_call_back cb, QWidget *parent = 0);

	~vc_input_dialog();

protected Q_SLOTS:
	void changeEvent(QEvent *e);

	void timer_update();

	void on_vc_input_dialog_accepted();
	void on_vc_input_dialog_rejected();

private:
	Ui::vc_input_dialog ui;

	avbotui::vc_result_call_back m_handler;
	avbotqtui_impl* ui_impl;

	int m_time_remains;
	bool m_posted;
};

