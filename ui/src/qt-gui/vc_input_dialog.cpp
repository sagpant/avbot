
#include <QTimer>
#include <QPushButton>
#include "vc_input_dialog.hpp"

vc_input_dialog::vc_input_dialog(avbotqtui_impl* _ui_impl, std::string imgdata, int timeout_sec, avbotui::vc_result_call_back cb, QWidget *parent)
	: QDialog(parent)
	, m_handler(cb)
	, ui_impl(_ui_impl)
	, m_time_remains(timeout_sec)
{
	ui.setupUi(this);

	m_posted = false;
	setAttribute(Qt::WA_DeleteOnClose);
	QImage qimg;

	qimg.loadFromData(reinterpret_cast<const uchar*>(imgdata.data()), imgdata.size());

	ui.vcimg->setPixmap(QPixmap::fromImage(qimg));

	auto timer =new QTimer(this);
	QObject::connect(timer, SIGNAL(timeout()), this, SLOT(timer_update()));
	timer->start(1000);

	timer_update();
}

void vc_input_dialog::timer_update()
{
	if (m_time_remains>0)
	{
		QPushButton * help_button = ui.buttonBox->button(QDialogButtonBox::Help);

		QString str = QString("(%1s)").arg(m_time_remains --);

		help_button->setText(str);
	}else
	{
		QPushButton * Cancel_button = ui.buttonBox->button(QDialogButtonBox::Cancel);

		Cancel_button->click();
	}
}


vc_input_dialog::~vc_input_dialog()
{
	if (!m_posted)
	{
		ui_impl->io_post(std::bind(m_handler, std::string()));
	}
}


void vc_input_dialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui.retranslateUi(this);
		break;
	default:
		break;
	}
}

void vc_input_dialog::on_vc_input_dialog_accepted()
{
	std::string vc;

	vc = ui.lineEdit->text().toStdString();

	ui_impl->io_post(std::bind(m_handler, vc));

	m_posted = true;
}

void vc_input_dialog::on_vc_input_dialog_rejected()
{

}
