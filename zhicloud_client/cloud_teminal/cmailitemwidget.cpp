#include "cmailitemwidget.h"
#include <QMessageBox>
#include <cmailboxwidget.h>
#include <QSettings>
#define MSGFILE "./msg.ini"
CMailItemWidget::CMailItemWidget(QWidget *parent)
	: QWidget(parent)
{
	isclickable = 1;
	isselected = 0;
	this->setFixedSize(435, 40);
	
	selectlabel = new MyLabel;
	selectlabel->setFixedSize(12,12);
	selectlabel->setPixmap(QPixmap(QString(":/mailbox/mailunselect")));

	connect(selectlabel, SIGNAL(clicked()), this, SLOT(click()));
	
	mailicon = new QLabel;
	mailicon->setFixedSize(15, 10);
	mailicon->setPixmap(QPixmap(QString(":/mailbox/unreadmsg")));
	
	theme = new MyLabel;
	theme->setFixedSize(110, 25);
	theme->setStyleSheet("background-color:transparent;font-size:11px;border:0px;");
	theme->setText(QStringLiteral("主题"));
	connect(theme, SIGNAL(clicked()), this, SLOT(openmail()));

	time = new QLabel;
	time->setFixedSize(80, 25);
	time->setStyleSheet("background-color:transparent;font-size:11px;border:0px;");
	time->setText(QStringLiteral("时间"));

	QHBoxLayout* main_layout = new QHBoxLayout;
	main_layout->setAlignment(Qt::AlignCenter);

	main_layout->addWidget(selectlabel);
	main_layout->addWidget(mailicon);
	main_layout->addWidget(theme);
	main_layout->addStretch();
	main_layout->addWidget(time);

	this->setLayout(main_layout);
}

CMailItemWidget::~CMailItemWidget()
{

}

void CMailItemWidget::setInfo(QString th, QString ctx, QString ti, QString isread, QColor color /*= QColor(0,0,0,0)*/)
{
	QPalette palette;
	palette.setColor(QPalette::Background, color);
	this->setPalette(palette);

	if (isread == "unread")
	{
		mailicon->setPixmap(QPixmap(QString(":/mailbox/unreadmsg")));
	}
	else
	{
		mailicon->setPixmap(QPixmap(QString(":/mailbox/readedmsg")));
	}

	theme->setText(th);
	
	time->setText(ti.left(8));

	content = ctx;
	stime = ti;
	stheme = th;
}

void CMailItemWidget::click()
{
	if (!isclickable)
	{
		return;
	}
	isselected = ~isselected;

	if (isselected)
	{
		selectlabel->setPixmap(QPixmap(QString(":/mailbox/mailselect")));
	}
	else
	{
		selectlabel->setPixmap(QPixmap(QString(":/mailbox/mailunselect")));
	}
}

void CMailItemWidget::openmail()
{
	if (!isclickable)
	{
		return;
	}
	m_mail->Openmail(content);

	mailicon->setPixmap(QPixmap(QString(":/mailbox/readedmsg")));

	QSettings* msgfile1 = new QSettings(MSGFILE, QSettings::IniFormat);
	if (msgfile1)
	{
		msgfile1->setValue(QString("messages/%1").arg(stime), QString("%1,%2,%3,%4").arg(stheme).arg(content).arg(stime).arg("readed"));
	}

   QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
   if (msgfile)
   {
      msgfile->beginReadArray("messages");
      QStringList keys = msgfile->allKeys();
      int unreadsize = 0;
      keys.removeOne("lasttime");
      int size = keys.size();
      //size = size - 1;
      for (int i = 0; i < size; i++)
      {
         QString val = msgfile->value(keys.at(i)).toString();
         QStringList list = val.split(",");
         if (list.size() >= 4)
         {
            if (list[3] == "unread")
            {
               unreadsize++;
            }
         }
      }

		m_mail->recvlabel->setText(QStringLiteral("收件箱(%1)").arg(size));
		m_mail->unreadlabel->setText(QStringLiteral("未读信息(%1)").arg(unreadsize));
	}

}

void CMailItemWidget::mhide()
{
	selectlabel->hide();
	mailicon->hide();
	theme->hide();
	time->hide();
}

void CMailItemWidget::mshow()
{
	selectlabel->show();
	mailicon->show();
	theme->show();
	time->show();
}

void CMailItemWidget::SetMailBox(CMailBoxWidget* mail)
{
	m_mail = mail;
}

void CMailItemWidget::clickable(int isclick /*= 1*/)
{
	isclickable = isclick;
}

int CMailItemWidget::getisselected()
{
	return isselected;
}