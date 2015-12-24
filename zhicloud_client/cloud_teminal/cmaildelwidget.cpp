#include "cmaildelwidget.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "cmailboxwidget.h"
#include <QTimer>
CMailDelWidget::CMailDelWidget(QWidget *parent)
	: QWidget(parent)
{
	par = (CMailBoxWidget*)parent;
	this->setFixedSize(400,180);
	setAutoFillBackground(true);
	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(QString(":/mailbox/mdelbg"))));
	this->setPalette(palette);


	QLabel* info = new QLabel;
	info->setFixedSize(350,25);
	info->setText(QStringLiteral("删除后邮件将无法恢复,您确定要删除吗?"));

	QPushButton* commit = new QPushButton;
	commit->setFixedSize(60, 24);
	commit->setObjectName("commit");
	commit->setStyleSheet("QPushButton#commit{border-image: url(:/mailbox/mcommitnomal);}"
		"QPushButton#commit:hover{border-image: url(:/mailbox/mcommitmove);}"
		"QPushButton#commit:pressed{border-image: url(:/mailbox/mcommitclick);}");
	connect(commit, SIGNAL(clicked()), this, SLOT(commitfunc()));


	QPushButton* cancel = new QPushButton;
	cancel->setFixedSize(60, 24);
	cancel->setObjectName("cancel");
	cancel->setStyleSheet("QPushButton#cancel{border-image: url(:/mailbox/mcancelnomal);}"
		"QPushButton#cancel:hover{border-image: url(:/mailbox/mcancelmove);}"
		"QPushButton#cancel:pressed{border-image: url(:/mailbox/mcancelclick);}");
	connect(cancel, SIGNAL(clicked()), this, SLOT(cancelfunc()));


	QHBoxLayout* btnlay_out = new QHBoxLayout;
	btnlay_out->setSpacing(20);
	btnlay_out->addStretch(80);
	btnlay_out->addWidget(commit);
	btnlay_out->addWidget(cancel);
	btnlay_out->addStretch(20);

	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->setAlignment(Qt::AlignCenter);
	main_layout->setContentsMargins(70, 50, 0, 0);
	main_layout->setSpacing(40);
	main_layout->addWidget(info);
	
	main_layout->addLayout(btnlay_out);

	this->setLayout(main_layout);

}

CMailDelWidget::~CMailDelWidget()
{

}

void CMailDelWidget::commitfunc()
{
	close();
	par->DelMail();
	
}

void CMailDelWidget::cancelfunc()
{
	close();
}