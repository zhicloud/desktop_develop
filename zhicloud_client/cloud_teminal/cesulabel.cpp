#include "cesulabel.h"
#include <QHBoxLayout>
cesulabel::cesulabel(QWidget *parent)
	: QWidget(parent)
{
	this->setFixedSize(220, 20);
	pjxz = new QLabel;
	pjxz->setFixedSize(50, 20);
	pjxz->setText(QStringLiteral("平均下载"));
	sudu = new QLabel;
	sudu->setFixedSize(70, 20);
	sudu->setText(QString(" 0.0KB/s"));
	sudu->setStyleSheet("background-color:transparent;color:rgb(39,149,234);font-size:11px;border:0px;");
	xdy = new QLabel;
	xdy->setFixedSize(35, 20);
	xdy->setText(QStringLiteral("相当于"));
	xm = new QLabel;
	xm->setFixedSize(30, 20);
	xm->setText(QString(" 0M"));
	xm->setStyleSheet("background-color:transparent;color:rgb(39,149,234);font-size:11px;border:0px;");
	dk = new QLabel;
	dk->setFixedSize(30, 20);
	dk->setText(QStringLiteral("带宽"));

	QHBoxLayout* main_layout = new QHBoxLayout;
	main_layout->setContentsMargins(0, 0, 0, 0);
	main_layout->addWidget(pjxz);
	main_layout->addWidget(sudu);
	main_layout->addWidget(xdy);
	main_layout->addWidget(xm);
	main_layout->addWidget(dk);
	
	this->setLayout(main_layout);
}

cesulabel::~cesulabel()
{

}

void cesulabel::setKBM(QString kb, QString m)
{
	sudu->setText(kb);
	xm->setText(m);
}
