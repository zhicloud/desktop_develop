#ifndef MYLABEL_H
#define MYLABEL_H

#include <QLabel>

class MyLabel : public QLabel
{
	Q_OBJECT

public:
	MyLabel(QWidget *parent = NULL);
	~MyLabel();

protected:
	virtual void mouseReleaseEvent(QMouseEvent * ev);
signals:
	void clicked(void);
};

#endif // MYLABEL_H
