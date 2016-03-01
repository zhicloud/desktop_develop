#include "UpdateWidget.h"
#include "log.h"

//#include <unistd.h>

#define INIFILE "./user.ini"
using namespace std;
CUpdateWidget::CUpdateWidget(QWidget *parent)
: QWidget(parent)
{
	
	createupdateNow();
	createcomfirmUpdate();
	createupdating();
	
	update_stack_layout = new QStackedLayout();
	update_stack_layout->setContentsMargins(0, 0, 0, 0);
	update_stack_layout->addWidget(_updateNowWidget);
	update_stack_layout->addWidget(_updatingWidget);
	update_stack_layout->addWidget(_comfirmUpdateWidget);
	update_stack_layout->setCurrentWidget(_updateNowWidget);

	this->setLayout(update_stack_layout);
	this->setFixedSize(315, 257);
	this->setWindowFlags(Qt::FramelessWindowHint);
	isDownloadding = 0;

	m_ptimer = new QTimer(this);

	//新建定时器
	m_settings = new QSettings(INIFILE, QSettings::IniFormat);
	connect(m_ptimer, SIGNAL(timeout()), this, SLOT(timerFunc()));
	
}

CUpdateWidget::~CUpdateWidget()
{

}

void CUpdateWidget::createupdateNow()
{
	_updateNowWidget = new QWidget();
	_updateNowWidget->setAutoFillBackground(true);
	_updateNowWidget->setFixedSize(315, 257);
	_updateNowWidget->setObjectName("update_now");
	_updateNowWidget->setStyleSheet("QWidget#update_now{border-image: url(:/upgrade/upbg);}");
	
	
	//upgrade btn
	upnowButton = new QPushButton();
	upnowButton->setObjectName("btn");
	upnowButton->setFixedSize(70, 26);
	upnowButton->setStyleSheet("QPushButton#btn{border-image: url(:/upgrade/upnomal);}"
		"QPushButton#btn:hover{border-image: url(:/upgrade/upmove);}"
		"QPushButton#btn:pressed{border-image: url(:/upgrade/upclick);}");
	connect(upnowButton, SIGNAL(clicked()), SLOT(okBtnClickSlot()));

	//cancel btn
	cancelButton = new QPushButton();
	cancelButton->setObjectName("cancelbtn");
	cancelButton->setFixedSize(70, 26);
	cancelButton->setStyleSheet("QPushButton#cancelbtn{border-image: url(:/upgrade/cancelnomal);}"
		"QPushButton#cancelbtn:hover{border-image: url(:/upgrade/cancelmove);}"
		"QPushButton#cancelbtn:pressed{border-image: url(:/upgrade/cancelclick);}");
	connect(cancelButton, SIGNAL(clicked()), SLOT(closeUpdateWindowSlot()));

	//显示文本框
	_versionInfo = new QLabel();
	_versionInfo->setStyleSheet("font-size:12px;color:rgb(228,228,228);");
	_versionInfo->setFixedSize(250, 200);
	_versionInfo->setWordWrap(true);
	QHBoxLayout* label_l = new QHBoxLayout;
	label_l->setAlignment(Qt::AlignCenter);
	label_l->addWidget(_versionInfo);
	QHBoxLayout* btn_l = new QHBoxLayout;
	btn_l->setAlignment(Qt::AlignCenter);
	btn_l->addStretch(91);
	btn_l->addWidget(cancelButton);
	btn_l->addWidget(upnowButton);
	btn_l->addStretch(9);
	QVBoxLayout* v_layout = new QVBoxLayout;
	v_layout->setContentsMargins(20, 0,0 , 20);
	v_layout->setAlignment(Qt::AlignCenter);
	v_layout->addLayout(label_l);
	v_layout->addStretch();
	v_layout->addLayout(btn_l);
	_updateNowWidget->setLayout(v_layout);
}

void CUpdateWidget::createcomfirmUpdate()
{
	_comfirmUpdateWidget = new QWidget();
	_comfirmUpdateWidget->setAutoFillBackground(true);
	_comfirmUpdateWidget->setFixedSize(315, 257);
	_comfirmUpdateWidget->setObjectName("_comfirmUpdateWidget");
	_comfirmUpdateWidget->setStyleSheet("QWidget#_comfirmUpdateWidget{border-image: url(:/upgrade/upbg);}");
	
	
	//upgrade btn
	comfirmOkButton = new QPushButton();
	comfirmOkButton->setObjectName("comfirmOkButton");
	comfirmOkButton->setFixedSize(70, 26);
	comfirmOkButton->setStyleSheet("QPushButton#comfirmOkButton{border-image: url(:/upgrade/upnomal);}"
		"QPushButton#comfirmOkButton:hover{border-image: url(:/upgrade/upmove);}"
		"QPushButton#comfirmOkButton:pressed{border-image: url(:/upgrade/upclick);}");
	connect(comfirmOkButton, SIGNAL(clicked()), SLOT(downloadWindowSlot()));

	//cancel btn
	comfirmCancelButton = new QPushButton();
	comfirmCancelButton->setObjectName("comfirmCancelButton");
	comfirmCancelButton->setFixedSize(70, 26);
	comfirmCancelButton->setStyleSheet("QPushButton#comfirmCancelButton{border-image: url(:/upgrade/cancelnomal);}"
		"QPushButton#comfirmCancelButton:hover{border-image: url(:/upgrade/cancelmove);}"
		"QPushButton#comfirmCancelButton:pressed{border-image: url(:/upgrade/cancelclick);}");
	connect(comfirmCancelButton, SIGNAL(clicked()), SLOT(closeUpdateWindowSlot()));

	//显示文本框
	_imformationInfo = new QLabel();
	_imformationInfo->setStyleSheet("font-size:20px;color:rgb(232,86,86);");
	_imformationInfo->setFixedSize(250, 200);
	_imformationInfo->setWordWrap(true);
	QPalette palette2;
    	palette2.setColor(QPalette::WindowText, QColor(255, 0, 0));
    	_imformationInfo->setPalette(palette2);
	_imformationInfo->setText(QStringLiteral("升级过程可能会出现长时间黑屏或多次重启，请不要做任何操作!!!!!!!!!!"));
	QHBoxLayout* label_l = new QHBoxLayout;
	label_l->setAlignment(Qt::AlignCenter);
	label_l->addWidget(_imformationInfo);
	QHBoxLayout* btn_l = new QHBoxLayout;
	btn_l->setAlignment(Qt::AlignCenter);
	btn_l->addStretch(91);
	btn_l->addWidget(comfirmCancelButton);
	btn_l->addWidget(comfirmOkButton);
	btn_l->addStretch(9);
	QVBoxLayout* v_layout = new QVBoxLayout;
	v_layout->setContentsMargins(20, 0,0 , 20);
	v_layout->setAlignment(Qt::AlignCenter);
	v_layout->addLayout(label_l);
	v_layout->addStretch();
	v_layout->addLayout(btn_l);
	_comfirmUpdateWidget->setLayout(v_layout);
}


void CUpdateWidget::createupdating()
{
	//下载页面
	_updatingWidget = new QWidget(this);
	_updatingWidget->setObjectName("updating");
	_updatingWidget->setStyleSheet("QWidget#updating{border-image: url(:/upgrade/upbg);}");


	_progressBarLabel = new QLabel();
	_progressBarLabel->setFixedSize(60, 60);
	_progressBarLabel->setText(QStringLiteral("正在下载:"));
	_progressBarLabel->setStyleSheet("font-size:12px;color:rgb(228,228,228);");
	
	//下载进度条
	_progressBar = new QProgressBar();
	_progressBar->setFixedSize(270, 3);
	_progressBar->setStyleSheet("QProgressBar::chunk {\
								background-color: #04C2B8;\
								position:absolute;}"\
								"QProgressBar{border: 0px solid grey;\
								border-radius: 1px;\
	 				    		}");

	_lab = new QLabel(_updatingWidget);//滑块
	_lab->setFixedSize(48, 32);

	_plabel = new QLabel();//百分比
	_plabel->setFixedSize(50, 20);

	_progressBar->setValue(0);
	_progressBar->setTextVisible(0);

	reboottips = new QLabel();//reboot tips
	reboottips->setFixedSize(120, 60);
	reboottips->setStyleSheet("font-size:12px;color:rgb(228,228,228);");
	reboottips->setText("");

	QHBoxLayout* l_yout = new QHBoxLayout;
	l_yout->addWidget(_progressBarLabel);
	l_yout->addWidget(_plabel);
	l_yout->addStretch();

	QVBoxLayout* main_layout = new QVBoxLayout;
	main_layout->addLayout(l_yout);
	main_layout->setContentsMargins(20, 50, 20, 0);
	main_layout->addWidget(_progressBar);
	main_layout->addWidget(reboottips);
	main_layout->addStretch();
	_updatingWidget->setLayout(main_layout);


}

void CUpdateWidget::sliderMovedSlot1(int pos)
{
	currentScrollBarPosition = pos;
}
//关闭更新窗口槽函数
void CUpdateWidget::closeUpdateWindowSlot()
{
	close();
}

//正在下载界面槽函数
void CUpdateWidget::downloadWindowSlot()
{
	
	system("cd /home;mkdir download_ext;chmod 777 download_ext");
 	_savePackagePath = "/home/download_ext/";
 	_savePackagePath += QString("upgrade.tgz");

//	_savePackagePath = "./cloud_teminal";
	if (QFile::exists(_savePackagePath)) 
	{
		QFile::remove(_savePackagePath);
	}

	_file = new QFile(_savePackagePath);
	if (!_file->open(QIODevice::WriteOnly))
	{
		delete _file;
		_file = 0;
		return;
	}
	startRequest(_url);

}

void CUpdateWidget::okBtnClickSlot()
{
	update_stack_layout->setCurrentWidget(_comfirmUpdateWidget);
}

void CUpdateWidget::startRequest(QUrl url)
{
	isSizeSet = false;
	QNetworkRequest networkRequest;
	networkRequest.setUrl(url);
	_reply = _qnam.get(networkRequest);

	connect(_reply, SIGNAL(readyRead()),
		this, SLOT(httpReadyRead()));
	connect(_reply, SIGNAL(downloadProgress(qint64, qint64)),
		this, SLOT(updateDataReadProgress(qint64, qint64)));
	connect(_reply, SIGNAL(finished()),
		this, SLOT(httpFinished()));
}
void CUpdateWidget::httpFinished()
{
	_file->flush();
	_file->close();
	isDownloadding = 0;
	reboottips->setText(QStringLiteral("下载成功,重启系统."));


	system("cp /home/download_ext/upgrade.tgz /home/download");

	
	/*if (!_version.isEmpty())
	{
		QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
		if (settings)
		{
			settings->setValue("version/ver", _version);
		}
	}*/
	m_ptimer->start(4000);
}

void CUpdateWidget::httpReadyRead()
{
	/*if (!isSizeSet)
	{
		QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
		int file_Size = _reply->header(QNetworkRequest::ContentLengthHeader).toInt();
		settings->setValue("version/size", file_Size);
		isSizeSet = true;
	}*/
	if (!isDownloadding)
	{
		isDownloadding = 1;
		update_stack_layout->setCurrentWidget(_updatingWidget);
	}

	if (_file)
		_file->write(_reply->readAll());
}

void CUpdateWidget::updateDataReadProgress(qint64 bytesRead, qint64 totalBytes)
{

	_progressBar->setMaximum(totalBytes);
	_progressBar->setValue(bytesRead);
	int k = 0;
	double p = ((double)bytesRead / (double)totalBytes);
	k = p * 445;
	_lab->setGeometry(51 + k, 281, 48, 32);
	_lab->show();
	_plabel->setStyleSheet("font-size:12px;color:rgb(228,228,228);");
	_plabel->setText(QString("%1%").arg((int)(p * 100)));
	


}

//立即安装界面槽函数
void  CUpdateWidget::installWindowSlot()
{	
	QString updateProgressPath = "install.exe "+_savePackagePath;
	char str[255];
	//strcpy_s(str, sizeof(str), updateProgressPath.toLocal8Bit().data());
	//WinExec(str, SW_HIDE);
}
void CUpdateWidget::setDownLoadUrl(QString strurl, QString version, QString updateinfo)
{
	_url = strurl;
	_version = version;
	_updateinfo = updateinfo;
	_versionInfo->setText(QStringLiteral("有新版本，可以开始升级") + _version + "\n" + _updateinfo);
}

void CUpdateWidget::timerFunc()
{
	system("reboot");
}
