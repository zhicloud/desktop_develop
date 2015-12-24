#ifndef QRECTEVENT_H
#define QRECTEVENT_H

#include <QEvent>
#include <QRect>
#include <qcoreevent.h>
class QRectEvent : public QEvent
{
public:
	QRectEvent(Type type);
	~QRectEvent();
	void SetRect(QRect rc);
	QRect GetRect(){ return m_rect; }
private:
	QRect m_rect;
};

#endif // QRECTEVENT_H
