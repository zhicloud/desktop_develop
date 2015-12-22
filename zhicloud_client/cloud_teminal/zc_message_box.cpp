#include "cuserpicpanel.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QGridLayout>
#include "cusercard.h"
#include "mainwindow.h"
CSiglePic1* CSiglePic1::curpic = NULL;

CUserCard* g_card3 = NULL;
ZCMessageBox* panel2 = NULL;
ZCMessageBox::ZCMessageBox(QString message, QWidget *parent,CUserCard* card)
	: QDialog(parent)
{
	m_win = (CMainWindow*)parent;
	m_isClose = false;
	this->setFixedSize(188, 144);
	g_card3 = card;
	setAutoFillBackground(true);

	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(":/dialog_res/dialogBackground")));
	this->setPalette(palette);


	QGridLayout* main_layout = new QGridLayout;
	main_layout->setContentsMargins(46,46,46,36);


	for (int i = 12; i < 12; i++)
	{
		CSiglePic1* pic = new CSiglePic1;
		pic->setFixedSize(32, 32);
		pic->setPixmap(QPixmap(QString(":/userpic/p%1").arg(i + 1)).scaled(QSize(32, 32), Qt::IgnoreAspectRatio));
		pic->setPixIndex(i);
		main_layout->addWidget(pic, i / 6, i % 6);
	}
	
	this->setLayout(main_layout);


	this->setWindowFlags(Qt::FramelessWindowHint);

	panel2 = this;

	QDesktopWidget* desktop = QApplication::desktop();
	QRect deskrect = desktop->availableGeometry();
	//printfQStringLiteral("--------------------msgbox: %d\n", deskrect.width());
	//居中显示
	this->move((deskrect.width() - this->width())/2, (deskrect.height() - this->height())/2);

	dialogLabel = new QLabel(this);
        dialogLabel->setGeometry(0,70, 188, 20);
        dialogLabel->setStyleSheet("font-size:12.5px");
        dialogLabel->setText(message);
        dialogLabel->setAlignment(Qt::AlignHCenter);
        QPalette palette2;
        palette2.setColor(QPalette::WindowText, QColor(232, 86, 86));
        dialogLabel->setPalette(palette2);

        //È·¶¨°´Å¥
        dialogButton = new QPushButton(this);
        dialogButton->setGeometry(12, 107, 164, 24);
        dialogButton->setObjectName("btn");
        dialogButton->setStyleSheet("QPushButton#btn{border-image: url(:/dialog_res/dialogButton);}"
                "QPushButton#btn:pressed{border-image: url(:/dialog_res/dialogButton_pressed);}");
	connect(dialogButton,SIGNAL(clicked()),SLOT(closeDialogSlot()));
}

ZCMessageBox::~ZCMessageBox()
{

}

CSiglePic1::CSiglePic1(QWidget *parent /*= 0*/)
{
	piclabel = new MyLabel(this);
	curpic = 0;
	PicIndex = 0;
	piclabel->setGeometry(0, 0, 32, 32);
	
	ismidselected = 0;
	piclabel->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgba(88,178, 255,100);");

	connect(piclabel, SIGNAL(clicked()), this, SLOT(clickfunc()));
}

void CSiglePic1::setPixmap(QPixmap pix)
{
	curpix = pix;
	piclabel->setPixmap(pix);
}

void CSiglePic1::clickfunc()
{
	if (!ismidselected)
	{
		piclabel->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgb(88,178, 255);");
		ismidselected = 1;

		if ((CSiglePic1::GetCurSelectPic() != NULL)
			&& CSiglePic1::GetCurSelectPic() != this)
		{
			CSiglePic1::GetCurSelectPic()->clickfunc();
		}
		CSiglePic1::SetCurSelectPic(this);
		return;
	}
	if (ismidselected)
	{
		piclabel->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgba(88,178, 255,100);");
		ismidselected = 0;
		CSiglePic1::SetCurSelectPic(NULL);
		return;
	}
}

CSiglePic1::~CSiglePic1()
{

}

void CSiglePic1::SetCurSelectPic(CSiglePic1* pic)
{
	
	curpic = pic;
	if (curpic)
	{
		
		if (g_card3)
		{
			QString fmt(":/userpic/p%1"); 
			QString str = fmt.arg(curpic->getPixIndex() + 1);
			g_card3->setPixmap((str));
			g_card3->main_win->picpath = str;
			if (panel2)
			{
				panel2->hide();
				g_card3->midclickfunc();
				g_card3->isshow = 0;
				g_card3->midpic->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgba(88,178, 255,100);");
			}



		}

	}
}

CSiglePic1* CSiglePic1::GetCurSelectPic()
{
	return curpic;
}

void CSiglePic1::setPixIndex(int i)
{
	PicIndex = i;
}

void ZCMessageBox::closeDialogSlot()
{
	if (NULL == dialogButton)
	{
		delete dialogButton;
		dialogButton = NULL;
	}
	if (NULL == dialogLabel)
	{
		delete dialogLabel;
		dialogLabel = NULL;
	}
	m_isClose = true;
	close();
}

bool ZCMessageBox::isClose()
{
	return m_isClose;
}

void ZCMessageBox::setFontColor(ZCMFONTCOLOR color)
{
	QPalette palette2;
	if (color == ZCMFONTRED)
	{
		palette2.setColor(QPalette::WindowText, QColor(232, 86, 86));
		dialogLabel->setPalette(palette2);
	}
	else
	{
		palette2.setColor(QPalette::WindowText, Qt::green);
		dialogLabel->setPalette(palette2);
	}
}
