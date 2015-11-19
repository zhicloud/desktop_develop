#include "cnetdiswidget.h"
#include "mainwindow.h"
#define SPICE_RECONNECT QEvent::User+109
#include <QCoreApplication>

#define TIMEOUTTIME 30

cnetdiswidget::cnetdiswidget(QWidget *parent)
	: QWidget(parent)
{
	count = TIMEOUTTIME;
	state = 0;
	this->setFixedSize(420, 60);
	setAutoFillBackground(true);
	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(QString(":/netdisconnect/bg"))));
	this->setPalette(palette);

	label = new QLabel(this);
	label->setFixedSize(310, 14);
	label->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
	label->setText(QStringLiteral("哎哟喂，您的网络不给力哦！正在为你重连，请等待.(30s)"));
	label->setGeometry(80, 23, 310, 14);

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timefunc()));


}

cnetdiswidget::~cnetdiswidget()
{

}

void cnetdiswidget::timefunc()
{
	count--;
	QString fmt = QStringLiteral("哎哟喂，您的网络不给力哦！正在为你重连，请等待.(%1s)");
	label->setText(fmt.arg(count));
	if((count % 6 == 0) && (count != 0))
	{
	  QEvent *event = new QEvent(QEvent::Type(SPICE_RECONNECT));
          QCoreApplication::postEvent(m_mainw, event);
	  //qDebug() <<"post msg reconnect++++++++++++++++++++++++++++++++";
	}
	if (count == 0)
	{
		timer->stop();
		count = TIMEOUTTIME;
		hide();
		state = 0;

		m_mainw->menuexit();
	}
}

void cnetdiswidget::wshow()
{

	QString fmt = QStringLiteral("哎哟喂，您的网络不给力哦！正在为你重连，请等待.(%1s)");
        label->setText(fmt.arg(TIMEOUTTIME));
	if (state == 1)
	{
		return;
	}
	state = 1;
	timer->start(1000);
	show();
}

void cnetdiswidget::whide()
{
	if (state == 0)
	{
		return;
	}
	state = 0;
	timer->stop();
	count = TIMEOUTTIME;
	hide();
}
