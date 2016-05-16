#include <webview.h>

myWebView::myWebView(QWidget *parent) : QWidget(parent)
{
	exitBtn = new QPushButton(QString::fromLocal8Bit("退出"));
	mainPageBtn = new QPushButton(QString::fromLocal8Bit("主页"));
	web = new QWebView;
	web->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	web->setCursor(QCursor(Qt::PointingHandCursor));
	
	web->setUrl(QUrl("http://wsbs.sc-n-tax.gov.cn"));
	QObject::connect(web, SIGNAL(linkClicked(QUrl)), this, SLOT(openUrl(QUrl)));
	QObject::connect(exitBtn, SIGNAL(clicked()), this, SLOT(exitApp()));
	QObject::connect(mainPageBtn, SIGNAL(clicked()), this, SLOT(setMainPage()));

	QHBoxLayout *hblayout = new QHBoxLayout;
	hblayout->addStretch();
	hblayout->addWidget(mainPageBtn);
	hblayout->addStretch();
	hblayout->addWidget(exitBtn);
	hblayout->addStretch();

	QVBoxLayout *vblayout = new QVBoxLayout;
	vblayout->addWidget(web);
	vblayout->addLayout(hblayout);
	setLayout(vblayout);
}
myWebView::~myWebView()
{
	if(exitBtn != NULL){
		delete exitBtn;
		exitBtn = NULL;
	}
	if(web != NULL){
		delete web;
		web = NULL;
	}	
}
void myWebView::openUrl(const QUrl &url)
{
	web->load(url);
}
void myWebView::setMainPage()
{
	web->setUrl(QUrl("http://wsbs.sc-n-tax.gov.cn"));
	web->grabKeyboard();
}
void myWebView::exitApp()
{
	hide();
	emit exitWeb();
	//releaseKeyboard();
	//qApp->exit();
}
