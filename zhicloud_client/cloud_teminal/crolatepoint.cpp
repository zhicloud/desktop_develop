#include "crolatepoint.h"
#include <QPainter>
#define INIFILE "./user.ini"
#include <QDateTime>
#include "ctoolbox.h"
CRolatePoint::CRolatePoint(QWidget *parent)
	: QWidget(parent)
{
	this->setFixedSize(153, 153);
	angle = 0;
	sudu = 0;
	totalbytes = 0;
	starttime = 0;
	_reply = NULL;
	

}

CRolatePoint::~CRolatePoint()
{

}

void CRolatePoint::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	QPixmap pix;
	bool bl = pix.load(":/toolbox/pointer");
	painter.translate(76, 76);                //使图片的中心作为旋转的中心
	painter.rotate(angle);                //顺时针旋转90°
	painter.translate(-76, -76);        //将原点复位
	painter.drawPixmap(0, 0, 153, 153, pix);
}

void CRolatePoint::start()
{
	QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
	QString svrurl;
	if (settings)
	{
		svrurl = settings->value("server/url").toString();
	}
	QString str = "download/Network_test.war";
	svrurl.replace("CloudDeskTopMS/interface/connect", str);



	QNetworkRequest networkRequest;
	networkRequest.setUrl(svrurl);
	_reply = _qnam.get(networkRequest);
	

	connect(_reply, SIGNAL(readyRead()),
		this, SLOT(httpReadyRead()));
	connect(_reply, SIGNAL(downloadProgress(qint64, qint64)),
		this, SLOT(updateDataReadProgress(qint64, qint64)));
	connect(_reply, SIGNAL(finished()),
		this, SLOT(httpFinished()));

	starttime = 0;
	maxsudu = 0;
	minsudu = 10000;
	totalbytes = 0;
	angle = 0;
	update();
}

void CRolatePoint::stop()
{
	if (_reply)
	{
		_reply->abort();
	}
}

void CRolatePoint::httpFinished()
{
	QEvent *event = new QEvent(QEvent::Type(CESUCOMPLETE));
	QCoreApplication::postEvent(toolbox, event);

	QCesuEvent *evv = new QCesuEvent(QEvent::Type(CESUINFOEVT));
	evv->pingjunsudu = (minsudu + maxsudu ) / 2;
	evv->sudu = sudu;
	QCoreApplication::postEvent(toolbox, evv);

}

void CRolatePoint::httpReadyRead()
{
	if (starttime == 0)
	{
		firstDateTime = QDateTime::currentDateTime();
		starttime = 1;
	}
	
}

void CRolatePoint::updateDataReadProgress(qint64 bytesRead, qint64 totalBytes)
{
	totalbytes = bytesRead;

	int time = abs(QDateTime::currentDateTime().secsTo(firstDateTime));
	if (time == 0)
	{
		time = 1;
	}
	if (time != 0)
	{
		sudu = (totalbytes / 1024 ) / time; //kbs

		if (sudu > maxsudu)
		{
			maxsudu = sudu;
		}

		if (sudu < minsudu)
		{
			minsudu = sudu;
		}
		qDebug() << "time =" << time << "angle =" << angle << "totalbtyes = " << totalbytes << "sudu = " << sudu;

		QCesuEvent *event = new QCesuEvent(QEvent::Type(CESUINFOEVT));
		event->sudu = sudu;
		QCoreApplication::postEvent(toolbox, event);




		float pingj = (maxsudu + minsudu) / 2;


		if (pingj > 0 && pingj < 6.4)
		{
			angle = pingj / 6.4 * 45;
		}
		else if (pingj > 6.4 && pingj < 12.8)
		{
			angle = (pingj - 6.4) / 6.4 * 45 + 45;
		}
		else if (pingj > 12.8 && pingj < 25.6)
		{
			angle = (pingj - 12.8) / 12.8 * 45 + 90;
		}
		else if (pingj > 25.6 && pingj < 51.2)
		{
			angle = (pingj - 25.6) / 25.6 * 45 + 135;
		}
		else if (pingj > 51.2 && pingj < 102.4)
		{
			angle = (pingj - 51.2) / 51.2 * 30 + 180;
		}
		else if (pingj > 102.4 && pingj < 512.0)
		{
			angle = (pingj - 102.4) /409.6  * 30 + 210;
		}
		else if (pingj >  512.0 && pingj < 1024.0)
		{
			angle = (pingj - 512.0) / 512.0 * 15 + 240;
		}
		else if (maxsudu == 0)
		{
			angle = 0;
		}
		else
		{
			angle = 260;
		}
		update();
	}
	
	
}
