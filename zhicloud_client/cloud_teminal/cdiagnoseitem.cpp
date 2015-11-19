#include "cdiagnoseitem.h"

CDiagnoseItem::CDiagnoseItem(QWidget *parent)
	: QWidget(parent)
{
	this->setFixedSize(200,15);
	text = new QLabel;
	icon = new QLabel;
	status = new QLabel;

	text->setFixedSize(115,13);
	text->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
	icon->setFixedSize(13, 13);
	status->setFixedSize(30,13);
	status->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");

	icon->setPixmap(QPixmap(QString(":/netdiagnose/iconsright")));
	status->setText(QStringLiteral("正常"));

	QHBoxLayout* mainw_layout = new QHBoxLayout;
	mainw_layout->setContentsMargins(0, 0, 0, 0);
	mainw_layout->addWidget(text);
	mainw_layout->addWidget(icon);
	mainw_layout->addWidget(status);
	this->setLayout(mainw_layout);

}

CDiagnoseItem::~CDiagnoseItem()
{

}

void CDiagnoseItem::setText(QString txt)
{
	text->setText(txt);
}

void CDiagnoseItem::setStatus(bool torf)
{
	if (torf == true)
	{
		icon->setPixmap(QPixmap(QString(":/netdiagnose/iconsright")));
		status->setText(QStringLiteral("正常"));
	}
	else
	{
		icon->setPixmap(QPixmap(QString(":/netdiagnose/iconswrong")));
		status->setText(QStringLiteral("异常"));
	}
}