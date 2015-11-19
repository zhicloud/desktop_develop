#include "mytextedit.h"

CMyTextEdit::CMyTextEdit(QString img, QString text, int wid, int hei,int readonly, QWidget *parent,int ispassword)
	: QWidget(parent)
{
	imgPath = img;
	width = wid;
	height = hei;
	showText = text;

	this->setFixedSize(width, height);
	this->setAutoFillBackground(true);

	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(imgPath)));
	this->setPalette(palette);
	inputEdit = new QLineEdit();
	/*inputEdit->setGeometry(5,0,width, height);*/
	inputEdit->setPlaceholderText(showText);

	if (ispassword)
	{
		inputEdit->setEchoMode(QLineEdit::Password);
	}
	
	inputEdit->setStyleSheet("background-color:transparent;color:white;font-size:15px;border:0px;");
	if (readonly)
	{
		inputEdit->setReadOnly(true);
	}
	QHBoxLayout *hLayout = new QHBoxLayout;
	hLayout->setAlignment(Qt::AlignCenter);
	hLayout->setContentsMargins(5, 0, 5, 0);
	hLayout->addWidget(inputEdit);
	this->setLayout(hLayout);
}

CMyTextEdit::~CMyTextEdit()
{

}
