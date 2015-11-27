#include "cusercard.h"
#include "clabel.h"
#include <QPushButton>
#include "cuserpicpanel.h"
#include "mainwindow.h"

CUserCard::CUserCard(int state, QWidget *parent, int noedit, int nodel, int hoverstate, int borderstate, int isaddusercd,int islogin)
	: QWidget(parent)
{
	islogincard = islogin;
	isaddusercard = isaddusercd;
	isshow = 0;
	panel = NULL;
	closebtn = NULL;
	editbtn = NULL;
	parentwidget = parent;
	noclickstate = state;
	this->setMinimumSize(77, 77+30);
	ismidselected = 0;
	midpic = new MyLabel(this);
	midpic->setPixmap(QPixmap(QString(":/usercard/defaultpic")));
	midpic->setGeometry(0, 7+10, 70, 70);
	midpic->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgba(88,178, 255,100);");
	midpic->setMouseTracking(true);
	midpic->installEventFilter(this);
	connect(midpic, SIGNAL(clicked()), this, SLOT(midclickfunc()));


	username = new QLabel(this);
	username->setFixedSize(65, 15);
	username->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:15px;border:0px;QToolTip{background-color:white}");
	username->setAlignment(Qt::AlignCenter);

	if (!nodel)
	{
		closebtn = new QPushButton(this);
		closebtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
		closebtn->setIcon(QIcon(QString(":/usercard/delnomal")));
		closebtn->setIconSize(QSize(14, 14));
		closebtn->setGeometry(63, 0+10, 14, 14);
		connect(closebtn, SIGNAL(pressed()), this, SLOT(clspressfunc()));
		connect(closebtn, SIGNAL(released()), this, SLOT(clsreleasefunc()));
		closebtn->hide();
	}

	if (!noedit)
	{
		editbtn = new QPushButton(this);
		editbtn->setIcon(QIcon(QString(":/usercard/edit")));
		editbtn->setGeometry(70 - 17, 10 + 77 - 17, 17, 17);
		//editbtn->setStyleSheet("background-color:transparent");

// 		QPalette palette;
// 		palette.setBrush(QPalette::Background, QBrush(QPixmap(QString(":/usercard/edit"))));
// 		editbtn->setPalette(palette);


		connect(editbtn, SIGNAL(pressed()), this, SLOT(editpressfunc()));
		connect(editbtn, SIGNAL(released()), this, SLOT(editreleasefunc()));
		editbtn->hide();
	}
	
	this->borderstate = borderstate;

	setMouseTracking(true);

	installEventFilter(this);
}

CUserCard::~CUserCard()
{

}

void CUserCard::setPixmap(QString pix, int height /*= 110*/, int width /*= 110*/)
{
	strpixpath = pix;
	midpic->setPixmap(QPixmap(pix));
}

void CUserCard::clsreleasefunc()
{
	if (!closebtn)
	{
		return;
	}
	closebtn->setIcon(QIcon(QString(":/usercard/delnomal")));

	main_win->deletecard(this);



}

void CUserCard::clspressfunc()
{
	if (!closebtn)
	{
		return;
	}
	closebtn->setIcon(QIcon(QString(":/usercard/delclicked")));
}

void CUserCard::editpressfunc()
{
	if (!editbtn)
	{
		return;
	}
	editbtn->setIcon(QIcon(QString(":/usercard/editselected")));
}

void CUserCard::editreleasefunc()
{
	if (!editbtn)
	{
		return;
	}
	editbtn->setIcon(QIcon(QString(":/usercard/edit")));
	if (!panel)
	{
		panel  = new CUserPicPanel(parentwidget,this);
	}
	
	if (!isshow)
	{
		QRect rc = this->geometry();
		panel->setGeometry(rc.x() - rc.width() / 2, rc.y() + rc.height() - 2 - 20, 296, 154);
		panel->show();
		isshow = 1;
	}
	else
	{
		panel->hide(); 
		isshow = 0;
	}

	
}

void CUserCard::midclickfunc()
{
	if (isaddusercard)
	{
		main_win->adduser();
	}
	if (islogincard)
	{
		main_win->secondwuserpic->setPixmap(strpixpath);
		main_win->edit->inputEdit->setText("");
		main_win->secondwuserpic->setText(struname);
		main_win->uname = struname;
		main_win->changeWidgetTo(main_win->secondLoginWidget);
	}
	
}

void CUserCard::setText(QString str)
{
	username->setText(str);
	username->setToolTip(str);
	/*int size = str.length();
	size = size * 10;
	if (size < 70)
	{
		username->setGeometry((71 - size) / 2, 77 + 15, 65, 15);
	}
	else
	{
		username->setGeometry(3, 77 + 15, 65, 15);

	}
	*/
	username->setGeometry(3,77+15,65,15);
	

}
void CUserCard::enterEvent(QEvent *)
{
	if (closebtn)
	{
		closebtn->show();
	}

	if (editbtn)
	{
		editbtn->show();
	}

	if (borderstate)
	{
		midpic->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgb(88,178, 255);");
	}
}

void CUserCard::leaveEvent(QEvent *)
{
	if (closebtn)
	{
		closebtn->hide();
	}

	if (editbtn)
	{
		editbtn->hide();
	}

	if (borderstate)
	{
		midpic->setStyleSheet("border-width: 1px;   border-style: solid;   border-color: rgba(88,178, 255,100);");
		
	}
}

QString CUserCard::GetPixPath()
{
	return strpixpath;
}

void CUserCard::SetMainW(CMainWindow* main_w)
{
	main_win = main_w;
}
