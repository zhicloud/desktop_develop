#include "cfblbutton.h"
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
CFblbutton::CFblbutton(QWidget *parent)
	: QPushButton(parent)
{
	m_index = -1;
	installEventFilter(this);
	setMouseTracking(true);
}

CFblbutton::~CFblbutton()
{

}

bool CFblbutton::eventFilter(QObject *obj, QEvent * ev)
{
	QMouseEvent* even = (QMouseEvent*)ev;
	if (even->type() == QEvent::Enter)
	{
		if (m_index	!= -1)
		{
			emit setFblIndex(m_index);
		}
	}
	return QWidget::eventFilter(obj, ev);
}

void CFblbutton::setIndex(int index)
{
	m_index = index;
}

int CFblbutton::getIndex()
{
	return m_index;
}
