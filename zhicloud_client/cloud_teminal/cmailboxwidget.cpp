#include "cmailboxwidget.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QSettings>
#include "mainwindow.h"
#define  MSGFILE "./msg.ini"
CMailBoxWidget::CMailBoxWidget(QWidget *parent)
	: QWidget(parent)
{
	whichbox = 1;//1mailbox,2unreadmailbox
	setAutoFillBackground(true);
	this->setFixedSize(570,341);
	int w = QApplication::desktop()->width();
	int h = QApplication::desktop()->height();
	this->setGeometry((w - 570) / 2, (h - 341) / 2 - 39, 570, 341);
	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(":/mailbox/mailboxbg")));
	this->setPalette(palette);
	this->setWindowFlags(Qt::FramelessWindowHint);


	QPainterPath path;
	QRectF rect = QRectF(0, 0, 570, 341);
	path.addRoundRect(rect, 3, 3);
	QPolygon polygon = path.toFillPolygon().toPolygon();//获得这个路径上的所有的点
	QRegion region(polygon);//根据这些点构造这个区域
	setMask(region);


	QPushButton* closebtn = new QPushButton;
	closebtn->setFixedSize(18, 18);
	closebtn->setObjectName("mclosebtn");
	closebtn->setStyleSheet("QPushButton#mclosebtn{border-image: url(:/mailbox/mclosenomal);}"
		"QPushButton#mclosebtn:hover{border-image: url(:/mailbox/mclosemove);}"
		"QPushButton#mclosebtn:pressed{border-image: url(:/mailbox/mcloseclick);}");
	connect(closebtn, SIGNAL(clicked()), SLOT(closebtnfunc()));

	QHBoxLayout* top_hly = new QHBoxLayout;
	//top_hly->setAlignment(Qt::AlignTop | Qt::AlignRight);
	top_hly->addStretch();
	top_hly->addWidget(closebtn);

	
	QWidget* leftwid = new QWidget;
	leftwid->setFixedSize(125,300);

	recvlabel = new MyLabel;
	recvlabel->setFixedSize(100, 25);
	int size = 0;
	QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
	if (msgfile)
	{
		msgfile->beginReadArray("messages");
		QStringList keys = msgfile->allKeys();
		size = keys.size();
		if (size > 1)
		{
		size = size - 1;
		}

	}
	recvlabel->setText(QStringLiteral("收件箱(%1)").arg(size));
	connect(recvlabel, SIGNAL(clicked()), SLOT(gotomailbox()));

	unreadlabel = new MyLabel;
	unreadlabel->setFixedSize(100, 25);
	unreadlabel->setText(QStringLiteral("未读信息(%1)").arg(size));

	connect(unreadlabel, SIGNAL(clicked()), SLOT(gotounreadmailbox()));

	QVBoxLayout* leftlayout = new QVBoxLayout;
	leftlayout->addWidget(recvlabel);
	leftlayout->addWidget(unreadlabel);
	leftlayout->addStretch();
	leftwid->setLayout(leftlayout);

	QWidget* rightop = new QWidget;
	rightop->setFixedSize(435,35);


	QPushButton* del = new QPushButton;
	del->setFixedSize(60,24);
	del->setObjectName("del");
	del->setStyleSheet("QPushButton#del{border-image: url(:/mailbox/mdelnomal);}"
		"QPushButton#del:hover{border-image: url(:/mailbox/mdelmove);}"
		"QPushButton#del:pressed{border-image: url(:/mailbox/mdelclick);}");
	connect(del, SIGNAL(clicked()), SLOT(delmailsfunc()));

	QPushButton* readed = new QPushButton;
	readed->setFixedSize(80, 24);
	readed->setObjectName("readed");
	readed->setStyleSheet("QPushButton#readed{border-image: url(:/mailbox/mreadclick);}"
		"QPushButton#readed:hover{border-image: url(:/mailbox/mreadmove);}"
		"QPushButton#readed:pressed{border-image: url(:/mailbox/mreadclick);}");
	connect(readed, SIGNAL(clicked()), SLOT(markmailsread()));

	CMailSearch* search = new CMailSearch;
	QHBoxLayout* top_layout = new QHBoxLayout;
	top_layout->setContentsMargins(0,0,0,0);
	top_layout->setAlignment(Qt::AlignCenter);
	top_layout->addWidget(del);
	top_layout->addWidget(readed);
	top_layout->addStretch(80);
	top_layout->addWidget(search);
	top_layout->addStretch(40);
	rightop->setLayout(top_layout);

	QWidget* rightmiwid = new QWidget;
	rightmiwid->setFixedSize(435,7*32);
	CMailItemWidget* first = new CMailItemWidget;
	QVBoxLayout* maillayout = new QVBoxLayout;
	maillayout->setContentsMargins(0, 0, 0, 60);
	maillayout->setSpacing(40);
	first->setInfo(QStringLiteral("主题"), "", QStringLiteral("时间"), "unread", QColor(228, 228, 228));
	first->clickable(0);
	maillayout->addWidget(first);
	
	int mailcount = 6;
	for (int i = 0; i < mailcount;i++)
	{
		CMailItemWidget* item = new CMailItemWidget;
		itemarray[i] = item;
		item->setInfo(QStringLiteral("云桌面更新通知"), QStringLiteral("2015-6-21"), 0);
		item->SetMailBox(this);
		maillayout->addWidget(item);
	}
	rightmiwid->setLayout(maillayout);

	QWidget* rightbottomwid = new QWidget;
	rightbottomwid->setFixedSize(435,30);

	QPushButton* prevpage = new QPushButton;
	QPushButton* nextpage = new QPushButton;
	pageno = new QLineEdit;
	pageno->setFixedSize(16, 13);
	pageno->setText("1");
	pagenumber = 1;
	prevpage->setFixedSize(32, 11);
	prevpage->setObjectName("prevpage");
	prevpage->setStyleSheet("QPushButton#prevpage{border-image: url(:/mailbox/prepagenomal);}"
		"QPushButton#prevpage:hover{border-image: url(:/mailbox/prepagenomal);}"
		"QPushButton#prevpage:pressed{border-image: url(:/mailbox/prepageclick);}");
	connect(prevpage, SIGNAL(clicked()), SLOT(prevpage()));
	
	nextpage->setFixedSize(32, 11);
	nextpage->setObjectName("nextpage");
	nextpage->setStyleSheet("QPushButton#nextpage{border-image: url(:/mailbox/nextpagenomal);}"
		"QPushButton#nextpage:hover{border-image: url(:/mailbox/nextpagenomal);}"
		"QPushButton#nextpage:pressed{border-image: url(:/mailbox/nextpageclick);}");
	connect(nextpage, SIGNAL(clicked()), SLOT(nextpage()));
	
	QHBoxLayout* rightbottomlayout = new QHBoxLayout;
	rightbottomlayout->setContentsMargins(0, 0, 0, 0);
	rightbottomlayout->setAlignment(Qt::AlignCenter);
	rightbottomlayout->addStretch(90);
	rightbottomlayout->addWidget(prevpage);
	rightbottomlayout->addWidget(pageno);
	rightbottomlayout->addWidget(nextpage);
	rightbottomlayout->addStretch(10);

	rightbottomwid->setLayout(rightbottomlayout);

	QWidget* right = new QWidget;
	QWidget* rightwid = new QWidget;
	QWidget* rightmsgwid = new QWidget;
	right->setFixedSize(435, 310);
	rightwid->setFixedSize(435,310);
	rightmsgwid->setFixedSize(435, 310);
	msg = new QLabel;
	msg->setFixedSize(300, 300);
	QHBoxLayout* msg_layout = new QHBoxLayout;
	msg_layout->addWidget(msg);
	rightmsgwid->setLayout(msg_layout);


	QVBoxLayout* rightlayout = new QVBoxLayout;
	rightlayout->setAlignment(Qt::AlignCenter);
	rightlayout->addWidget(rightop);
 	rightlayout->addWidget(rightmiwid);
 	rightlayout->addWidget(rightbottomwid);
	rightwid->setLayout(rightlayout);

	stack = new QStackedLayout;
	stack->setContentsMargins(0, 0, 0, 50);
	stack->addWidget(rightwid);
	stack->addWidget(rightmsgwid);
	right->setLayout(stack);
	QHBoxLayout* hlayout = new QHBoxLayout;
	hlayout->addWidget(leftwid);
	hlayout->addWidget(right);



	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->addLayout(top_hly);
	main_layout->addLayout(hlayout);
	this->setLayout(main_layout);

}

CMailBoxWidget::~CMailBoxWidget()
{

}

void CMailBoxWidget::closebtnfunc()
{
	hide();
}

void CMailBoxWidget::ShowMail()
{
	QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
	if (msgfile)
	{
		msgfile->beginReadArray("messages");
		QStringList keys = msgfile->allKeys();
		int unreadsize = 0;
		int size = keys.size();
		if (size >= 1)
		{
			size = size - 1;
		}
		for (int i = 0; i < size;i++)
		{
			QString val = msgfile->value(keys.at(i)).toString();
			QStringList list = val.split(",");
			if (list[3] == "unread")
			{
				unreadsize++;
			}
		}
		
		recvlabel->setText(QStringLiteral("收件箱(%1)").arg(size));
		unreadlabel->setText(QStringLiteral("未读信息(%1)").arg(unreadsize));


		if (size > 6)
		{
			if (size - pagenumber*6 >= 0)
			{
				for (int i = 0; i < 6;i++)
				{
					QString val = msgfile->value(keys.at(i+6*(pagenumber - 1))).toString();
					QStringList list = val.split(",");
					itemarray[i]->setInfo(list[0], list[1], list[2],list[3]);
					itemarray[i]->mshow();
				}
			}
			else
			{
				int row = size % 6;
				for (int i = 0; i < row;i++)
				{
					QString val = msgfile->value(keys.at(i + 6 * (pagenumber - 1))).toString();
					QStringList list = val.split(",");
					itemarray[i]->setInfo(list[0], list[1], list[2],list[3]);
					itemarray[i]->mshow();
				}
				for (int i = row; i < 6;i++)
				{
					itemarray[i]->mhide();
				}
			}
		}
		else
		{
			int i = 0;
			for (; i < size;i++)
			{
				QString val = msgfile->value(keys.at(i)).toString();
				QStringList list = val.split(",");
				itemarray[i]->setInfo(list[0],list[1],list[2],list[3]);
				itemarray[i]->mshow();
			}
			for (; i < 6;i++)
			{
				itemarray[i]->mhide();
			}
			
		}
	}
}

void CMailBoxWidget::AddMail(QString keytime, QString val)
{
	QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
	if (msgfile)
	{
		msgfile->setValue(keytime,val);
	}
}

void CMailBoxWidget::nextpage()
{

	if (whichbox == 2)
	{
		int no = unreadsize / 6;
		if (no * 6 < unreadsize)
		{
			no++;
		}
		if ((pagenumber < no))
		{
			pagenumber++;
		}
		pageno->setText(QString("%1").arg(pagenumber));
		gotounreadmailbox();
		return;
	}

	QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
	if (msgfile)
	{
		msgfile->beginReadArray("messages");
		QStringList keys = msgfile->allKeys();
		int size = keys.size();
		if (size > 1)
		{
			size = size - 1;
		}
		int no = size / 6;
		if (no * 6 < size)
		{
			no++;
		}
		if ((pagenumber < no))
		{
			pagenumber++;
		}

		pageno->setText(QString("%1").arg(pagenumber));
	}
	ShowMail();
}

void CMailBoxWidget::prevpage()
{
	if (pagenumber > 1)
	{
		pagenumber--;
		pageno->setText(QString("%1").arg(pagenumber));
	}
	if (whichbox == 2)
	{
		gotounreadmailbox();
	}
	else
	{
		ShowMail();
	}
	
}

void CMailBoxWidget::Openmail(QString ctx)
{
	msg->setText(ctx);
	stack->setCurrentIndex(1);
}

void CMailBoxWidget::gotomailbox()
{
	if (whichbox == 2)
	{
		pagenumber = 1;
		pageno->setText(QString("%1").arg(pagenumber));
		whichbox = 1;
	}
	ShowMail();
	stack->setCurrentIndex(0);
}

void CMailBoxWidget::gotounreadmailbox()
{
	if (whichbox == 1)
	{
		pagenumber = 1;
		pageno->setText(QString("%1").arg(pagenumber));
		whichbox = 2;
	}

	QStringList unreadmsg;
	QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
	if (msgfile)
	{
		msgfile->beginReadArray("messages");
		QStringList keys = msgfile->allKeys();
		keys.removeOne("lasttime");
		int size = keys.size();
		for (int i = 0; i < size; i++)
		{
			QString val = msgfile->value(keys.at(i)).toString();
			QStringList list = val.split(",");
			if (list[3] == "readed")
			{
				continue;
			}
			else
			{
				unreadmsg.push_back(keys.at(i));
			}

		}
		unreadsize = unreadmsg.size();
		unreadlabel->setText(QStringLiteral("未读信息(%1)").arg(unreadsize));
		recvlabel->setText(QStringLiteral("收件箱(%1)").arg(size));
		if (unreadsize > 6)
		{
			
			if (unreadsize - pagenumber * 6 >= 0)
			{
				for (int i = 0; i < 6; i++)
				{
					QString val = msgfile->value(unreadmsg.at(i + 6 * (pagenumber - 1))).toString();
						QStringList list = val.split(",");
						itemarray[i]->setInfo(list[0], list[1], list[2], list[3]);
						itemarray[i]->mshow();
				}
			}
			else
			{
				int row = unreadsize % 6;
				if (row == 0)
				{
					pagenumber--;
					row = 6;
					pageno->setText(QString("%1").arg(pagenumber));
				}
				for (int i = 0; i < row; i++)
				{
					QString val = msgfile->value(unreadmsg.at(i + 6 * (pagenumber - 1))).toString();
						QStringList list = val.split(",");
						itemarray[i]->setInfo(list[0], list[1], list[2], list[3]);
						itemarray[i]->mshow();
				}
				for (int i = row; i < 6; i++)
				{
					itemarray[i]->mhide();
				}
			}
		}
		else
		{
			int i = 0;
			for (; i < unreadsize; i++)
			{
				QString val = msgfile->value(unreadmsg.at(i)).toString();
				QStringList list = val.split(",");
				itemarray[i]->setInfo(list[0], list[1], list[2], list[3]);
				itemarray[i]->mshow();
			}
			for (; i < 6; i++)
			{
				itemarray[i]->mhide();
			}
			pageno->setText(QString("%1").arg(1));
		}
	}

	stack->setCurrentIndex(0);
}

void CMailBoxWidget::DelMail()
{
	
		
		QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
		if (msgfile)
		{

			msgfile->beginReadArray("messages");

			for (int i = 0; i < 6; i++)
			{
				if (itemarray[i]->getisselected())
				{
					msgfile->remove(itemarray[i]->stime);
					itemarray[i]->click();
				}
				
			}
		}
		if (whichbox == 1)
		{
			ShowMail();
		}
		if (whichbox == 2)
		{
			gotounreadmailbox();
		}
		
}

void CMailBoxWidget::markmailsread()
{
	
	QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
	if (msgfile)
	{

		msgfile->beginReadArray("messages");

		for (int i = 0; i < 6; i++)
		{
			if (itemarray[i]->getisselected())
			{
				msgfile->setValue(itemarray[i]->stime, QString("%1,%2,%3,%4").arg(itemarray[i]->stheme).arg(itemarray[i]->content).arg(itemarray[i]->stime).arg("readed"));
				itemarray[i]->click();
			}

		}
	}
	if (whichbox == 1)
	{
		ShowMail();
	}
	if (whichbox == 2)
	{
		gotounreadmailbox();
	}

}

void CMailBoxWidget::delmailsfunc()
{
	int ishow = 0;
	for (int i = 0; i < 6;i++)
	{
		if (itemarray[i]->getisselected())
		{
			ishow = 1;
			break;
		}
	}

	if (ishow)
	{
		CMailDelWidget* widget = new CMailDelWidget(this);
		widget->setGeometry((570 - 400) / 2, (341 - 180) / 2, 400, 180);
		widget->show();
	}
	
}
