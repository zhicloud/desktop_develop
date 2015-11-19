#include "cloading.h"

CLoading::CLoading(QWidget *parent)
	: QWidget(parent)
{

	this->setFixedSize(150, 25);
	txtshow = QStringLiteral("正在登陆,请稍等.");

	loadingbtn = new QPushButton;
	loadingbtn->setFixedSize(14, 14);
	loadingbtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
	loadingbtn->setIcon(QIcon(QString(":/loading/0")));
	loadingbtn->setIconSize(QSize(14, 14));

	label = new QLabel;
	label->setFixedSize(130, 14);
	label->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
	label->setText(txtshow);

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(timefunc()));
	count = 1;

	QHBoxLayout* main_l = new QHBoxLayout;
	main_l->setAlignment(Qt::AlignCenter);
	main_l->addWidget(loadingbtn);
	main_l->addWidget(label);

	this->setLayout(main_l);


	label->hide();
	loadingbtn->hide();

}

CLoading::~CLoading()
{

}

void CLoading::timefunc()
{
	loadingbtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
	QString fmt = ":/loading/%1";
	QString path = fmt.arg(count);
	loadingbtn->setIcon(QIcon(path));
	loadingbtn->setIconSize(QSize(14, 14));
	count++;
	if (25 == count)
	{
		count = 1;
	}

	if (count == 5)
	{
		label->setText(txtshow + QStringLiteral(".."));
	}
	if (count == 10)
	{
		label->setText(txtshow + QStringLiteral("..."));
	}
	if (count == 15)
	{
		label->setText(txtshow + QStringLiteral("...."));
	}
	if (count == 20)
	{
		label->setText(txtshow + QStringLiteral("....."));
	}
}

void CLoading::loadingshow()
{
	count = 1;
	label->show();
	loadingbtn->show();
	timer->start(40);
}

void CLoading::loadinghide()
{
	label->hide();
	loadingbtn->hide();
	timer->stop();
}

void CLoading::setText(QString strtxtshow)
{
	txtshow = strtxtshow;
	label->setText(txtshow + ".");
}