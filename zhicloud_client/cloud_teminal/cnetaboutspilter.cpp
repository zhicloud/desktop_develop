#include "cnetaboutspilter.h"
#include <QHBoxLayout>
CNetAboutSpilter::CNetAboutSpilter(QWidget *parent)
	: QWidget(parent)
{
	this->setFixedSize(2, 280);
	setAutoFillBackground(true);
	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(QString(":/net_about/spilter"))));
	this->setPalette(palette);


	slider = new QLabel(this);
	slider->setFixedSize(2, 15);
	slider->setPixmap(QPixmap(":/net_about/cursor"));
}

CNetAboutSpilter::~CNetAboutSpilter()
{
	
}

void CNetAboutSpilter::moveSlider(int index)
{

	slider->setGeometry(0, index*34, 2, 19);
}