#include "cuserpicpanel.h"
#include <QGridLayout>
#include "cusercard.h"
#include "mainwindow.h"
CSiglePic* CSiglePic::curpic = NULL;

CUserCard* g_card = NULL;
CUserPicPanel* panel = NULL;
CUserPicPanel::CUserPicPanel(QWidget *parent,CUserCard* card)
	: QWidget(parent)
{
	this->setFixedSize(296, 154);
	g_card = card;
	setAutoFillBackground(true);

	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(":/userpic/picbg")));
	this->setPalette(palette);


	QGridLayout* main_layout = new QGridLayout;
	main_layout->setContentsMargins(46,46,46,36);


	for (int i = 0; i < 12; i++)
	{
		CSiglePic* pic = new CSiglePic;
		pic->setFixedSize(32, 32);
		pic->setPixmap(QPixmap(QString(":/userpic/p%1").arg(i + 1)).scaled(QSize(32, 32), Qt::IgnoreAspectRatio));
		pic->setPixIndex(i);
		main_layout->addWidget(pic, i / 6, i % 6);
	}
	
	this->setLayout(main_layout);


	this->setWindowFlags(Qt::FramelessWindowHint);

	panel = this;

}

CUserPicPanel::~CUserPicPanel()
{

}

CSiglePic::CSiglePic(QWidget *parent /*= 0*/)
{
	piclabel = new MyLabel(this);
	curpic = 0;
	PicIndex = 0;
	piclabel->setGeometry(0, 0, 32, 32);
	
	ismidselected = 0;
	piclabel->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgba(88,178, 255,100);");

	connect(piclabel, SIGNAL(clicked()), this, SLOT(clickfunc()));
}

void CSiglePic::setPixmap(QPixmap pix)
{
	curpix = pix;
	piclabel->setPixmap(pix);
}

void CSiglePic::clickfunc()
{
	if (!ismidselected)
	{
		piclabel->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgb(88,178, 255);");
		ismidselected = 1;

		if ((CSiglePic::GetCurSelectPic() != NULL)
			&& CSiglePic::GetCurSelectPic() != this)
		{
			CSiglePic::GetCurSelectPic()->clickfunc();
		}
		CSiglePic::SetCurSelectPic(this);
		return;
	}
	if (ismidselected)
	{
		piclabel->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgba(88,178, 255,100);");
		ismidselected = 0;
		CSiglePic::SetCurSelectPic(NULL);
		return;
	}
}

CSiglePic::~CSiglePic()
{

}

void CSiglePic::SetCurSelectPic(CSiglePic* pic)
{
	
	curpic = pic;
	if (curpic)
	{
		
		if (g_card)
		{
			QString fmt(":/userpic/p%1"); 
			QString str = fmt.arg(curpic->getPixIndex() + 1);
			g_card->setPixmap((str));
			g_card->main_win->picpath = str;
			if (panel)
			{
				panel->hide();
				g_card->midclickfunc();
				g_card->isshow = 0;
				g_card->midpic->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgba(88,178, 255,100);");
			}



		}

	}
}

CSiglePic* CSiglePic::GetCurSelectPic()
{
	return curpic;
}

void CSiglePic::setPixIndex(int i)
{
	PicIndex = i;
}