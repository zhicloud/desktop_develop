#ifndef __MYWEBVIEW__
#define __MYWEBVIEW__

#include <QWebFrame>
#include <QWebView>
#include <QUrl>
//#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>

class myWebView : public QWidget
{
	Q_OBJECT
public:
	myWebView(QWidget *parent = 0);
	~myWebView();
public slots:
	void exitApp();
	void setMainPage();
	void openUrl(const QUrl &);
signals:
	void exitWeb();
private:
	QPushButton *exitBtn;
	QPushButton *mainPageBtn;
	QWebView *web;
};
#endif
