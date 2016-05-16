#ifndef CMENUWIDGET_H
#define CMENUWIDGET_H

#include <QWidget>
#include <mainwindow.h>

#include "cmailboxwidget.h"
#include "feedbackwidget.h"
#include "ctoolbox.h"

class CSpiceMultVEx;

class CMenuWidget : public QWidget
{
	Q_OBJECT

public:
	void setInfo(QString name,QString picpath);
	CMenuWidget(QWidget *parent, CMainWindow* main_win);
	~CMenuWidget();
	void delBtn();
	void setViewer(CSpiceMultVEx *viewer);
signals:
	void sendF11();//xzg

public slots:
	void clickmail();
	void clicktoolbox();
	void clickchangeuser();
	void clickexit();
	void clickreboot();
	void clickshutdown();
	void viewerGrabSlot();
private:
	CMainWindow* main_w;
	CSpiceMultVEx *m_viewer;
	QLabel* logo;
	QLabel* name;
	
	CMailBoxWidget* mailwid;
	FeedbackWidget* feedbackwid;
	CToolBox*		toolbox;
public:
	QPushButton* mail;
	int mail_status;
};

#endif // CMENUWIDGET_H
