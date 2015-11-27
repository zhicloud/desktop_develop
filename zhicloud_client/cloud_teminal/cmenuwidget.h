#ifndef CMENUWIDGET_H
#define CMENUWIDGET_H

#include <QWidget>
#include <mainwindow.h>

#include "cmailboxwidget.h"
#include "feedbackwidget.h"

class CMenuWidget : public QWidget
{
	Q_OBJECT

public:
	void setInfo(QString name,QString picpath);
	CMenuWidget(QWidget *parent, CMainWindow* main_win);
	~CMenuWidget();
public slots:
	void clickmail();
	void clickfankui();
	void clickchangeuser();
	void clickexit();
	void clickreboot();
	void clickshutdown();
private:
	CMainWindow* main_w;
	QLabel* logo;
	QLabel* name;
	
	CMailBoxWidget* mailwid;
	FeedbackWidget* feedbackwid;
public:
	QPushButton* mail;
};

#endif // CMENUWIDGET_H
