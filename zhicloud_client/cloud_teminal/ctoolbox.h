#ifndef CTOOLBOX_H
#define CTOOLBOX_H

#include <QWidget>
#include <QtWidgets/QApplication>
#include <QDesktopWidget>
#include "feedbackwidget.h"
#include "crolatepoint.h"
#include "cesulabel.h"
#include <QTimer>
class CToolBox : public QWidget
{
	Q_OBJECT

public:
	CToolBox(QWidget *parent = 0);
	~CToolBox();
	void grabKey();
	void setUnamePwd(QString &uname,QString &pwd);
protected:
	virtual void customEvent(QEvent *event);
signals:
	void closeSignal();
public slots:
	void cesu();
	void fankui();
	void closeSlot();
	void startcesu();
private:
	CRolatePoint* pointer;
	FeedbackWidget* feedback;
	QStackedLayout* rightmainlayout;
	int isstartcesu;
	QPushButton* start;
	QLabel* downlspeed;
	cesulabel* pingjun;
	int iscompelete;
};

#endif // CTOOLBOX_H
