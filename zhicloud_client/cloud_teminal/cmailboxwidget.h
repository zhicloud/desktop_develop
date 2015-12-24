#ifndef CMAILBOXWIDGET_H
#define CMAILBOXWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include "cmailitemwidget.h"
#include <cmailsearch.h>
#include <QStackedLayout>
#include "cmaildelwidget.h"
class CMailBoxWidget : public QWidget
{
	Q_OBJECT

public:
	CMailBoxWidget(QWidget *parent = NULL);
	~CMailBoxWidget();
	void ShowMail();
	void AddMail(QString keytime,QString val);
	void DelMail();
	void Openmail(QString ctx);
public slots:
	void closebtnfunc();
	void nextpage();
	void prevpage();
	void gotomailbox();
	void gotounreadmailbox();
	void delmailsfunc();
	void markmailsread();
private:
	CMailItemWidget* itemarray[6];
	int pagenumber;
	QLineEdit* pageno;
	QStackedLayout* stack;
	QLabel* msg;
public:
	MyLabel* recvlabel;
	MyLabel* unreadlabel;
	int whichbox;
	int unreadsize;

};

#endif // CMAILBOXWIDGET_H
