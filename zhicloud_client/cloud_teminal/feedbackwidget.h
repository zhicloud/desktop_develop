#ifndef FEEDBACKWIDGET_H
#define FEEDBACKWIDGET_H

#include <QDialog>
#include <QWidget>
#include <QHBoxLayout>
#include <QIcon>
#include <QMouseEvent>
#include <QPushButton>
#include <QTime>
#include <QCoreApplication>
#include <QThread>
//#include <qt_windows.h>
#include <QProgressBar>
#include <QScrollBar>
#include <QTextEdit>
#include <QPushButton>
#include <QtNetwork/qnetworkcookie.h>
//#include <QtNetwork/QNetworkAccessManager>
//#include <QtNetwork/QNetworkReply>
//#include <QtNetwork/QNetworkRequest>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
//#include <QNetworkReply>
#include <QDesktopWidget>
#include <QFile>
//#include <QNetworkAccessManager>
#include <QtWidgets/QApplication>
#include <QUrl>
#include <QtGui>
#include <QMessageBox>
#include <QLabel>
#include <QTextCodec>
#include <QStackedLayout>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QMessageBox>
#include "zc_message_box.h"

class FeedbackWidget : public QWidget
{
    Q_OBJECT

public:
    FeedbackWidget(QWidget *parent = 0);
    ~FeedbackWidget();
	void grabKey();
	void setUnamePwd(QString &uname,QString &pwd);
public slots:
    void uploadFeedbackSlot();
	void replyFinished(QNetworkReply* reply);
	void timeabort();
	void closeFeedbackSlot();
signals:
	void closeFeebackSignal();
private:
	void parseResult(QByteArray &data,bool &ret);
private:
    int currentScreenWidth;
    int currentScreenHeight;
    QNetworkAccessManager* m_manager;
	QTimer dohttptimer;
    QNetworkReply* m_reply;
    QWidget* Feedmain;
    QPushButton* upnowButton;
    QPushButton* cancelButton;
    QPlainTextEdit* _versionInfo;
    QComboBox* typeBox;
    QStackedLayout* update_stack_layout;
	ZCMessageBox *m_messageBox;
	QString m_strError;
	QString m_strUrl;
	QString m_strUname;
	QString m_strPwd;
};

#endif // FEEDBACKWIDGET_H
