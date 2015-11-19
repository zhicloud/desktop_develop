#include "qrectevent.h"

QRectEvent::QRectEvent(Type type)
: QEvent(type)
{

}

QRectEvent::~QRectEvent()
{

}

void QRectEvent::SetRect(QRect rc)
{
	m_rect = rc;
}