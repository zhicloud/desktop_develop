#ifndef CMAILITEMWIDGET_H
#define CMAILITEMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include "mylabel.h"

class CMailBoxWidget;
class CMailItemWidget : public QWidget
{
	Q_OBJECT

public:
	CMailItemWidget(QWidget *parent = NULL);
	~CMailItemWidget();
	void SetMailBox(CMailBoxWidget* mail);
	void setInfo(QString theme,QString time,QString ctx,QString isread  = QString("unread"),QColor color = QColor(0,0,0,0));
	void clickable(int isclick = 1);
	int getisselected();
public slots:
	void click();
	void openmail();
	void mhide();
	void mshow();
private:
	MyLabel* selectlabel;
	QLabel* mailicon;
	int isselected;
	MyLabel* theme;
	QLabel* time;

	
	
	CMailBoxWidget* m_mail;
	int isclickable;
public:
	QString stime;
	QString stheme;
	QString content;
	
};

#endif // CMAILITEMWIDGET_H
