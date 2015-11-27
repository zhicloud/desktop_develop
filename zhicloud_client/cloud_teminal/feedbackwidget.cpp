#include "feedbackwidget.h"

#define INIFILE "./user.ini"
FeedbackWidget::FeedbackWidget(QWidget *parent)
    : QWidget(parent)
{
        currentScreenWidth = QApplication::desktop()->width();
        currentScreenHeight = QApplication::desktop()->height();
        this->setGeometry((currentScreenWidth - 480) / 2, (currentScreenHeight - 311) / 2 - 39, 480, 311);
 //     m_manager = new QNetworkAccessManager(this);
        createView();

        update_stack_layout = new QStackedLayout();
        update_stack_layout->setContentsMargins(0, 0, 0, 0);
        update_stack_layout->addWidget(Feedmain);
        update_stack_layout->setCurrentWidget(Feedmain);

        this->setLayout(update_stack_layout);
        this->setFixedSize(480, 311);
        this->setAttribute(Qt::WA_TranslucentBackground, true);
        this->setWindowFlags(Qt::FramelessWindowHint);

		m_manager = new QNetworkAccessManager(this);
		connect(m_manager, SIGNAL(finished(QNetworkReply*)),
				this, SLOT(replyFinished(QNetworkReply*)));
}

FeedbackWidget::~FeedbackWidget()
{

}

void FeedbackWidget::createView()
{
        Feedmain = new QWidget();
       // Feedmain->setAutoFillBackground(true);
        Feedmain->setFixedSize(480, 311);
        Feedmain->setObjectName("update_now");
        Feedmain->setStyleSheet("QWidget#update_now{border-image: url(:/feedback_res/fbbg);}");

        //upgrade btn
        closeButton = new QPushButton();
        closeButton->setObjectName("btn");
        closeButton->setFixedSize(18, 18);
        closeButton->setStyleSheet("QPushButton#btn{border-image: url(:/feedback_res/closenomal);}"
            "QPushButton#btn:hover{border-image: url(:/feedback_res/closemove);}"
            "QPushButton#btn:pressed{border-image: url(:/feedback_res/closeclick);}");
        connect(closeButton, SIGNAL(clicked()), SLOT(closeFeedbackSlot()));

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
        typeBox->setFixedSize(90,20);
        typeBox->addItem("故障反馈");
        typeBox->addItem("意见建议");
		typeBox->addItem("其他");

        QHBoxLayout* box_l = new QHBoxLayout;
        box_l->setAlignment(Qt::AlignLeft);
        box_l->addStretch(1);
        box_l->addWidget(typeBox);
        box_l->addStretch(99);

        QHBoxLayout* close_l = new QHBoxLayout;
        close_l->setAlignment(Qt::AlignLeft);
        close_l->addStretch(99);
        close_l->addWidget(closeButton);
        close_l->addStretch(1);

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
            v_layout->setContentsMargins(102, 4, 14, 21);
            v_layout->setAlignment(Qt::AlignCenter);
            v_layout->addLayout(close_l);
            v_layout->addStretch(4);
            v_layout->addLayout(box_l);
            v_layout->addStretch(1);
            v_layout->addLayout(label_l);
            v_layout->addStretch(1);
            v_layout->addLayout(btn_l);
            Feedmain->setLayout(v_layout);
            Feedmain->show();
}

void FeedbackWidget::uploadFeedbackSlot()
{
	this->setCursor(Qt::WaitCursor);
	upnowButton->setEnabled(false);
	_versionInfo->setReadOnly(true);
	QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
	if (settings)
	{
		m_strUrl = settings->value("server/url").toString();
	}
	else
	{
		m_messageBox.setText("请检查user.ini!\n上传失败!");
		m_messageBox.exec();
		return;
	}
    QString str_Contents;
    str_Contents = _versionInfo->toPlainText();
	int type;
	type = typeBox->currentIndex()+1;
	QString strfmt("msgfeedback?user_name=%1&password=%2&content=%3&type=%4");
	QString username = "hahahahaha";
	QString pwd = "2acc04bab53c014253a8684dbc9071fe";
	QString url = strfmt.arg(username).arg(pwd).arg(str_Contents).arg(type);
	if (m_strUrl == NULL)
	{
		m_messageBox.setText("请检查user.ini中的server/url!\n上传失败!");
		m_messageBox.exec();
		return;
	}
	m_strUrl.replace("connect", url);
	QNetworkRequest request;
	request.setUrl(QUrl(m_strUrl));
//	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
//	_versionInfo->setPlainText(m_strUrl);
	m_reply = m_manager->get(request);

	dohttptimer.setSingleShot(true);
	connect(&dohttptimer, SIGNAL(timeout()), this, SLOT(timeabort()));
	dohttptimer.start(10000);   // 5 secs. timeout
}

void FeedbackWidget::closeFeedbackSlot()
{
    close();
}

void FeedbackWidget::replyFinished(QNetworkReply* reply)
{
	dohttptimer.stop();
	if (reply && reply->error() == QNetworkReply::NoError)
	{
		QByteArray data = reply->readAll();
		int len = data.size();
		qDebug() << tr(data);
		bool ret;
		parseResult(data, ret);
		if (ret)
		{
			m_messageBox.setText("上传成功");
			m_messageBox.exec();
		} 
		else
		{
			QString str_Tmp = "上传失败\n";
			str_Tmp += m_strError;
			m_messageBox.setText(str_Tmp);
			m_messageBox.exec();
		}
	}
	else
	{
		m_messageBox.setText("网络返回错误!");
		m_messageBox.exec();
	}
	this->setCursor(Qt::ArrowCursor);
	upnowButton->setEnabled(true);
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
