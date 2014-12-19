
#pragma once
#include <QResizeEvent>
#include <QWidget>
#include <QLabel>

class QCenteredImageView : public QWidget
{
public:
    explicit QCenteredImageView(QWidget* parent = 0, Qt::WindowFlags f = 0)
		: QWidget(parent, f)
	{
		m_label = new QLabel(this);
		m_label->setScaledContents(true);
	}

    virtual void resizeEvent(QResizeEvent* e)
	{
		if (pixmap())
		{
			auto s = pixmap()->size();
			s.scale(e->size().width(), e->size().height(), Qt::KeepAspectRatio);

			int left, top, right, bottom;
			getContentsMargins(&left, &top, &right, &bottom);

			QRect effectiveRect(QPoint(0,0), e->size());
			effectiveRect.adjust(+left, +top, -right, -bottom);

			QRect label_rect(QPoint(0,0), s);

			label_rect.moveCenter(effectiveRect.center());

			m_label->setGeometry(label_rect);

			e->accept();
		}
		else
		{
			QWidget::resizeEvent(e);
		}

	}

	const QPixmap* pixmap() const
	{
		return m_label->pixmap();
	}

    virtual QSize sizeHint() const
    {
		if (pixmap())
		{
			auto s = pixmap()->size();
			return s*2;
		}else return QWidget::sizeHint();
	}

public Q_SLOTS:
    void setPixmap(const QPixmap &p)
	{
		m_label->setPixmap(p);
	}


private:
	QLabel* m_label;
};
