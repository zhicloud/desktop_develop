#include "feedbackwidget.h"

#define INIFILE "./user.ini"
FeedbackWidget::FeedbackWidget(QWidget *parent)
    : QWidget(parent)
{
        this->setFixedSize(430, 300);
		this->setAttribute(Qt::WA_TranslucentBackground, true);
		this->setWindowFlags(Qt::FramelessWindowHint);


		Feedmain = new QWidget();
		Feedmain->setAutoFillBackground(true);
		Feedmain->setFixedSize(430, 300);
		Feedmain->setObjectName("update_now");
		Feedmain->setStyleSheet("QWidget#update_now{border-image: url(:/feedback_res/fbbg);}");
		
		//upgrade btn

		upnowButton = new QPushButton();
		upnowButton->setObjectName("btn");
		upnowButton->setFixedSize(60, 24);
		upnowButton->setStyleSheet("QPushButton#btn{border-image: url(:/feedback_res/upnomal);}"
			"QPushButton#btn:hover{border-image: url(:/feedback_res/upmove);}"
			"QPushButton#btn:pressed{border-image: url(:/feedback_res/upclick);}");
		connect(upnowButton, SIGNAL(clicked()), SLOT(uploadFeedbackSlot()));

		//cancel btn
		cancelButton = new QPushButton();
		cancelButton->setObjectName("cancelbtn");
		cancelButton->setFixedSize(60, 24);
		cancelButton->setStyleSheet("QPushButton#cancelbtn{border-image: url(:/feedback_res/cancelnomal);}"
			"QPushButton#cancelbtn:hover{border-image: url(:/feedback_res/cancelmove);}"
			"QPushButton#cancelbtn:pressed{border-image: url(:/feedback_res/cancelclick);}");
		connect(cancelButton, SIGNAL(clicked()), SLOT(closeFeedbackSlot()));

		typeBox = new QComboBox();
		typeBox->setObjectName("typeBox");
		typeBox->setFixedSize(90, 20);
		typeBox->addItem("故障反馈");
		typeBox->addItem("意见建议");
		typeBox->addItem("其他");

		QHBoxLayout* box_l = new QHBoxLayout;
		box_l->setAlignment(Qt::AlignLeft);
		box_l->addStretch(1);
		box_l->addWidget(typeBox);
		box_l->addStretch(99);


		_versionInfo = new QPlainTextEdit(this);
		_versionInfo->setAttribute(Qt::WA_TranslucentBackground, true);
		_versionInfo->setStyleSheet("font-size:14px;");
		_versionInfo->setFixedSize(330, 152);
		QHBoxLayout* label_l = new QHBoxLayout;
		label_l->setAlignment(Qt::AlignLeft);
		label_l->addStretch(10);
		label_l->addWidget(_versionInfo);
		label_l->addStretch(90);
		QHBoxLayout* btn_l = new QHBoxLayout;
		btn_l->setAlignment(Qt::AlignLeft);
		btn_l->addStretch(92);
		btn_l->addWidget(upnowButton);
		btn_l->addWidget(cancelButton);
		btn_l->addStretch(8);

		QVBoxLayout* v_layout = new QVBoxLayout;
		v_layout->setContentsMargins(76, 20, 14, 21);
		v_layout->setAlignment(Qt::AlignCenter);
		v_layout->addStretch(4);
		v_layout->addLayout(box_l);
		v_layout->addStretch(2);
		v_layout->addLayout(label_l);
		v_layout->addStretch(1);
		v_layout->addLayout(btn_l);
		Feedmain->setLayout(v_layout);
		
		QHBoxLayout* main_layout = new QHBoxLayout;
		main_layout->addWidget(Feedmain);
		this->setLayout(main_layout);

		m_messageBox = new ZCMessageBox(QStringLiteral(""), this, NULL);
		
		m_manager = new QNetworkAccessManager(this);
		connect(m_manager, SIGNAL(finished(QNetworkReply*)),
				this, SLOT(replyFinished(QNetworkReply*)));
		
				
}

FeedbackWidget::~FeedbackWidget()
{

}

void FeedbackWidget::uploadFeedbackSlot()
{
	this->setCursor(Qt::BusyCursor);
	upnowButton->setEnabled(false);
	cancelButton->setEnabled(false);
	_versionInfo->setReadOnly(true);
	QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
	if (settings)
	{
		m_strUrl = settings->value("server/url").toString();
	}
	else
	{
		if (NULL != m_messageBox)
		{
			delete m_messageBox;
			m_messageBox = NULL;
		}
		m_messageBox = new ZCMessageBox(QStringLiteral("请检查user.ini!"));
		m_messageBox->setFontColor(ZCMFONTRED);
		m_messageBox->exec();
		return;
	}
    QString str_Contents;
    str_Contents = _versionInfo->toPlainText();
	int type;
	type = typeBox->currentIndex()+1;
	QString strfmt("msgfeedback?user_name=%1&password=%2&content=%3&type=%4");
	QString username = m_strUname;
	QString pwd = m_strPwd;
	QString url = strfmt.arg(username).arg(pwd).arg(str_Contents).arg(type);
	if (m_strUrl == NULL)
	{
		if (NULL != m_messageBox)
		{
			delete m_messageBox;
			m_messageBox = NULL;
		}
		m_messageBox = new ZCMessageBox(QStringLiteral("请检查user.ini中的server/url!!"));
		m_messageBox->setFontColor(ZCMFONTRED);
		m_messageBox->exec();
		return;
	}
	m_strUrl.replace("connect", url);
	qDebug() << "======feedback========" << m_strUrl << endl;
	QNetworkRequest request;
	request.setUrl(QUrl(m_strUrl));
//	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
//	_versionInfo->setPlainText(m_strUrl);
	m_reply = m_manager->get(request);

	dohttptimer.setSingleShot(true);
	connect(&dohttptimer, SIGNAL(timeout()), this, SLOT(timeabort()));
	dohttptimer.start(10000);   // 5 secs. timeout
}

void FeedbackWidget::replyFinished(QNetworkReply* reply)
{
	dohttptimer.stop();
	if (reply && reply->error() == QNetworkReply::NoError)
	{
		QByteArray data = reply->readAll();
		int len = data.size();
		//qDebug() << tr(data);
		bool ret;
		parseResult(data, ret);
		if (ret)
		{
			if (NULL != m_messageBox)
			{
				delete m_messageBox;
				m_messageBox = NULL;
			}
			m_messageBox = new ZCMessageBox(QStringLiteral("上传成功"));
			m_messageBox->setFontColor(ZCMFONTGREEN);
			m_messageBox->exec();
		} 
		else
		{
			printf("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
			if (NULL != m_messageBox)
			{
				delete m_messageBox;
				m_messageBox = NULL;
			}
			m_messageBox = new ZCMessageBox(QStringLiteral("上传失败"));
			m_messageBox->setFontColor(ZCMFONTRED);
			m_messageBox->show();
		}
	}
	else
	{
		if (NULL != m_messageBox)
		{
			delete m_messageBox;
			m_messageBox = NULL;
		}
		m_messageBox = new ZCMessageBox(QStringLiteral("网络返回错误"));
		m_messageBox->setFontColor(ZCMFONTRED);
		m_messageBox->exec();
	}
	this->setCursor(Qt::ArrowCursor);
	upnowButton->setEnabled(true);
	cancelButton->setEnabled(true);
	_versionInfo->setReadOnly(false);
}

void FeedbackWidget::parseResult(QByteArray &data, bool &ret)
{
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	if (jsonError.error == QJsonParseError::NoError)
	{
		if (jsonDoc.isObject())
		{
			QJsonObject jsonObj = jsonDoc.object();
			QStringList str = jsonObj.keys();
			QString  status = jsonObj.value("status").toString();
			if (status == "success")
			{
				ret = true;
			}
			else if (status == "fail")
			{
				ret = false;
				m_strError = jsonObj.value("message").toString();
			}
		}
	}
}

void FeedbackWidget::timeabort()
{
	m_reply->abort();
	this->setCursor(Qt::ArrowCursor);
	upnowButton->setEnabled(true);
	_versionInfo->setReadOnly(false);
}

void FeedbackWidget::closeFeedbackSlot()
{
	emit closeFeebackSignal();
}

void FeedbackWidget::setUnamePwd(QString &uname,QString &pwd)
{
	m_strUname = uname;
	m_strPwd = pwd;
}

void FeedbackWidget::grabKey()
{
	//qDebug() << "_versionInfo->grabKeyboard();" << endl;
	_versionInfo->grabKeyboard();
}
