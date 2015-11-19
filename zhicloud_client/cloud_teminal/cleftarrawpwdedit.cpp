#include "cleftarrawpwdedit.h"

#include <QPushButton>
#include "mainwindow.h"
#include <QHBoxLayout>
CLeftArrawPWDEdit::CLeftArrawPWDEdit(QWidget *parent)
	: QWidget(parent)
{
	this->setFixedSize(190, 12);
	setAutoFillBackground(true);
	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(QString(":/first_login_res/input"))));
	this->setPalette(palette);
	inputEdit = new QLineEdit();
	inputEdit->setStyleSheet("background-color:transparent;color:white;font-size:12px;border:0px;");
	inputEdit->setEchoMode(QLineEdit::Password);
	inputEdit->setFixedSize(180,12);
	

// 	QPushButton* arraw = new QPushButton;
// 	arraw->setFixedSize(10,10);
// 	arraw->setIcon(QIcon(QString(":/second_logres/inputpwd")));
// 	arraw->setObjectName("arraw");
// 	
// 	arraw->setStyleSheet("QPushButton#arraw{border-image: url(:/second_logres/uninputpwd);}"
// 		"QPushButton#arraw:hover{border-image: url(:/second_logres/inputpwd);}"
// 		"QPushButton#arraw:pressed{border-image: url(:/second_logres/pwdarrowclicked);}");
// 	arraw->setToolTip(QStringLiteral("登陆"));
// 	connect(arraw, SIGNAL(clicked()), this, SLOT(login()));
// 
 	QHBoxLayout* main_layout = new QHBoxLayout;
 	main_layout->addWidget(inputEdit);
// 	main_layout->addStretch();
// 	main_layout->addWidget(arraw);
	this->setLayout(main_layout);


}

CLeftArrawPWDEdit::~CLeftArrawPWDEdit()
{

}

void CLeftArrawPWDEdit::login()
{
	if (main_w)
	{
		main_w->clicklogin();
	}
}

void CLeftArrawPWDEdit::setMainWin(CMainWindow* main)
{
	main_w = main;
}
