#include "fbl_message_box.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QGridLayout>

fbl_message_box::fbl_message_box(QString message, QWidget *parent)
: QDialog(parent)
{
	isAccept = false;
	this->setFixedSize(188, 144);
	setAutoFillBackground(true);

	QPalette palette;
	palette.setBrush(QPalette::Background, QBrush(QPixmap(":/dialog_res/dialogBackground")));
	this->setPalette(palette);


	QGridLayout* main_layout = new QGridLayout;
	main_layout->setContentsMargins(46, 46, 46, 36);

	this->setLayout(main_layout);


	this->setWindowFlags(Qt::FramelessWindowHint);

	QDesktopWidget* desktop = QApplication::desktop();
	QRect deskrect = desktop->availableGeometry();
	//printfQStringLiteral("--------------------msgbox: %d\n", deskrect.width());
	//居中显示
	this->move((deskrect.width() - this->width()) / 2, (deskrect.height() - this->height()) / 2);

	dialogLabel = new QLabel(this);
	dialogLabel->setGeometry(0, 70, 188, 20);
	dialogLabel->setStyleSheet("font-size:12.5px");
	dialogLabel->setText(message);
	dialogLabel->setAlignment(Qt::AlignHCenter);
	QPalette palette2;
	palette2.setColor(QPalette::WindowText, QColor(232, 86, 86));
	dialogLabel->setPalette(palette2);

	//È·¶¨°´Å¥
	cancelButton = new QPushButton(this);
	cancelButton->setGeometry(12, 107, 70, 30);
	cancelButton->setObjectName("cancelbtn");
	cancelButton->setStyleSheet("QPushButton#cancelbtn{border-image: url(:/net_about/retnomal);}"
		"QPushButton#cancelbtn:hover{border-image: url(:/net_about/rethover);}"
		"QPushButton#cancelbtn:pressed{border-image: url(:/net_about/retclick);}");
	connect(cancelButton, SIGNAL(clicked()), SLOT(closeDialogSlot()));

	okButton = new QPushButton(this);
	okButton->setGeometry(104, 107, 70, 30);
	okButton->setObjectName("okbtn");
	okButton->setStyleSheet("QPushButton#okbtn{border-image: url(:/setsvrurl/nomal);}"
		"QPushButton#okbtn:hover{border-image: url(:/setsvrurl/hover);}"
		"QPushButton#okbtn:pressed{border-image: url(:/net_about/sureclick);}");
	connect(okButton, SIGNAL(clicked()), SLOT(okBtnClickSlot()));
}

fbl_message_box::~fbl_message_box()
{

}

void fbl_message_box::closeDialogSlot()
{
	isAccept = false;
	close();
}

void fbl_message_box::okBtnClickSlot()
{
	isAccept = true;
	close();
}

bool fbl_message_box::acceptBox()
{
	return isAccept;
}

