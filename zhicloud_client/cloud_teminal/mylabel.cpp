#include "mylabel.h"

MyLabel::MyLabel(QWidget * parent) : 
QLabel(parent)
{
}
void MyLabel::mouseReleaseEvent(QMouseEvent * ev)
{
	Q_UNUSED(ev)emit clicked();
}

MyLabel::~MyLabel()
{

}
