#ifndef CNETDISWIDGET_H
#define CNETDISWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>

class CMainWindow;

class cnetdiswidget : public QWidget
{
	Q_OBJECT

public:
	cnetdiswidget(QWidget *parent);
	~cnetdiswidget();
	void setMainw(CMainWindow* win){ m_mainw = win; };
	CMainWindow* getMainw(){ return m_mainw; };
	void wshow();
	void whide();
	int getState(){ return state; };
public slots:
	void timefunc();
private:
	QLabel* label;

	QTimer* timer;

	int count;

	int state;

	CMainWindow* m_mainw;
};

#endif // CNETDISWIDGET_H
