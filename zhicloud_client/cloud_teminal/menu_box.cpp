#include "cuserpicpanel.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QGridLayout>
#include "cusercard.h"
#include "mainwindow.h"
#include "menu_box.h"
CSiglePic2* CSiglePic2::curpic = NULL;

CUserCard* g_card2 = NULL;
ZCMenuBox* panel3 = NULL;
ZCMenuBox::ZCMenuBox(QString message, QWidget *parent, CUserCard* card, int whichop)
	: QDialog(parent)
{
	op = whichop;
	this->setFixedSize(188, 144);
	g_card2 = card;
	setAutoFillBackground(true);

	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(":/dialog_res/dialogBackground")));
	this->setPalette(palette);


	QGridLayout* main_layout = new QGridLayout;
	main_layout->setContentsMargins(46,46,46,36);


	for (int i = 12; i < 12; i++)
	{
		CSiglePic2* pic = new CSiglePic2;
		pic->setFixedSize(32, 32);
		pic->setPixmap(QPixmap(QString(":/userpic/p%1").arg(i + 1)).scaled(QSize(32, 32), Qt::IgnoreAspectRatio));
		pic->setPixIndex(i);
		main_layout->addWidget(pic, i / 6, i % 6);
	}
	
	this->setLayout(main_layout);


	//this->setWindowFlags(Qt::FramelessWindowHint);
	this->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

	panel3 = this;

	QDesktopWidget* desktop = QApplication::desktop();
	QRect deskrect = desktop->availableGeometry();
	//printfQStringLiteral("--------------------msgbox: %d\n", deskrect.width());
	//居中显示
	this->move((deskrect.width() - this->width())/2, (deskrect.height() - this->height())/2);

	isokbutton = -1;
	dialogLabel = new QLabel(this);
        dialogLabel->setGeometry(0,70, 188, 20);
        dialogLabel->setStyleSheet("font-size:12.5px");
        dialogLabel->setText(message);
        dialogLabel->setAlignment(Qt::AlignHCenter);
        QPalette palette2;
        palette2.setColor(QPalette::WindowText, QColor(232, 86, 86));
        dialogLabel->setPalette(palette2);

        //È·¶¨°´Å¥
		okButton = new QPushButton(this);
		okButton->setGeometry(92, 107, 70, 30);
		okButton->setObjectName("okbtn");
		okButton->setStyleSheet("QPushButton#okbtn{border-image: url(:/changepwd/surenomal);}"
                "QPushButton#okbtn:pressed{border-image: url(:/changepwd/sureclicked);}");
		cancelButton = new QPushButton(this);
		cancelButton->setGeometry(12, 107, 70, 30);
		cancelButton->setObjectName("cancelbtn");
		cancelButton->setStyleSheet("QPushButton#cancelbtn{border-image: url(:/changepwd/retnomal);}"
			"QPushButton#cancelbtn:pressed{border-image: url(:/changepwd/retclicked);}");
		connect(cancelButton, SIGNAL(clicked()), SLOT(closeDialogSlot()));
		connect(okButton, SIGNAL(clicked()), SLOT(okDialogSlot()));
}

ZCMenuBox::~ZCMenuBox()
{

}

CSiglePic2::CSiglePic2(QWidget *parent /*= 0*/)
{
	piclabel = new MyLabel(this);
	curpic = 0;
	PicIndex = 0;
	piclabel->setGeometry(0, 0, 32, 32);
	
	ismidselected = 0;
	piclabel->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgba(88,178, 255,100);");

	connect(piclabel, SIGNAL(clicked()), this, SLOT(clickfunc()));
}

void CSiglePic2::setPixmap(QPixmap pix)
{
	curpix = pix;
	piclabel->setPixmap(pix);
}

void CSiglePic2::clickfunc()
{
	if (!ismidselected)
	{
		piclabel->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgb(88,178, 255);");
		ismidselected = 1;

		if ((CSiglePic2::GetCurSelectPic() != NULL)
			&& CSiglePic2::GetCurSelectPic() != this)
		{
			CSiglePic2::GetCurSelectPic()->clickfunc();
		}
		CSiglePic2::SetCurSelectPic(this);
		return;
	}
	if (ismidselected)
	{
		piclabel->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgba(88,178, 255,100);");
		ismidselected = 0;
		CSiglePic2::SetCurSelectPic(NULL);
		return;
	}
}

CSiglePic2::~CSiglePic2()
{

}

void CSiglePic2::SetCurSelectPic(CSiglePic2* pic)
{
	
	curpic = pic;
	if (curpic)
	{
		
		if (g_card2)
		{
			QString fmt(":/userpic/p%1"); 
			QString str = fmt.arg(curpic->getPixIndex() + 1);
			g_card2->setPixmap((str));
			g_card2->main_win->picpath = str;
			if (panel3)
			{
				panel3->hide();
				g_card2->midclickfunc();
				g_card2->isshow = 0;
				g_card2->midpic->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgba(88,178, 255,100);");
			}



		}

	}
}

CSiglePic2* CSiglePic2::GetCurSelectPic()
{
	return curpic;
}

void CSiglePic2::setPixIndex(int i)
{
	PicIndex = i;
}

void ZCMenuBox::closeDialogSlot()
{
	isokbutton = 2;
	close();
}

void ZCMenuBox::okDialogSlot()
{
	isokbutton = 1;

	if (op == 1)
	{
		main_win->menushutdown();
	}
	if (op == 2)
	{
		main_win->menureboot();
	}
	if (op == 3)
	{
		main_win->menuexit();
	}
	if (op == 4)
	{
		main_win->changeuser();
	}
	close();
}

int ZCMenuBox::isOk()
{
	return isokbutton;
}

void ZCMenuBox::setWinMain(CMainWindow* mainw)
{
	main_win = mainw;
}
