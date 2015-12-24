#include "cmenuwidget.h"
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include "mainwindow.h"
#include "SpiceMulViewer.h"
#include <QDesktopWidget>
#include <memory>
#include "menu_box.h"

CMenuWidget* g_menu_ptr = NULL;

CMenuWidget::CMenuWidget(QWidget *parent, CMainWindow* main_win)
	: QWidget(parent)
{
	toolbox = NULL;
	g_menu_ptr = this;
	feedbackwid = NULL;
	mailwid = NULL;
	main_w = main_win;
	this->setFixedSize(648, 52);
	setAutoFillBackground(true);
	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(QString(":/menu/bg"))));
	this->setPalette(palette);


	logo = new QLabel;
	name = new QLabel;

	mail = new QPushButton;
	QPushButton* toolbox = new QPushButton;

	QPushButton* changeuser = new QPushButton;
	QPushButton* reboot = new QPushButton;
	QPushButton* shutd = new QPushButton;
	QPushButton* exit = new QPushButton;
	mail->setToolTip(QStringLiteral("邮件"));
	toolbox->setToolTip(QStringLiteral("工具箱"));
	changeuser->setToolTip(QStringLiteral("切换用户"));
	reboot->setToolTip(QStringLiteral("重启"));
	shutd->setToolTip(QStringLiteral("关机"));
	exit->setToolTip(QStringLiteral("退出"));

	logo->setFixedSize(16,16);
	logo->setPixmap(QPixmap(QString(":/menu/logo")));

	name->setFixedSize(150, 12);
	name->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
	name->setText("929015200@qq.com");

	mail->setFixedSize(26,18);
	mail->setObjectName("mail");
	if (main_win->ishasmail)
	{
		mail_status = 1;
		mail->setStyleSheet("QPushButton#mail{border-image: url(:/menu/msghave);}"
			"QPushButton#mail:hover{border-image: url(:/menu/msghave);}"
			"QPushButton#mail:pressed{border-image: url(:/menu/msghaveclick);}");
	}
	else
	{
		mail_status = 0;
		mail->setStyleSheet("QPushButton#mail{border-image: url(:/menu/msgno);}"
			"QPushButton#mail:hover{border-image: url(:/menu/msgno);}"
			"QPushButton#mail:pressed{border-image: url(:/menu/msgnoclick);}");

	}
	
	connect(mail, SIGNAL(clicked()), this, SLOT(clickmail()));

	toolbox->setFixedSize(15, 11);
	toolbox->setObjectName("toolbox");
	toolbox->setStyleSheet("QPushButton#toolbox{border-image: url(:/toolbox/toolboxnomal);}"
		"QPushButton#toolbox:hover{border-image: url(:/toolbox/toolboxnomal);}"
		"QPushButton#toolbox:pressed{border-image: url(:/toolbox/toolboxclick);}");
	connect(toolbox, SIGNAL(clicked()), this, SLOT(clicktoolbox()));

	changeuser->setFixedSize(66, 12);
	changeuser->setObjectName("changeuser");
	changeuser->setStyleSheet("QPushButton#changeuser{border-image: url(:/menu/cunomal);}"
		"QPushButton#changeuser:hover{border-image: url(:/menu/cunomal);}"
		"QPushButton#changeuser:pressed{border-image: url(:/menu/cuclick);}");

	connect(changeuser, SIGNAL(clicked()), this, SLOT(clickchangeuser()));
	

	reboot->setFixedSize(40, 12);
	reboot->setObjectName("reboot");
	reboot->setStyleSheet("QPushButton#reboot{border-image: url(:/menu/rebootnomal);}"
		"QPushButton#reboot:hover{border-image: url(:/menu/rebootnomal);}"
		"QPushButton#reboot:pressed{border-image: url(:/menu/rebootclick);}");

	connect(reboot, SIGNAL(clicked()), this, SLOT(clickreboot()));

	shutd->setFixedSize(40, 12);
	shutd->setObjectName("shutd");
	shutd->setStyleSheet("QPushButton#shutd{border-image: url(:/menu/shutnomal);}"
		"QPushButton#shutd:hover{border-image: url(:/menu/shutnomal);}"
		"QPushButton#shutd:pressed{border-image: url(:/menu/shutdclick);}");

	connect(shutd, SIGNAL(clicked()), this, SLOT(clickshutdown()));

	exit->setFixedSize(40, 12);
	exit->setObjectName("exit");
	exit->setStyleSheet("QPushButton#exit{border-image: url(:/menu/exitnomal);}"
		"QPushButton#exit:hover{border-image: url(:/menu/exitnomal);}"
		"QPushButton#exit:pressed{border-image: url(:/menu/exitclick);}");

	connect(exit, SIGNAL(clicked()), this, SLOT(clickexit()));

	QHBoxLayout* main_layout = new QHBoxLayout;
	main_layout->setAlignment(Qt::AlignCenter);
	main_layout->setContentsMargins(40, 0, 40, 10);	
	main_layout->addWidget(logo);
	main_layout->addWidget(name);
	main_layout->addWidget(mail);
	main_layout->addSpacing(20);
	main_layout->addWidget(toolbox);
	main_layout->addStretch();
	main_layout->addWidget(changeuser);
	main_layout->addSpacing(20);
	main_layout->addWidget(reboot);
	main_layout->addSpacing(20);
	main_layout->addWidget(shutd);
	main_layout->addSpacing(20);
	main_layout->addWidget(exit);

	this->setLayout(main_layout);


	installEventFilter(this);

	setMouseTracking(true);
}

CMenuWidget::~CMenuWidget()
{
	g_menu_ptr = NULL;
}

void CMenuWidget::clickchangeuser()
{
	ZCMenuBox *msgbox = new ZCMenuBox(QString("确定切换用户吗？"), NULL, NULL,4);
	msgbox->setWinMain(main_w);
	msgbox->exec();

}

void CMenuWidget::setInfo(QString uname, QString picpath)
{
	//logo->setPixmap(QPixmap(picpath).scaled(QSize(14,14)));
	name->setText(uname);
}

void CMenuWidget::clickexit()
{
	ZCMenuBox *msgbox = new ZCMenuBox(QString("确定退出吗？"), NULL, NULL, 3);
	//ZCMenuBox *msgbox = new ZCMenuBox(QString(""), NULL, NULL, 3);
	msgbox->setWinMain(main_w);
	msgbox->exec();
}

void CMenuWidget::clickreboot()
{
	ZCMenuBox *msgbox = new ZCMenuBox(QString("确定重启吗？"), NULL, NULL,2);
	msgbox->setWinMain(main_w);
	msgbox->exec();
}

void CMenuWidget::clickshutdown()
{
	
	ZCMenuBox *msgbox = new ZCMenuBox(QString("确定关机吗？"), NULL, NULL,1);
	msgbox->setWinMain(main_w);
	msgbox->exec();
}

void CMenuWidget::clickmail()
{
	main_w->ishasmail = 0;
	if (!mailwid)
	{
		mailwid = new CMailBoxWidget(main_w);
	}
	mailwid->ShowMail();
	mailwid->show();

	//mail->setStyleSheet("QPushButton#mail{border-image: url(:/menu/msgno);}"
	//	"QPushButton#mail:hover{border-image: url(:/menu/msgno);}"
	//	"QPushButton#mail:pressed{border-image: url(:/menu/msgnoclick);}");
}

void CMenuWidget::clicktoolbox()
{
	

	if (!toolbox)
	{
		toolbox = new CToolBox(main_w);
		toolbox->setUnamePwd(main_w->lastsucuname,main_w->lastsucupwd);
		connect(toolbox,SIGNAL(closeSignal()),this,SLOT(viewerGrabSlot()));
	}
	toolbox->grabKey();
	toolbox->show();
}
void CMenuWidget::delBtn()
{
	delete mailwid;
	mailwid = NULL;
	delete toolbox;
	toolbox = NULL;	
}

void CMenuWidget::viewerGrabSlot()
{
	m_viewer->grabKeyboard();
}

void CMenuWidget::setViewer(CSpiceMultVEx *viewer)
{
	m_viewer = viewer;
}
