#include "ctoolbox.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include "cesulabel.h"
#include "crolatepoint.h"
CToolBox::CToolBox(QWidget *parent)
	: QWidget(parent)
{
	isstartcesu = 0;
	this->setFixedSize(569, 341);
	setAutoFillBackground(true);
	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(":/toolbox/bg")));
	this->setPalette(palette);

	QPainterPath path;
	QRectF rect = QRectF(0, 0, 570, 341);
	path.addRoundRect(rect, 3, 3);
	QPolygon polygon = path.toFillPolygon().toPolygon();//获得这个路径上的所有的点
	QRegion region(polygon);//根据这些点构造这个区域
	setMask(region);

	int currentScreenWidth = QApplication::desktop()->width();
	int currentScreenHeight = QApplication::desktop()->height();
	this->setGeometry((currentScreenWidth - 569) / 2, (currentScreenHeight - 341) / 2 - 39, 569, 341);

	QPushButton* closeButton = new QPushButton(this);
	closeButton->setObjectName("btn");
	closeButton->setFixedSize(18, 18);
	closeButton->setStyleSheet("QPushButton#btn{border-image: url(:/feedback_res/closenomal);}"
		"QPushButton#btn:hover{border-image: url(:/feedback_res/closemove);}"
		"QPushButton#btn:pressed{border-image: url(:/feedback_res/closeclick);}");
	closeButton->setGeometry(569 - 27,8,18,18);
	connect(closeButton, SIGNAL(clicked()), SLOT(closeSlot()));


	QPushButton* cesu = new QPushButton;
	QPushButton* fankui = new QPushButton;

	cesu->setFixedSize(78, 12);
	cesu->setObjectName("cesu");
	cesu->setStyleSheet("QPushButton#cesu{border-image: url(:/toolbox/speedbtnnomal);}"
		"QPushButton#cesu:hover{border-image: url(:/toolbox/speedbtnmove);}"
		"QPushButton#cesu:pressed{border-image: url(:/toolbox/speedbtnclick);}");

	connect(cesu, SIGNAL(clicked()), SLOT(cesu()));


	fankui->setFixedSize(78, 12);
	fankui->setObjectName("fankui");
	fankui->setStyleSheet("QPushButton#fankui{border-image: url(:/toolbox/fankuinomal);}"
		"QPushButton#fankui:hover{border-image: url(:/toolbox/fankuimove);}"
		"QPushButton#fankui:pressed{border-image: url(:/toolbox/fankuiclick);}");
	connect(fankui, SIGNAL(clicked()), SLOT(fankui()));

	QVBoxLayout* leftlayout = new QVBoxLayout;
	leftlayout->setContentsMargins(20, 0, 0, 0);
	leftlayout->setSpacing(30);
	leftlayout->addStretch(5);
	leftlayout->addWidget(cesu);
	leftlayout->addWidget(fankui);
	leftlayout->addStretch(70);

	QWidget* leftwid = new QWidget;
	leftwid->setFixedSize(100,300);
	leftwid->setLayout(leftlayout);


	QWidget* midwid = new QWidget;
	midwid->setFixedSize(190, 300);
	pointer = new CRolatePoint;
	pointer->SetTool(this);
	pointer->setFixedSize(153, 153);

	QHBoxLayout* phlayout = new QHBoxLayout;
	phlayout->addStretch(40);
	phlayout->addWidget(pointer);
	phlayout->addStretch(60);
	QVBoxLayout* mid_layout = new QVBoxLayout;
	mid_layout->addStretch(40);
	mid_layout->addLayout(phlayout);
	mid_layout->addStretch(60);
	midwid->setLayout(mid_layout);

	QWidget* righttwid = new QWidget;
	righttwid->setFixedSize(240, 300);
	QLabel* netdelay = new QLabel;
	netdelay->setFixedSize(50, 20);
	netdelay->setText(QStringLiteral("125ms"));
	downlspeed = new QLabel;
	downlspeed->setFixedSize(50,20);
	downlspeed->setText(QStringLiteral("0.0KB/s"));
// 	QLabel* pingjun = new QLabel;
// 	pingjun->setFixedSize(220,20);
// 	pingjun->setText(QStringLiteral("Æ½¾ùÏÂÔØ1000.6KB/s Ïàµ±ÓÚ8M¿í´ø"));
	pingjun = new cesulabel;

	start = new QPushButton;
	start->setFixedSize(139, 28);
	start->setObjectName("start");
	start->setStyleSheet("QPushButton#start{border-image: url(:/toolbox/startnomal);}"
		"QPushButton#start:hover{border-image: url(:/toolbox/startmove);}"
		"QPushButton#start:pressed{border-image: url(:/toolbox/startclick);}");

	connect(start, SIGNAL(clicked()), SLOT(startcesu()));
	

	QHBoxLayout* start_hl = new QHBoxLayout;
	start_hl->addStretch(40);
	start_hl->addWidget(start);
	start_hl->addStretch(60);

	QHBoxLayout* hl = new QHBoxLayout;
	hl->addStretch(30);
	hl->addWidget(netdelay);
	hl->addStretch(45);
	hl->addWidget(downlspeed);
	hl->addStretch(25);

	QVBoxLayout*mainv = new QVBoxLayout;
	mainv->setContentsMargins(0, 50, 0, 50);
	mainv->addLayout(hl);
	mainv->addWidget(pingjun);
	mainv->addLayout(start_hl);
	righttwid->setLayout(mainv);

	
	QWidget* right = new QWidget;
	right->setFixedSize(430, 300);
	QHBoxLayout* right_lay_out = new QHBoxLayout;
	right_lay_out->addWidget(midwid);
	right_lay_out->addWidget(righttwid);
	right->setLayout(right_lay_out);

	
	feedback = new FeedbackWidget();
	connect(feedback,SIGNAL(closeFeebackSignal()),this,SLOT(closeSlot()));
	QWidget* rightmain = new QWidget;
	rightmain->setFixedSize(430, 300);
	rightmainlayout = new QStackedLayout;
	rightmainlayout->setContentsMargins(0, 0, 0, 0);
	rightmainlayout->addWidget(right);
	rightmainlayout->addWidget(feedback);
	//rightmainlayout->setCurrentIndex(1);
	rightmain->setLayout(rightmainlayout);

	QHBoxLayout* main_layout = new QHBoxLayout;
	main_layout->setContentsMargins(10, 30, 10, 10);
	main_layout->addWidget(leftwid);
	main_layout->addWidget(rightmain);
	this->setLayout(main_layout);

}

CToolBox::~CToolBox()
{
closeSlot();
}

void CToolBox::cesu()
{
	rightmainlayout->setCurrentIndex(0);
}

void CToolBox::setUnamePwd(QString &uname,QString &pwd)
{
	feedback->setUnamePwd(uname,pwd);
}
	

void CToolBox::fankui()
{
	rightmainlayout->setCurrentIndex(1);
}

void CToolBox::closeSlot()
{
	emit closeSignal();
	close();
}

void CToolBox::grabKey()
{
	feedback->grabKey();
}

void CToolBox::startcesu()
{
	iscompelete = 0;
	if (!isstartcesu)
	{
		isstartcesu = 1;
		start->setStyleSheet("QPushButton#start{border-image: url(:/toolbox/cancelnomal);}"
			"QPushButton#start:hover{border-image: url(:/toolbox/cancelmove);}"
			"QPushButton#start:pressed{border-image: url(:/toolbox/cancelclick);}");

		pointer->start();

		downlspeed->setText(QStringLiteral("0.0KB/s"));
		pingjun->setKBM("0.0Kb/s", "0M");

	}
	else
	{
		isstartcesu = 0;

		pointer->stop();

		start->setStyleSheet("QPushButton#start{border-image: url(:/toolbox/startnomal);}"
			"QPushButton#start:hover{border-image: url(:/toolbox/startmove);}"
			"QPushButton#start:pressed{border-image: url(:/toolbox/startclick);}");
	}
}

void CToolBox::customEvent(QEvent *event)
{
	int type = event->type();

	switch (type)
	{
	case CESUCOMPLETE:
	{
		start->setStyleSheet("QPushButton#start{border-image: url(:/toolbox/againnomal);}"
			 "QPushButton#start:hover{border-image: url(:/toolbox/againmove);}"
			 "QPushButton#start:pressed{border-image: url(:/toolbox/againclick);}");

		iscompelete = 1;
		isstartcesu = 0;
		break;
	}
	case CESUINFOEVT:
	{
		CESUINFO info;
		QCesuEvent* ev = (QCesuEvent*)event;
		ev->Getcesuinfo(info);
		QString fmt = "%1KB/s";
		downlspeed->setText(fmt.arg(info.sudu));

		if (iscompelete)
		{
				QString fmt = "%1KB/s";
			QString fmtm = "0M";
			if (info.pingjunsudu > 0 && info.pingjunsudu < 7)
			{
				fmtm = "64K";

			}
			if (info.pingjunsudu > 7 && info.pingjunsudu < 12)
			{
				fmtm = "128K";
			}
			if (info.pingjunsudu > 13 && info.pingjunsudu < 25)
			{
				fmtm = "256K";
			}
			if (info.pingjunsudu > 26 && info.pingjunsudu < 50)
			{
				fmtm = "512K";
			}
			if (info.pingjunsudu > 50 && info.pingjunsudu < 100)
			{
				fmtm = "1M";
			}
			if (info.pingjunsudu > 100 && info.pingjunsudu < 200)
			{
				fmtm = "2M";
			}
			if (info.pingjunsudu > 200 && info.pingjunsudu < 300)
			{
				fmtm = "3M";
			}
			if (info.pingjunsudu > 300 && info.pingjunsudu < 400)
			{
				fmtm = "4M";
			}
			if (info.pingjunsudu > 400 && info.pingjunsudu < 500)
			{
				fmtm = "5M";
			}
			if (info.pingjunsudu > 500 && info.pingjunsudu < 1000)
			{
				fmtm = "10M";
			}
			if (info.pingjunsudu > 1000)
			{
				fmtm = ">10M";
			}
			pingjun->setKBM(fmt.arg(info.pingjunsudu), fmtm);
			iscompelete = 0;
		}

		break;
	}
	default:
		;
	}
}
