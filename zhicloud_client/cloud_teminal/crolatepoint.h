#ifndef CROLATEPOINT_H
#define CROLATEPOINT_H

#include <QWidget>
#include <QTimer>
#include <QtNetwork>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QUrl>

#define CESUCOMPLETE QEvent::User+130
#define CESUINFOEVT QEvent::User+131
class CToolBox;

typedef struct cesuStruct
{
	int sudu;
	int pingjunsudu;
}CESUINFO;

class QCesuEvent : public QEvent
{
public:
	QCesuEvent(Type type)
		:QEvent(type)
	{}
	~QCesuEvent()
	{}
	void Getcesuinfo(CESUINFO& info)
	{
		info.sudu = sudu;
		info.pingjunsudu = pingjunsudu;
	}
public:
	int pingjunsudu;
	int sudu;
};

class CRolatePoint : public QWidget
{
	Q_OBJECT

public:
	CRolatePoint(QWidget *parent = NULL);
	~CRolatePoint();
	void SetTool(CToolBox* box){ toolbox = box; }
	void start();
	void stop();

protected:
	virtual void paintEvent(QPaintEvent *event);
	
public slots :
	void httpFinished();
	void httpReadyRead();
	void updateDataReadProgress(qint64 bytesRead, qint64 totalBytes);
private:
	int angle;
	int sudu;
	QNetworkAccessManager _qnam;
	QNetworkReply *_reply;
	qint64 totalbytes;
	qint64 starttime;
	QDateTime firstDateTime;
	CToolBox* toolbox;

	int maxsudu;
	int minsudu;
};

#endif // CROLATEPOINT_H
