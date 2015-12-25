#include "fblsetwidget.h"
#include <QMouseEvent>
#include "cfblbutton.h"
#include "fbl_message_box.h"
#include <QDesktopWidget>
#include <QApplication>
//#include <QFile>
#include <QTextStream>
//#include <linux/reboot.h>
fblSetWidget::fblSetWidget(QWidget *parent)
:QWidget(parent)
{
	fblIndex = 1;
	isfblBtnShow = false;
	setMouseTracking(true);
	setFixedSize(400, 350);
	createView();
	initView();
}

void fblSetWidget::createView()
{
#ifdef ZS
	QLabel* picl = new QLabel;
	picl->setFixedSize(116, 100);
	picl->setPixmap(QPixmap(QString(":/sysset_res/800X600-ICON")));

	QLabel* pic2 = new QLabel;
	pic2->setFixedSize(128, 110);
	pic2->setPixmap(QPixmap(QString(":/sysset_res/1024X768-ICON")));

	QLabel* pic3 = new QLabel;
	pic3->setFixedSize(151, 100);
	pic3->setPixmap(QPixmap(QString(":/sysset_res/1336X768-ICON")));

	QHBoxLayout* hpiclayout1 = new QHBoxLayout;
	hpiclayout1->setAlignment(Qt::AlignTop);
	hpiclayout1->addStretch(30);
	hpiclayout1->addWidget(picl);
	hpiclayout1->addStretch(70);

	QHBoxLayout* hpiclayout2 = new QHBoxLayout;
	hpiclayout2->setAlignment(Qt::AlignTop);
	hpiclayout2->addStretch(30);
	hpiclayout2->addWidget(pic2);
	hpiclayout2->addStretch(70);

	QHBoxLayout* hpiclayout3 = new QHBoxLayout;
	hpiclayout3->setAlignment(Qt::AlignTop);
	hpiclayout3->addStretch(30);
	hpiclayout3->addWidget(pic3);
	hpiclayout3->addStretch(70);

	QWidget *setshowwidget1 = new QWidget;
	setshowwidget1->setFixedSize(400, 120);
	setshowwidget1->setLayout(hpiclayout1);

	QWidget *setshowwidget2 = new QWidget;
	setshowwidget2->setFixedSize(400, 120);
	setshowwidget2->setLayout(hpiclayout2);

	QWidget *setshowwidget3 = new QWidget;
	setshowwidget3->setFixedSize(400, 120);
	setshowwidget3->setLayout(hpiclayout3);

	QWidget* picwidget = new QWidget;
	picwidget->setFixedSize(400, 120);
	hpiclayout = new QStackedLayout;
	hpiclayout->setContentsMargins(0, 0, 0, 0);
	hpiclayout->addWidget(setshowwidget1);
	hpiclayout->addWidget(setshowwidget2);
	hpiclayout->addWidget(setshowwidget3);
	hpiclayout->setCurrentIndex(0);
	picwidget->setLayout(hpiclayout);

	fblBtn = new CFblbutton(this);
	fblBtn->setFixedSize(178, 28);
	fblBtn->setObjectName("fblBtn");
	fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/800X600-down);}");
	connect(fblBtn, SIGNAL(clicked()), SLOT(hideOrShow()));

	CFblbutton *fblBtn1 = new CFblbutton(this);
	fblBtn1->setIndex(0);
	fblBtn1->setFixedSize(178, 28);
	fblBtn1->setObjectName("fblBtn1");
	fblBtn1->setStyleSheet("QPushButton#fblBtn1{border-image: url(:/sysset_res/800X600-down);}"
		"QPushButton#fblBtn1:hover{border-image: url(:/sysset_res/800X600-move);}"
		"QPushButton#fblBtn1:pressed{border-image: url(:/sysset_res/800X600-choose);}");
	fblBtn1->hide();

	connect(fblBtn1, SIGNAL(setFblIndex(int)), this, SLOT(setFblIndex(int)));
	connect(fblBtn1, SIGNAL(clicked()), this, SLOT(chooseFbl()));

	CFblbutton *fblBtn2 = new CFblbutton(this);
	fblBtn2->setIndex(1);
	fblBtn2->setFixedSize(178, 28);
	fblBtn2->setObjectName("fblBtn2");
	fblBtn2->setStyleSheet("QPushButton#fblBtn2{border-image: url(:/sysset_res/1024X768-down);}"
		"QPushButton#fblBtn2:hover{border-image: url(:/sysset_res/1024X768-move);}"
		"QPushButton#fblBtn2:pressed{border-image: url(:/sysset_res/1024X768-choose);}");
	fblBtn2->hide();

	connect(fblBtn2, SIGNAL(setFblIndex(int)), this, SLOT(setFblIndex(int)));
	connect(fblBtn2, SIGNAL(clicked()), this, SLOT(chooseFbl()));

	CFblbutton *fblBtn3 = new CFblbutton(this);
	fblBtn3->setIndex(2);
	fblBtn3->setFixedSize(178, 28);
	fblBtn3->setObjectName("fblBtn3");
	fblBtn3->setStyleSheet("QPushButton#fblBtn3{border-image: url(:/sysset_res/1336X768-down);}"
		"QPushButton#fblBtn3:hover{border-image: url(:/sysset_res/1336X768-move);}"
		"QPushButton#fblBtn3:pressed{border-image: url(:/sysset_res/1336X768-choose);}");
	fblBtn3->hide();

	connect(fblBtn3, SIGNAL(setFblIndex(int)), this, SLOT(setFblIndex(int)));
	connect(fblBtn3, SIGNAL(clicked()), this, SLOT(chooseFbl()));

	fblBtn_Vec.push_back(fblBtn);
	fblBtn_Vec.push_back(fblBtn1);
	fblBtn_Vec.push_back(fblBtn2);
	fblBtn_Vec.push_back(fblBtn3);

	QVBoxLayout* vboxLayout = new QVBoxLayout;
	vboxLayout->setSpacing(0);
	vboxLayout->setAlignment(Qt::AlignLeft);
	vboxLayout->addWidget(fblBtn);
	vboxLayout->addSpacing(1);
	vboxLayout->addWidget(fblBtn1);
	vboxLayout->addWidget(fblBtn2);
	vboxLayout->addWidget(fblBtn3);

	QHBoxLayout* hboxLayout = new QHBoxLayout;
	hboxLayout->addStretch(30);
	hboxLayout->addLayout(vboxLayout);
	hboxLayout->addStretch(70);

	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->setContentsMargins(20, 30, 0, 0);
	main_layout->setAlignment(Qt::AlignLeft);
	main_layout->addStretch(0);
	main_layout->addWidget(picwidget);
	main_layout->addStretch(0);
	main_layout->addLayout(hboxLayout);
	main_layout->addStretch(1);
	setLayout(main_layout);
#else
	QLabel* picl = new QLabel;
	picl->setFixedSize(116, 100);
	picl->setPixmap(QPixmap(QString(":/sysset_res/800X600-ICON")));

	QLabel* pic2 = new QLabel;
	pic2->setFixedSize(128, 110);
	pic2->setPixmap(QPixmap(QString(":/sysset_res/1024X768-ICON")));

	QLabel* pic3 = new QLabel;
	pic3->setFixedSize(140, 94);
	pic3->setPixmap(QPixmap(QString(":/sysset_res/1280X720-ICON")));

	QLabel* pic4 = new QLabel;
	pic4->setFixedSize(151, 100);
	pic4->setPixmap(QPixmap(QString(":/sysset_res/1336X768-ICON")));

	QLabel* pic5 = new QLabel;
	pic5->setFixedSize(160, 110);
	pic5->setPixmap(QPixmap(QString(":/sysset_res/1920X1080-ICON")));

	QHBoxLayout* hpiclayout1 = new QHBoxLayout;
	hpiclayout1->setAlignment(Qt::AlignTop);
	hpiclayout1->addStretch(30);
	hpiclayout1->addWidget(picl);
	hpiclayout1->addStretch(70);

	QHBoxLayout* hpiclayout2 = new QHBoxLayout;
	hpiclayout2->setAlignment(Qt::AlignTop);
	hpiclayout2->addStretch(30);
	hpiclayout2->addWidget(pic2);
	hpiclayout2->addStretch(70);

	QHBoxLayout* hpiclayout3 = new QHBoxLayout;
	hpiclayout3->setAlignment(Qt::AlignTop);
	hpiclayout3->addStretch(30);
	hpiclayout3->addWidget(pic3);
	hpiclayout3->addStretch(70);

	QHBoxLayout* hpiclayout4 = new QHBoxLayout;
	hpiclayout4->setAlignment(Qt::AlignTop);
	hpiclayout4->addStretch(30);
	hpiclayout4->addWidget(pic4);
	hpiclayout4->addStretch(70);

	QHBoxLayout* hpiclayout5 = new QHBoxLayout;
	hpiclayout5->setAlignment(Qt::AlignTop);
	hpiclayout5->addStretch(30);
	hpiclayout5->addWidget(pic5);
	hpiclayout5->addStretch(70);

	QWidget *setshowwidget1 = new QWidget;
	setshowwidget1->setFixedSize(400, 120);
	setshowwidget1->setLayout(hpiclayout1);

	QWidget *setshowwidget2 = new QWidget;
	setshowwidget2->setFixedSize(400, 120);
	setshowwidget2->setLayout(hpiclayout2);

	QWidget *setshowwidget3 = new QWidget;
	setshowwidget3->setFixedSize(400, 120);
	setshowwidget3->setLayout(hpiclayout3);

	QWidget *setshowwidget4 = new QWidget;
	setshowwidget4->setFixedSize(400, 120);
	setshowwidget4->setLayout(hpiclayout4);

	QWidget *setshowwidget5 = new QWidget;
	setshowwidget5->setFixedSize(400, 120);
	setshowwidget5->setLayout(hpiclayout5);

	QWidget* picwidget = new QWidget;
	picwidget->setFixedSize(400, 120);
	hpiclayout = new QStackedLayout;
	hpiclayout->setContentsMargins(0, 0, 0, 0);
	hpiclayout->addWidget(setshowwidget1);
	hpiclayout->addWidget(setshowwidget2);
	hpiclayout->addWidget(setshowwidget3);
	hpiclayout->addWidget(setshowwidget4);
	hpiclayout->addWidget(setshowwidget5);
	hpiclayout->setCurrentIndex(0);
	picwidget->setLayout(hpiclayout);

	fblBtn = new CFblbutton(this);
	fblBtn->setFixedSize(178, 28);
	fblBtn->setObjectName("fblBtn");
	fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/800X600-down);}");
	connect(fblBtn, SIGNAL(clicked()), SLOT(hideOrShow()));

	CFblbutton *fblBtn1 = new CFblbutton(this);
	fblBtn1->setIndex(0);
	fblBtn1->setFixedSize(178, 28);
	fblBtn1->setObjectName("fblBtn1");
	fblBtn1->setStyleSheet("QPushButton#fblBtn1{border-image: url(:/sysset_res/800X600-down);}"
		"QPushButton#fblBtn1:hover{border-image: url(:/sysset_res/800X600-move);}"
		"QPushButton#fblBtn1:pressed{border-image: url(:/sysset_res/800X600-choose);}");
	fblBtn1->hide();

	connect(fblBtn1, SIGNAL(setFblIndex(int)), this, SLOT(setFblIndex(int)));
	connect(fblBtn1, SIGNAL(clicked()), this, SLOT(chooseFbl()));

	CFblbutton *fblBtn2 = new CFblbutton(this);
	fblBtn2->setIndex(1);
	fblBtn2->setFixedSize(178, 28);
	fblBtn2->setObjectName("fblBtn2");
	fblBtn2->setStyleSheet("QPushButton#fblBtn2{border-image: url(:/sysset_res/1024X768-down);}"
		"QPushButton#fblBtn2:hover{border-image: url(:/sysset_res/1024X768-move);}"
		"QPushButton#fblBtn2:pressed{border-image: url(:/sysset_res/1024X768-choose);}");
	fblBtn2->hide();

	connect(fblBtn2, SIGNAL(setFblIndex(int)), this, SLOT(setFblIndex(int)));
	connect(fblBtn2, SIGNAL(clicked()), this, SLOT(chooseFbl()));

	CFblbutton *fblBtn3 = new CFblbutton(this);
	fblBtn3->setIndex(2);
	fblBtn3->setFixedSize(178, 28);
	fblBtn3->setObjectName("fblBtn3");
	fblBtn3->setStyleSheet("QPushButton#fblBtn3{border-image: url(:/sysset_res/1280X720-down);}"
		"QPushButton#fblBtn3:hover{border-image: url(:/sysset_res/1280X720-move);}"
		"QPushButton#fblBtn3:pressed{border-image: url(:/sysset_res/1280X720-choose);}");
	fblBtn3->hide();
	connect(fblBtn3, SIGNAL(setFblIndex(int)), this, SLOT(setFblIndex(int)));
	connect(fblBtn3, SIGNAL(clicked()), this, SLOT(chooseFbl()));

	CFblbutton *fblBtn4 = new CFblbutton(this);
	fblBtn4->setIndex(3);
	fblBtn4->setFixedSize(178, 28);
	fblBtn4->setObjectName("fblBtn4");
	fblBtn4->setStyleSheet("QPushButton#fblBtn4{border-image: url(:/sysset_res/1336X768-down);}"
		"QPushButton#fblBtn4:hover{border-image: url(:/sysset_res/1336X768-move);}"
		"QPushButton#fblBtn4:pressed{border-image: url(:/sysset_res/1336X768-choose);}");
	fblBtn4->hide();

	connect(fblBtn4, SIGNAL(setFblIndex(int)), this, SLOT(setFblIndex(int)));
	connect(fblBtn4, SIGNAL(clicked()), this, SLOT(chooseFbl()));

	CFblbutton *fblBtn5 = new CFblbutton(this);
	fblBtn5->setIndex(4);
	fblBtn5->setFixedSize(178, 28);
	fblBtn5->setObjectName("fblBtn5");
	fblBtn5->setStyleSheet("QPushButton#fblBtn5{border-image: url(:/sysset_res/1920X1080-down);}"
		"QPushButton#fblBtn5:hover{border-image: url(:/sysset_res/1920X1080-move);}"
		"QPushButton#fblBtn5:pressed{border-image: url(:/sysset_res/1920X1080-choose);}");
	fblBtn5->hide();

	connect(fblBtn5, SIGNAL(setFblIndex(int)), this, SLOT(setFblIndex(int)));
	connect(fblBtn5, SIGNAL(clicked()), this, SLOT(chooseFbl()));

	fblBtn_Vec.push_back(fblBtn);
	fblBtn_Vec.push_back(fblBtn1);
	fblBtn_Vec.push_back(fblBtn2);
	fblBtn_Vec.push_back(fblBtn3);
	fblBtn_Vec.push_back(fblBtn4);
	fblBtn_Vec.push_back(fblBtn5);

	QVBoxLayout* vboxLayout = new QVBoxLayout;
	vboxLayout->setSpacing(0);
	vboxLayout->setAlignment(Qt::AlignLeft);
	vboxLayout->addWidget(fblBtn);
	vboxLayout->addSpacing(1);
	vboxLayout->addWidget(fblBtn1);
	vboxLayout->addWidget(fblBtn2);
	vboxLayout->addWidget(fblBtn3);
	vboxLayout->addWidget(fblBtn4);
	vboxLayout->addWidget(fblBtn5);

	QHBoxLayout* hboxLayout = new QHBoxLayout;
	hboxLayout->addStretch(30);
	hboxLayout->addLayout(vboxLayout);
	hboxLayout->addStretch(70);

	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->setContentsMargins(20, 30, 0, 0);
	main_layout->setAlignment(Qt::AlignLeft);
	main_layout->addStretch(0);
	main_layout->addWidget(picwidget);
	main_layout->addStretch(0);
	main_layout->addLayout(hboxLayout);
	main_layout->addStretch(1);
	setLayout(main_layout);
#endif
}

void fblSetWidget::hideOrShow()
{
	if (isfblBtnShow)
	{
		for (int i = 1; i < fblBtn_Vec.size(); i++)
		{
			fblBtn_Vec[i]->hide();
		}
	}
	else
	{
		for (int i = 1; i < fblBtn_Vec.size(); i++)
		{
			fblBtn_Vec[i]->show();
		}
	}
	isfblBtnShow = !isfblBtnShow;
}


fblSetWidget::~fblSetWidget()
{

}

void fblSetWidget::setFblIndex(int index)
{
	hpiclayout->setCurrentIndex(index);
}

void fblSetWidget::chooseFbl()
{
#ifdef ZS
	CFblbutton *btn = (CFblbutton *)sender();
	int index = btn->getIndex();
	int  i_fbl;
	for (int i = 1; i < fblBtn_Vec.size(); i++)
	{
		fblBtn_Vec[i]->hide();
	}
	isfblBtnShow = false;
	QString str_Tmp = "echo %1 >>  /tmp/resolution_fifo";
	QString str_Fbl;
	switch (index)
	{
	case 0:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/800X600-down);}");
		str_Tmp = str_Tmp.arg(17);
		str_Fbl = "800X600";
		i_fbl = 17;
		break;
	case 1:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1024X768-down);}");
		str_Tmp = str_Tmp.arg(31);
		str_Fbl = "1024X768";
		i_fbl = 31;
		break;
	case 2:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1336X768-down);}");
		str_Tmp = str_Tmp.arg(2);
		str_Fbl = "1336X768";
		i_fbl = 2;
		break;
	default:
		break;
	}
	fbl_message_box * msg_box = new fbl_message_box(QStringLiteral("应用分辨率%1并重启？").arg(str_Fbl));
	msg_box->setFontColor(FBLMSGFONTWHITE);
	msg_box->setCmd(str_Tmp);
	msg_box->show();
#else
	CFblbutton *btn = (CFblbutton *)sender();
	int index = btn->getIndex();
	int  i_fbl;
	for (int i = 1; i < fblBtn_Vec.size(); i++)
	{
		fblBtn_Vec[i]->hide();
	}
	isfblBtnShow = false;
	QString str_Tmp = "echo %1 >>  /tmp/resolution_fifo";
	QString str_Fbl;
	switch (index)
	{
	case 0:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/800X600-down);}");
		str_Tmp = str_Tmp.arg(17);
		str_Fbl = "800X600";
		i_fbl = 17;
		break;
	case 1:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1024X768-down);}");
		str_Tmp = str_Tmp.arg(31);
		str_Fbl = "1024X768";
		i_fbl = 31;
		break;
	case 2:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1280X720-down);}");
		str_Tmp = str_Tmp.arg(4);
		str_Fbl = "1280X720";
		i_fbl = 4;
		break;
	case 3:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1336X768-down);}");
		str_Tmp = str_Tmp.arg(2);
		str_Fbl = "1336X768";
		i_fbl = 2;
		break;
	case 4:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1920X1080-down);}");
		str_Tmp = str_Tmp.arg(16);
		str_Fbl = "1920X1080";
		i_fbl = 16;
		break;
	default:
		break;
	}
	fbl_message_box * msg_box = new fbl_message_box(QStringLiteral("应用分辨率%1并重启？").arg(str_Fbl));
	msg_box->setFontColor(FBLMSGFONTWHITE);
   msg_box->setCmd(str_Tmp);
	msg_box->show();
#endif
}

void fblSetWidget::initView()
{
#ifdef ZS
	int currentScreenWidth = QApplication::desktop()->width();
    int currentScreenHeight = QApplication::desktop()->height();
	int i = 17;
	if(currentScreenWidth == 800)
	{
		i = 17;
	}
	if(currentScreenWidth == 1024)
        {
		i = 31;
        }
	if(currentScreenWidth == 1366)
        {
		i = 2;
        }
	switch (i)
	{
	case 17:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/800X600-down);}");
		hpiclayout->setCurrentIndex(0);
		break;
	case 31:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1024X768-down);}");
		hpiclayout->setCurrentIndex(1);
		break;
	case 2:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1336X768-down);}");
		hpiclayout->setCurrentIndex(2);
		break;
	default:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/800X600-down);}");
		hpiclayout->setCurrentIndex(0);
		break;
	}
	for (size_t i = 1; i < fblBtn_Vec.size(); i++)
	{
		fblBtn_Vec[i]->hide();
	}
	isfblBtnShow = false;
#else
	int currentScreenWidth = QApplication::desktop()->width();
    int currentScreenHeight = QApplication::desktop()->height();
	printf("currentScreenWidth = %d\n",currentScreenWidth);
	int i = 17;
	if(currentScreenWidth == 800)
	{
		i = 17;
	}
	if(currentScreenWidth == 1024)
        {
		i = 31;
        }
	if(currentScreenWidth == 1280)
        {
		i = 4;
        }
	if(currentScreenWidth == 1366)
        {
		i = 2;
        }
	if(currentScreenWidth == 1920)
        {
                i = 16;
        }
	
	printf("i = %d............\n",i);
	switch (i)
	{
	case 17:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/800X600-down);}");
		hpiclayout->setCurrentIndex(0);
		break;
	case 31:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1024X768-down);}");
		hpiclayout->setCurrentIndex(1);
		break;
	case 4:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1280X720-down);}");
		hpiclayout->setCurrentIndex(2);
		break;
	case 2:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1336X768-down);}");
		hpiclayout->setCurrentIndex(3);
		break;
	case 16:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1920X1080-down);}");
		hpiclayout->setCurrentIndex(4);
		break;
	default:
		fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/800X600-down);}");
		hpiclayout->setCurrentIndex(0);
		break;
	}
	for (size_t i = 1; i < fblBtn_Vec.size(); i++)
	{
		fblBtn_Vec[i]->hide();
	}
	isfblBtnShow = false;
#endif
}



