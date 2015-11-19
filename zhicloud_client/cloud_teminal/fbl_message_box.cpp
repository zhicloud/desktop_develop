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

	QPainterPath path;
	QRectF rect = QRectF(0, 0, 188, 143);
	path.addRoundRect(rect, 3, 3);
	QPolygon polygon = path.toFillPolygon().toPolygon();//获得这个路径上的所有的点
	QRegion region(polygon);//根据这些点构造这个区域
	setMask(region);
		

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

	m_timer = new QTimer(this);
	connect(m_timer, SIGNAL(timeout()), this, SLOT(rebootSlot()));
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
	setText(QStringLiteral("正在设置,请稍候..."));
	setCursor(Qt::BusyCursor);
	printf("chmod 777 /tmp/resolution_fifo\n");
	QByteArray para = m_strCmd.toLatin1();
	//qDebug() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << m_strCmd << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>"<< endl;
	system("chmod 777 /tmp/resolution_fifo");
	int ret = system(para.data());
	printf("return value is %d\n", ret);
	m_timer->start(8000);
}

bool fbl_message_box::acceptBox()
{
	return isAccept;
}

void fbl_message_box::setFontColor(FBLMSGFONTCOLOR color)
{
	QPalette palette2;

	switch (color)
	{
		case FBLMSGFONTRED:
			palette2.setColor(QPalette::WindowText, Qt::red);
			dialogLabel->setPalette(palette2);
			break;
		case FBLMSGFONTGREEN:
			palette2.setColor(QPalette::WindowText, Qt::green);
			dialogLabel->setPalette(palette2);
			break;
		case FBLMSGFONTWHITE:
			palette2.setColor(QPalette::WindowText, Qt::white);
			dialogLabel->setPalette(palette2);
			break;
		case FBLMSGFONTGRAY:
			palette2.setColor(QPalette::WindowText, Qt::gray);
			dialogLabel->setPalette(palette2);
			break;
		case FBLMSGFONTBLACK:
			palette2.setColor(QPalette::WindowText, Qt::black);
			dialogLabel->setPalette(palette2);
			break;
		default:
			break;
	}
}

void fbl_message_box::setText(QString text)
{
	dialogLabel->setText(text);
}

void fbl_message_box::setCmd(QString str)
{
	m_strCmd = str;
}

void fbl_message_box::rebootSlot()
{
	m_timer->stop();
	setText(QStringLiteral("设置完成,即将重启!"));
	setCursor(Qt::ArrowCursor);
	system("/home/ResetIo.sh");
}




