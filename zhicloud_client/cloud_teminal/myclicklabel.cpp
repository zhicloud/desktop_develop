#include "myclicklabel.h"
#include <QMouseEvent>

// mark: public:--------------------------------------------------
MyClickLabel::MyClickLabel(QWidget *parent) :
QLabel(parent)
{
	QPalette pa;
	pa.setColor(QPalette::WindowText, QColor(255, 255, 255, 178));		//设置透明度为70%
	setPalette(pa);
}

MyClickLabel::MyClickLabel(const QString &text, QWidget *parent) :
QLabel(parent)
{
	setText(text);
}

// mark: protected:------------------------------------------------
void MyClickLabel::mouseReleaseEvent(QMouseEvent *ev)
{
	if (ev->button() == Qt::LeftButton)
		emit clicked();//(this);
}

void MyClickLabel::enterEvent(QEvent * )
{
	QPalette pa;
	pa.setColor(QPalette::WindowText, QColor(255, 255, 255, 255));
	setPalette(pa);
}

void MyClickLabel::leaveEvent(QEvent *)
{
	QPalette pa;
	pa.setColor(QPalette::WindowText, QColor(255, 255, 255, 178));		//设置透明度为70%
	setPalette(pa);
}
