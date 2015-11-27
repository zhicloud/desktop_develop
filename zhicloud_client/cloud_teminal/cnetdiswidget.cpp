#include "cnetdiswidget.h"
#include "mainwindow.h"
cnetdiswidget::cnetdiswidget(QWidget *parent)
	: QWidget(parent)
{
	count = 20;
	state = 0;
	this->setFixedSize(340, 60);
	setAutoFillBackground(true);
	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(QString(":/netdisconnect/bg"))));
	this->setPalette(palette);

	label = new QLabel(this);
	label->setFixedSize(250, 14);
	label->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:14px;border:0px;");
	label->setText(QStringLiteral("网络断开连接,请检查网络.(20s)"));
	label->setGeometry(80, 23, 200, 14);

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timefunc()));


}

cnetdiswidget::~cnetdiswidget()
{

}

void cnetdiswidget::timefunc()
{
	count--;
	QString fmt = QStringLiteral("网络断开连接,请检查网络.(%1s)");
	label->setText(fmt.arg(count));
	if (count == 0)
	{
		timer->stop();
		count = 20;
		hide();
		state = 0;

		m_mainw->menuexit();
	}
}

void cnetdiswidget::wshow()
{

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
	count = 20;
	hide();
}
