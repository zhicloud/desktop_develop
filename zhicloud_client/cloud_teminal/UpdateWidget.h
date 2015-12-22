#ifndef UPDATEDIALOG_H
#define UPDATEDIALOG_H
//#include "thinloginclient.h"
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
#include <QtNetwork>
#include <QSslError>
#include <QAuthenticator>
#include <QNetworkReply>
#include <QFile>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QtGui>
#include <QMessageBox>
#include <QLabel>
#include "myclicklabel.h"
#include <QTextCodec>
#include <QStackedLayout>

const int MAX_PAGES = 256;

class CQueryVersionInfo;

class CUpdateWidget : public QWidget
{
	Q_OBJECT

public:
	CUpdateWidget(QWidget *parent = 0);
	~CUpdateWidget();
	void setDownLoadUrl(QString strurl,QString version,QString updateinfo);
	void startRequest(QUrl _url);
	QLabel			*_versionLabel;
	QString			_url;
	QSettings		*settings;
	QSettings		*m_settings;
	QString _version;
	QString _updateinfo;
signals:

public slots :
	void  closeUpdateWindowSlot();
	void  downloadWindowSlot();
	void  installWindowSlot();
	void  sliderMovedSlot1(int pos);

	void  okBtnClickSlot();
	void timerFunc();

	void httpFinished();
	void httpReadyRead();
	void updateDataReadProgress(qint64 bytesRead, qint64 totalBytes);


protected:
	
	QPoint dragPosition;

private:
	void createupdateNow();
	void createupdating();
	void createupdateAbout();
	void createcomfirmUpdate();
	void sendUrlRequst();

private:
	int				isDownloadding;
	QStackedLayout* update_stack_layout;
	QWidget			*_updateNowWidget;
	QWidget			*_comfirmUpdateWidget;
	QWidget			*_updatingWidget;
	QWidget			*_updateAboutWidget;
	QWidget			*_pagesWidget[MAX_PAGES];
	int				_maxPages;
	QPushButton     *cancelButton;
	QPushButton		*upnowButton;
	QPushButton     *comfirmOkButton;
	QPushButton		*comfirmCancelButton;
	QPushButton		*_installButton;
	QPushButton		*_bu;
	CQueryVersionInfo *_queryVersionInfo;
	int currentScrollBarPosition;
	QString			url_ip;
	QString			url_port;
	
	
	QFile			*_file;
	QNetworkAccessManager _qnam;
	bool			httpRequestAborted;
	bool			isSizeSet;
	QNetworkReply	*_reply;
	QProgressBar	*_progressBar;
	QString			_savePackagePath;
	QLabel          *_progressLabel;
	QLabel			*_progressBarLabel;
	QLabel			*reboottips;
	QLabel			*barLabel;
	QLabel			*_lab;
	QPainter		*painter;
	QLabel          *_plabel;
	QLabel          *_queryVersionlabel;
	//MyClickLabel    *_queryVersionlabel2;
	QLabel			*_versionInfo;
	QLabel			*_imformationInfo;
	QTextEdit		*_showVersionInfo;
	//MyClickLabel	*_updateDetail;
	QPushButton		*_confirm;
	
	
	QTimer* m_ptimer;
	
};

#endif // UPDATEDIALOG_H
