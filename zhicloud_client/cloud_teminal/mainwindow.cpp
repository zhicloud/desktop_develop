#include "mainwindow.h"
#include "mylog.h"
#include "clogthred.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QtWidgets/QApplication>
#include <QTextCodec>
#include <QDesktopWidget>
#include "clabel.h"
#include "util.h"
#include "cusercard.h"
#include "cleftarrawpwdedit.h"
#include <QCheckBox>
#include "cmenuwidget.h"
#include "chostlist.h"
#include "ringlist.h"
#include "common.h"
#include "UpdateWidget.h"
#include "cmailboxwidget.h"
#include <QFile>
//#include <QTableView>//by xzg
#include <QHeaderView>// by xzg

#define CENTER_WIDGET_W 800
#define CENTER_WIDGET_H 600
#define ORI_DIS_EVENT QEvent::User+102
#define SPICE_CHANNEL_ERROR_LINK QEvent::User+103
#define SPICE_CHANNEL_CLOSED QEvent::User+104
#define SPICE_CHANNEL_ERROR_AUTH QEvent::User+105
#define SPICE_RECONNECT QEvent::User+109
#define ERROR_IO_SHOW_WIDGET QEvent::User+120



#ifndef OS_X86  
const QString cpu_architecture = "ARM";
#ifndef ZS
const QString main_version = "1.1.2";
#else
const QString main_version = "1.1.6";
#endif
#else
const QString cpu_architecture = "X86";
#ifndef XH 
const QString main_version = "1.1.1";
#else
const QString main_version = "1.1.1";
#endif
#endif

#ifndef XH 
const QString hardware_company = "SD";
#else
const QString hardware_company = "XH";
#endif

#ifndef OS_X86 
const QString os_version = "Ubuntu Linaro 13.09";
#else
const QString os_version = "Ubuntu 4.8.2-19";
#endif

#ifndef ZS
const QString custom_type = "TC";
#else
const QString custom_type = "ZS";

#endif

   CMainWindow::CMainWindow(QWidget *parent)
: QMainWindow(parent)
{
   bool ret = SpiceMulViewer::Spice_Init();//by xzg
   isKillNetMgr = 0;
   reconnecting = 0;
   adduserretwid = NULL;
   changesetretbtn = NULL;
   isadduser = 0;
   ishasmail = 0;
   islogining = 0;
   isviewshow = 0;
   isdhcpclick = 0;
   isstarthost = false;
   hasonceclick = false;
   ismacright = false;
   isadduserretshow = false;

   m_okBtn = NULL;
   hloading = NULL;

   isdhcpclicktimer = new QTimer(this);
   connect(isdhcpclicktimer, SIGNAL(timeout()), this, SLOT(isdhcpclicktimerout()));
   menu = NULL;
   viewer = NULL;
   prewdiget = NULL;
   adduserretbtn = NULL;
   cs_btn = NULL;
   changsetBtn = NULL;
   userchangeBtn = NULL;
   currentScreenWidth = QApplication::desktop()->width();
   currentScreenHeight = QApplication::desktop()->height();
   widthRatio = currentScreenWidth / 1920.0;
   heightRatio = currentScreenHeight / 1080.0;
   this->setFixedSize(currentScreenWidth, currentScreenHeight);
   userlist = NULL;


   main_widget = new QWidget();
   //viewer = new SpiceMulViewer(main_widget, currentScreenWidth, currentScreenHeight);//by xzg
   //viewer->hide();//by xzg
   //viewer->OpenSpice(NULL, NULL);//by xzg
   setCentralWidget(main_widget);

   center_widget = new QWidget(main_widget);
   center_widget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);

   mainLayout = new QStackedLayout();
   mainLayout->setAlignment(Qt::AlignCenter);

   m_usbConfig = new usbonfig;//by xzg

   createsetsvrurlWidget();
   createFirstLoginWidget();
   createSecondLoginWidget();
   createChangePassWidget();
   createMoreThan3Widget();
   create3UserWdiget();
   create2UserWidget();
   create1UserWidget();
   create0userWidget();
   createAbout_NetSettingWidget();
#ifdef ZS
   createLoginWidget();// by xzg
   /************by xzg**************/
   webView = new myWebView(main_widget);
   connect(webView, SIGNAL(exitWeb()), this, SLOT(setViewerGrabKeyboard()));
   webView->setFixedSize(currentScreenWidth, currentScreenHeight);
   webView->hide();
   /************by xzg**************/
#endif


   mainLayout->addWidget(setsvrurlWidget);//0
   mainLayout->addWidget(firstLoginWidget);
   mainLayout->addWidget(secondLoginWidget);
   //mainLayout->addWidget(morethan3Widget);
   mainLayout->addWidget(changePswdWidget);
   //mainLayout->addWidget(changeuserwidget);
   mainLayout->addWidget(about_netsetwidget);

   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   if (settings)
   {
      QString url = settings->value("server/url").toString();
      QString luser = settings->value("lastsucuser/path").toString();
      QString picpath = "users/" + luser;
      QString nickname = "usersname/" + luser;
      uname = settings->value("lastsucuser/path").toString();
      QString pic = settings->value(picpath).toString();
      nickname = settings->value(nickname).toString();
      settings->beginReadArray("users");
      QStringList keys = settings->allKeys();
      int size = keys.size();
      if (url.isEmpty())
      {
         mainLayout->setCurrentWidget(setsvrurlWidget);
      }
      else if (size > 0)
      {
         if (pic.isEmpty())
         {
            qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
            pic = QString(":/userpic/p%1").arg((qrand() % 11) + 1);
         }
         secondwuserpic->setPixmap(pic);
         secondwuserpic->setText(nickname);

         someItemOnWidget(secondLoginWidget);
         mainLayout->setCurrentWidget(secondLoginWidget);
      }
      else
      {
         someItemOnWidget(firstLoginWidget);
         mainLayout->setCurrentWidget(firstLoginWidget);
      }
   }
   mainLayout->setAlignment(Qt::AlignCenter);
   this->setStyleSheet("QMainWindow{border-image: url(:/first_login_res/background);}");
   center_widget->setLayout(mainLayout);



   neterror = new QPushButton(this);
   neterror->setFixedSize(126, 14);
   neterror->setObjectName("neterrorBtn");
   neterror->setStyleSheet("QPushButton#neterrorBtn{border-image: url(:/first_login_res/neterror);}"
         "QPushButton#neterrorBtn:hover{border-image: url(:/first_login_res/neterror);}"
         "QPushButton#neterrorBtn:pressed{border-image: url(:/first_login_res/neterror);}");
   neterror->setGeometry((currentScreenWidth - 126) / 2,currentScreenHeight - 108 -64, 126, 14);
   neterror->hide();

   netdicwidget = new cnetdiswidget(this);
   netdicwidget->setMainw(this);
   netdicwidget->setFixedSize(420, 60);
   netdicwidget->setGeometry((currentScreenWidth - 420) / 2, (currentScreenHeight - 60) / 2, 420, 60);
   netdicwidget->hide();


#ifdef ZS
   /*QWidget **/logo_widget = new QWidget(this);//by xzg
   logo_widget->setFixedSize(340, 70);


   QLabel *zylogoLabel = new QLabel;
   zylogoLabel->setFixedSize(172, 58);
   QPixmap *zylogoPixmap = new QPixmap(":/first_login_res/zylogo");
   zylogoLabel->setPixmap(*zylogoPixmap);

   QLabel *zslogoLabel = new QLabel;
   zslogoLabel->setFixedSize(108, 58);
   QPixmap *zslogoPixmap = new QPixmap(":/first_login_res/zslogo");
   zslogoLabel->setPixmap(*zslogoPixmap);

   logo_widget->setGeometry((currentScreenWidth - 340) / 2, currentScreenHeight - 108, 340, 70);

   QHBoxLayout* logolayout = new QHBoxLayout;
   logolayout->addStretch(1);
   logolayout->addWidget(zslogoLabel);
   logolayout->addStretch(10);
   logolayout->addWidget(zylogoLabel);
   logolayout->addStretch(1);
   logo_widget->setLayout(logolayout);

   //    QHBoxLayout* logwd_layout = new QHBoxLayout;
   //    logwd_layout->setAlignment(Qt::AlignCenter);
   //    logwd_layout->addWidget(logo_widget);

#else
   QLabel *logoLabel = new QLabel;
   logoLabel->setFixedSize(185, 64);
   QPixmap *logoPixmap = new QPixmap(":/first_login_res/zylogo");
   logoLabel->setPixmap(*logoPixmap);
   logoLabel->setGeometry(currentScreenWidth/2-185/2, currentScreenHeight - 108, 48, 44);
   QHBoxLayout* logolayout = new QHBoxLayout;
   logolayout->setAlignment(Qt::AlignCenter);
   logolayout->addWidget(logoLabel);
#endif



   QVBoxLayout* h_layout = new QVBoxLayout;
   h_layout->setAlignment(Qt::AlignCenter);
   h_layout->addStretch();
   h_layout->addStretch();
   h_layout->addStretch();
   h_layout->addWidget(center_widget);
   h_layout->addStretch();
   h_layout->addStretch();
#ifdef ZS
   h_layout->addWidget(logo_widget,0,Qt::AlignHCenter);
#else
   h_layout->addLayout(logolayout);
#endif
   //h_layout->addStretch();
   main_widget->setLayout(h_layout);

   this->setWindowTitle(QStringLiteral("致云科技桌面云终端"));
   this->setWindowFlags(Qt::FramelessWindowHint);

   installEventFilter(this);
   setMouseTracking(true);


   m_manager = new QNetworkAccessManager(this);

   connect(m_manager, SIGNAL(finished(QNetworkReply*)),
         this, SLOT(replyFinished(QNetworkReply*)));

   okButtonIsRet = true;
   msgbox = NULL;

   //modify by lcx
   //CheckNeedUpgrade();
   isUpgradeCheckd = false;

   netchecktimer = new QTimer(this);
   connect(netchecktimer, SIGNAL(timeout()), this, SLOT(netcheckfunc()));
   netchecktimer->start(2000);

}

CMainWindow::~CMainWindow()
{
	if(usbMonitorThread->isRunning()){//by xzg
		usbMonitorThread->terminate();
		delete usbMonitorThread;
		usbMonitorThread = NULL;
	}
}

void CMainWindow::dealUsbHotPlugEvent(bool f)//by xzg
{
	if(m_usbConfig != NULL){
		m_usbConfig->setUsbAddOrRemove(f);
		m_usbConfig->read_usb_list();
		list<UsbDeviceInfo > dev_list;
		dev_list.clear();
		m_usbConfig->readfromfile(dev_list);
		//qDebug() << "#####usb dev size =======================" << dev_list.size();
		if(!dev_list.empty()){
			model->clear();
			QStringList header;
			header << QStringLiteral("设备名称") << QStringLiteral("设备类型") << QStringLiteral("透传设置");
			model->setHorizontalHeaderLabels(header);
			
			list<UsbDeviceInfo>::iterator it = dev_list.begin();
			for(; it != dev_list.end(); it++){
				QString devName(it->product_name);
				QStringList devNameType = devName.split(" ");
				if(devNameType.count() > 1){
					QString dev;
					QString name;
					dev = devNameType.at(0);
					for(int i = 1; i < devNameType.count(); i++){
						name.append(devNameType.at(i));
						name.append(QString(" "));
					}
					QStandardItem *item1 = new QStandardItem(dev);
					QStandardItem *item2 = new QStandardItem(name);
					QStandardItem *item3 = new QStandardItem(QStringLiteral("透传"));
					item3->setCheckable(true);
					item3->setData(it->vid, Qt::UserRole + 1);
					item3->setData(it->pid, Qt::UserRole + 2);
					if(!it->enable)
						item3->setCheckState(Qt::Checked);
					else
						item3->setCheckState(Qt::Unchecked);
					QList<QStandardItem *> itemlist;
					itemlist.append(item1);
					itemlist.append(item2);
					itemlist.append(item3);
					model->appendRow(itemlist);
				}
			}
			tb->setColumnWidth(0, 125);
			tb->setColumnWidth(1, 130);
			tb->setColumnWidth(2, 65);
		}
	}
}

void CMainWindow::createFirstLoginWidget()
{

   firstLoginWidget = new QWidget();
   firstLoginWidget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);
   QVBoxLayout *mainLayout = new QVBoxLayout;



   photoLabel = new CUserCard(1, firstLoginWidget, 0, 1, 0, 1);
   photoLabel->SetMainW(this);
   //photoLabel->setFixedSize(74, 74);
   qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
   QString randpic = QString(":/userpic/p%1").arg((qrand() % 11) + 1);
   photoLabel->setPixmap(randpic);
   QHBoxLayout* photo_layout = new QHBoxLayout();
   photo_layout->setAlignment(Qt::AlignCenter);
   photo_layout->addWidget(photoLabel);

   userTextEdit = new CMyTextEdit(":/first_login_res/input", QStringLiteral("用户名"), 190, 30);
   passwordTextEdit = new CMyTextEdit(":/first_login_res/input", QStringLiteral("密码"), 190, 30,0,0,1);


   firstLoginBtn = new QPushButton();
   firstLoginBtn->setFixedSize(190, 30);
   firstLoginBtn->setObjectName("firstLoginBtn");
   firstLoginBtn->setStyleSheet("QPushButton#firstLoginBtn{border-image: url(:/first_login_res/btn_pressed);}"
         "QPushButton#firstLoginBtn:hover{border-image: url(:/first_login_res/btn_hover);}"
         "QPushButton#firstLoginBtn:pressed{border-image: url(:/first_login_res/btn_pressed);}");

   connect(firstLoginBtn, SIGNAL(clicked()), this, SLOT(clicklogin()));



   mainLayout->setAlignment(Qt::AlignCenter);
   mainLayout->setContentsMargins(0, 80, 0, 0);

   mainLayout->addLayout(photo_layout);


   QVBoxLayout* v_layout = new QVBoxLayout;
   v_layout->addSpacing(20);
   v_layout->addWidget(userTextEdit);
   v_layout->addSpacing(3);
   v_layout->addWidget(passwordTextEdit);
   v_layout->addSpacing(6);
   v_layout->addSpacing(30);
   v_layout->addWidget(firstLoginBtn);


   //test
   QHBoxLayout* load_hl = new QHBoxLayout;
   load_hl->setAlignment(Qt::AlignCenter);
   floading = new CLoading();
   floading->setFixedSize(150,25);
   //loading->setGeometry((currentScreenWidth - 150) / 2, 500, 150, 25);
   load_hl->addWidget(floading);
   v_layout->addLayout(load_hl);

   QHBoxLayout* h_layout = new QHBoxLayout;
   h_layout->setAlignment(Qt::AlignCenter);
   h_layout->addLayout(v_layout);

   mainLayout->addLayout(h_layout);

   mainLayout->addSpacing(74);

   firstLoginWidget->setLayout(mainLayout);
}

void CMainWindow::createSecondLoginWidget()
{

   secondLoginWidget = new QWidget();
   secondLoginWidget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);

   secondwuserpic = new CUserCard(1, 0, 1, 1, 1, 0);
   secondwuserpic->setPixmap((QString(":/changeuser/default")));
   secondwuserpic->setText(uname);

   edit = new CLeftArrawPWDEdit;
   edit->setMainWin(this);
   edit->setFixedSize(190, 32);

   secondLoginBtn = new QPushButton;
   secondLoginBtn->setFixedSize(28,28);
   secondLoginBtn->setObjectName("secondLoginBtn");
   secondLoginBtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
   secondLoginBtn->setStyleSheet("QPushButton#secondLoginBtn{border-image: url(:/second_logres/lnomal);}"
         "QPushButton#secondLoginBtn:hover{border-image: url(:/second_logres/lnomal);}"
         "QPushButton#secondLoginBtn:pressed{border-image: url(:/second_logres/lclick);}");
   secondLoginBtn->setIconSize(QSize(28, 28));
   connect(secondLoginBtn, SIGNAL(clicked()), this, SLOT(clicklogin()));


   QPushButton* changpwdbtn = new QPushButton;
   changpwdbtn->setFixedSize(48, 12);
   changpwdbtn->setObjectName("changpwdbtn");
   changpwdbtn->setStyleSheet("QPushButton#changpwdbtn{border-image: url(:/second_logres/changepwdnomal);}"
         "QPushButton#changpwdbtn:pressed{border-image: url(:/second_logres/changpwdclicked);}");
   connect(changpwdbtn, SIGNAL(clicked()), this, SLOT(changepwd()));


   sloading = new CLoading;
   sloading->setFixedSize(150,25);

   QHBoxLayout* sl_hl = new QHBoxLayout;
   sl_hl->setAlignment(Qt::AlignCenter);
   sl_hl->addWidget(sloading);

   QHBoxLayout* pic_layout = new QHBoxLayout;
   pic_layout->setAlignment(Qt::AlignCenter);
   pic_layout->addWidget(secondwuserpic);

   QHBoxLayout* eidt_layout = new QHBoxLayout;
   eidt_layout->setAlignment(Qt::AlignCenter);
   eidt_layout->addStretch(62);
   eidt_layout->addWidget(edit);
   eidt_layout->addWidget(secondLoginBtn);
   eidt_layout->addStretch(58);

   QHBoxLayout* changebtn_layout = new QHBoxLayout;
   changebtn_layout->addStretch(82);
   changebtn_layout->addWidget(changpwdbtn);
   changebtn_layout->addStretch(58);


   QVBoxLayout* main_layout = new QVBoxLayout;
   main_layout->setAlignment(Qt::AlignCenter);
   main_layout->addLayout(pic_layout);
   main_layout->addSpacing(30);
   main_layout->addLayout(eidt_layout);
   main_layout->addSpacing(20);
   main_layout->addLayout(changebtn_layout);
   main_layout->addSpacing(50);
   main_layout->addLayout(sl_hl);

   secondLoginWidget->setLayout(main_layout);

}



void CMainWindow::createMoreThan3Widget()
{
   morethan3Widget = new QWidget();
   morethan3Widget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);


   QHBoxLayout* h_layout = new QHBoxLayout;
   h_layout->setAlignment(Qt::AlignCenter);
   h_layout->setContentsMargins(0, 0, 0, 170);
   h_layout->setSpacing(30);

   QPushButton* lbtn = new QPushButton;
   QPushButton* rbtn = new QPushButton;
   lbtn->setFixedSize(17, 17);
   lbtn->setObjectName("lbtn");
   lbtn->setStyleSheet("QPushButton#lbtn{border-image: url(:/morethan3/lnomal);}"
         "QPushButton#lbtn:hover{border-image: url(:/morethan3/lhover);}"
         "QPushButton#lbtn:pressed{border-image: url(:/morethan3/lclick);}");

   connect(lbtn, SIGNAL(clicked()), this, SLOT(turnleft()));

   rbtn->setFixedSize(17, 17);
   rbtn->setObjectName("rbtn");
   rbtn->setStyleSheet("QPushButton#rbtn{border-image: url(:/morethan3/rnomal);}"
         "QPushButton#rbtn:hover{border-image: url(:/morethan3/rhover);}"
         "QPushButton#rbtn:pressed{border-image: url(:/morethan3/rclick);}");

   connect(rbtn, SIGNAL(clicked()), this, SLOT(turnright()));

   h_layout->addWidget(lbtn);

   for (int i = 0; i < 3; i++)
   {
      CUserCard* card = new CUserCard(1, 0, 1, 0, 0, 1,0,1);
      card->SetMainW(this);
      cardarray[i] = card;
      h_layout->addWidget(card);

   }

   CUserCard* card = new CUserCard(1, 0, 1, 1, 1, 1,1);
   card->setPixmap((QString(":/changeuser/addusernomal")), 110, 110);
   card->SetMainW(this);


   h_layout->addWidget(card);
   h_layout->addWidget(rbtn);

   QVBoxLayout* main_layout = new QVBoxLayout;
   main_layout->setAlignment(Qt::AlignCenter);
   main_layout->setContentsMargins(0, 100, 0, 80);

   main_layout->addLayout(h_layout);
   morethan3Widget->setLayout(main_layout);

   mainLayout->addWidget(morethan3Widget);
}

void CMainWindow::createChangePassWidget()
{
   changePswdWidget = new QWidget;
   changePswdWidget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);
   cpwdusericon = new QLabel;
   cpwdusericon->setFixedSize(48, 48);
   cpwdusericon->setPixmap(QPixmap(QString(":/userpic/p1")));

   cpwdusername = new QLabel;
   cpwdusername->setFixedSize(150, 28);
   cpwdusername->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   cpwdusername->setText("929015200@qq.com");

   QLabel* changepwd = new QLabel;
   changepwd->setFixedSize(100, 28);
   changepwd->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   changepwd->setText(QStringLiteral("修改密码"));

   pwdeidt = new CMyTextEdit(":/first_login_res/input", QStringLiteral("原始密码"), 270, 30,0,0,1);
   newpwdeidt = new CMyTextEdit(":/first_login_res/input", QStringLiteral("新密码"), 270, 30,0,0,1);
   surenewpwdeidt = new CMyTextEdit(":/first_login_res/input", QStringLiteral("确认新密码"), 270, 30,0,0,1);

   QPushButton *returnBtn = new QPushButton();
   returnBtn->setFixedSize(70, 30);
   returnBtn->setObjectName("returnBtn");
   returnBtn->setStyleSheet("QPushButton#returnBtn{border-image: url(:/changepwd/retnomal);}"
         "QPushButton#returnBtn:hover{border-image: url(:/changepwd/rethover);}"
         "QPushButton#returnBtn:pressed{border-image: url(:/changepwd/retclicked);}");

   connect(returnBtn, SIGNAL(clicked()), this, SLOT(changepwdret()));

   QPushButton *sureBtn = new QPushButton();
   sureBtn->setFixedSize(70, 30);
   sureBtn->setObjectName("sureBtn");
   sureBtn->setStyleSheet("QPushButton#sureBtn{border-image: url(:/changepwd/surenomal);}"
         "QPushButton#sureBtn:hover{border-image: url(:/changepwd/surehover);}"
         "QPushButton#sureBtn:pressed{border-image: url(:/changepwd/sureclicked);}");
   connect(sureBtn, SIGNAL(clicked()), this, SLOT(changepwdsure()));

   QHBoxLayout* h_usericon_name = new QHBoxLayout;
   h_usericon_name->setAlignment(Qt::AlignCenter);
   h_usericon_name->addStretch(36);
   h_usericon_name->addWidget(cpwdusericon);
   h_usericon_name->addStretch(3);
   h_usericon_name->addWidget(cpwdusername);
   h_usericon_name->addStretch(42);

   QHBoxLayout* h_changepwd = new QHBoxLayout;
   h_changepwd->addStretch(36);
   h_changepwd->addWidget(changepwd);
   h_changepwd->addStretch(60);

   QHBoxLayout* h_btn_layout = new QHBoxLayout;
   h_btn_layout->setAlignment(Qt::AlignCenter);
   h_btn_layout->addStretch(54);
   h_btn_layout->addWidget(returnBtn);
   h_btn_layout->addStretch(1);
   h_btn_layout->addWidget(sureBtn);
   h_btn_layout->addStretch(37);

   QVBoxLayout* v_layout = new QVBoxLayout;
   v_layout->setAlignment(Qt::AlignCenter);

   v_layout->addWidget(pwdeidt);
   v_layout->addSpacing(3);
   v_layout->addWidget(newpwdeidt);
   v_layout->addSpacing(3);
   v_layout->addWidget(surenewpwdeidt);


   QHBoxLayout* h_layout = new QHBoxLayout;
   h_layout->setAlignment(Qt::AlignCenter);
   h_layout->addLayout(v_layout);

   QVBoxLayout* main_layout = new QVBoxLayout;
   main_layout->setAlignment(Qt::AlignCenter);
   main_layout->addLayout(h_usericon_name);
   main_layout->addSpacing(15);
   main_layout->addLayout(h_changepwd);
   main_layout->addSpacing(15);
   main_layout->addLayout(h_layout);
   main_layout->addSpacing(10);
   main_layout->addLayout(h_btn_layout);
   main_layout->addStretch();
   main_layout->setContentsMargins(0, 160, 0, 0);

   QHBoxLayout* main_l = new QHBoxLayout;
   main_l->setAlignment(Qt::AlignCenter);

   main_l->addLayout(main_layout);

   changePswdWidget->setLayout(main_l);
}

void CMainWindow::createCloudHostSelectWidget()
{
   chostselectWdiget = new QWidget;
   chostselectWdiget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);

   QLabel* usericon = new QLabel;
   usericon->setFixedSize(48, 48);
   usericon->setPixmap(QPixmap(upic));

   QLabel* username = new QLabel;
   username->setFixedSize(100, 20);
   username->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   username->setText(uname);

   QLabel* selectlabel = new QLabel;
   selectlabel->setFixedSize(150, 20);
   selectlabel->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   selectlabel->setText(QStringLiteral("请选择您需要登录的云主机"));



   QScrollArea *pArea = new QScrollArea(chostselectWdiget);
   setScrollSheet(pArea);
   pArea->viewport()->setStyleSheet("background-color:transparent;");

   hostlistwd = new CHostList(pArea);//需要滚动的是一个Qwidget，而如果是在设计器里面拖入控件，会自动添加一个

   map<QString, K_yType>::iterator it = m_hostinfo.begin();
   while (it != m_hostinfo.end())
   {
      hostlistwd->AddCloudItem((it->second)["host_name"],it->second);
      it++;
   }

   // 	hostlistwd->AddCloudItem(QString("Windows xp"));
   // 	hostlistwd->AddCloudItem(QString("Windows 7"));
   // 	hostlistwd->AddCloudItem(QString("Windows 8.1"));
   // 	hostlistwd->AddCloudItem(QString("Windows 9.1"));
   // 	hostlistwd->AddCloudItem(QString("Windows 10.1"));
   hostlistwd->endAdd();

   pArea->setWidget(hostlistwd);//这里设置滚动窗口qw，
   pArea->setFixedSize(280, 92);//要显示的区域大小


   QPushButton *returnBtn = new QPushButton();
   returnBtn->setFixedSize(70, 30);
   returnBtn->setObjectName("retBtn");
   returnBtn->setStyleSheet("QPushButton#retBtn{border-image: url(:/hostselect/retnomal);}"
         "QPushButton#retBtn:hover{border-image: url(:/hostselect/rethover);}"
         "QPushButton#retBtn:pressed{border-image: url(:/hostselect/retclicked);}");
   connect(returnBtn, SIGNAL(clicked()), this, SLOT(hostret()));

   m_okBtn = new QPushButton();
   m_okBtn->setFixedSize(70, 30);
   m_okBtn->setObjectName("okBtn");
   m_okBtn->setStyleSheet("QPushButton#okBtn{border-image: url(:/hostselect/surenomal);}"
         "QPushButton#okBtn:hover{border-image: url(:/hostselect/surehover);}"
         "QPushButton#okBtn:pressed{border-image: url(:/hostselect/sureclicked);}");

   m_okBtn->setToolTip(QStringLiteral("需要等一会儿"));
   connect(m_okBtn, SIGNAL(clicked()), this, SLOT(logintohost()));
   //connect(this, SIGNAL(spicelogsignal(K_yType)), this, SLOT(dologintohost(K_yType)));


   hloading = new CLoading();
   //loading->setGeometry((currentScreenWidth - 150) / 2, 500, 150, 25);


   QHBoxLayout* logo_l = new QHBoxLayout;
   logo_l->setAlignment(Qt::AlignCenter);
   logo_l->addStretch(42);
   logo_l->addWidget(usericon);
   logo_l->addWidget(username);
   logo_l->addStretch(59);

   QHBoxLayout* s_layout = new QHBoxLayout;
   s_layout->setAlignment(Qt::AlignCenter);
   s_layout->addStretch(41);
   s_layout->addWidget(selectlabel);
   s_layout->addStretch(59);


   QHBoxLayout* hostlist_l = new QHBoxLayout;
   hostlist_l->setAlignment(Qt::AlignCenter);
   hostlist_l->addWidget(pArea);

   QHBoxLayout* btn_layout = new QHBoxLayout;
   btn_layout->addStretch(45);
   btn_layout->addWidget(returnBtn);
   btn_layout->addWidget(m_okBtn);
   btn_layout->addStretch(30);
   QVBoxLayout* main_layout = new QVBoxLayout;
   main_layout->setAlignment(Qt::AlignCenter);

   QHBoxLayout* load_hl = new QHBoxLayout;
   load_hl->setAlignment(Qt::AlignCenter);
   load_hl->addWidget(hloading);

   main_layout->addLayout(logo_l);
   main_layout->addSpacing(20);
   main_layout->addLayout(s_layout);

   main_layout->addLayout(hostlist_l);

   main_layout->addLayout(btn_layout);
   //main_layout->addSpacing(10);
   main_layout->addLayout(load_hl);


   main_layout->setContentsMargins(0, 200, 0, 100);
   chostselectWdiget->setLayout(main_layout);


   mainLayout->addWidget(chostselectWdiget);

}

void CMainWindow::createChangeUserWidget()
{
   // 
   // 
   // 	
   // 
   // 			morethan3Widget = new QWidget();
   // 			morethan3Widget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);
   // 
   // 
   // 
   // 			QHBoxLayout* h_layout = new QHBoxLayout;
   // 			h_layout->setAlignment(Qt::AlignCenter);
   // 			h_layout->setContentsMargins(0, 0, 0, 150);
   // 			h_layout->setSpacing(30);
   // 
   // 			QPushButton* lbtn = new QPushButton;
   // 			QPushButton* rbtn = new QPushButton;
   // 			lbtn->setFixedSize(17, 17);
   // 			lbtn->setObjectName("lbtn");
   // 			lbtn->setStyleSheet("QPushButton#lbtn{border-image: url(:/morethan3/lnomal);}"
   // 				"QPushButton#lbtn:hover{border-image: url(:/morethan3/lhover);}"
   // 				"QPushButton#lbtn:pressed{border-image: url(:/morethan3/lclick);}");
   // 
   // 			connect(lbtn, SIGNAL(clicked()), this, SLOT(turnleft()));
   // 
   // 			rbtn->setFixedSize(17, 17);
   // 			rbtn->setObjectName("rbtn");
   // 			rbtn->setStyleSheet("QPushButton#rbtn{border-image: url(:/morethan3/rnomal);}"
   // 				"QPushButton#rbtn:hover{border-image: url(:/morethan3/rhover);}"
   // 				"QPushButton#rbtn:pressed{border-image: url(:/morethan3/rclick);}");
   // 
   // 			connect(rbtn, SIGNAL(clicked()), this, SLOT(turnright()));
   // 
   // 			h_layout->addWidget(lbtn);
   // 
   // 			for (int i = 0; i < 3; i++)
   // 			{
   // 				CUserCard* card = new CUserCard(1, 0, 1, 0, 0, 1);
   // 				card->SetMainW(this);
   // 				cardarray[i] = card;
   // 				USERCARDINFO info;
   // 				elemtype elem;
   // 				int suc = list_getelem(userlist, i+1, &elem);
   // 				if (elem)
   // 				{
   // 					info = *((USERCARDINFO*)elem);
   // 				}
   // 				card->setPixmap(*info.logopath);
   // 				h_layout->addWidget(card);
   // 
   // 			}
   // 			list_getelem(userlist, 1, &cur);
   // 
   // 			CUserCard* card = new CUserCard(1, 0, 1, 1, 1, 1);
   // 			card->setPixmap((QString(":/changeuser/addusernomal")), 110, 110);
   // 			h_layout->addWidget(card);
   // 			h_layout->addWidget(rbtn);
   // 
   // 			QVBoxLayout* main_layout = new QVBoxLayout;
   // 			main_layout->setAlignment(Qt::AlignCenter);
   // 			main_layout->setContentsMargins(0, 100, 0, 80);
   // 
   // 			main_layout->addLayout(h_layout);
   // 			morethan3Widget->setLayout(main_layout);
   // 
   // 			mainLayout->addWidget(morethan3Widget);
   // 			mainLayout->setCurrentWidget(morethan3Widget);
   // 
   // 		}
   // 		else
   // 		{
   // 			changeuserwidget = new QWidget;
   // 			changeuserwidget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);
   // 			QGridLayout* midlayout = new QGridLayout;
   // 			midlayout->setSpacing(30);
   // 			for (int i = 0; i < size;i++)
   // 			{
   // 				CUserCard* leftbtn = new CUserCard(1, 0, 1, 0, 1, 1);
   // 				leftbtn->setPixmap(settings->value(keys.at(i)).toString());
   // 				midlayout->addWidget(leftbtn, 0, i);
   // 			}
   // 			
   // 			CUserCard* rightbtn = new CUserCard(1, 0, 1, 1, 1, 1);
   // 			rightbtn->setPixmap((QString(":/changeuser/addusernomal")));
   // 
   // 			midlayout->addWidget(rightbtn, 0, size);
   // 			midlayout->setContentsMargins(0, 0, 0, 150);
   // 
   // 			QHBoxLayout* h_layout = new QHBoxLayout;
   // 			h_layout->setAlignment(Qt::AlignCenter);
   // 			h_layout->addLayout(midlayout);
   // 
   // 
   // 			QVBoxLayout* main_layout = new QVBoxLayout;
   // 			main_layout->addLayout(h_layout);
   // 
   // 			changeuserwidget->setLayout(main_layout);
   // 
   // 			mainLayout->addWidget(changeuserwidget);
   // 			mainLayout->setCurrentWidget(changeuserwidget);
   // 		}
   // 		
   // 
   // 	}

}

void CMainWindow::setScrollSheet(QScrollArea* scroll)
{

   scroll->setStyleSheet("QScrollArea"
         "{"
         "background-color:transparent;"
         "border-radius:0px;"
         "}"
         "QScrollBar:vertical"
         "{"
         "width:3px;"
         "background:rgba(0,0,0,100%);"
         "background:url(:/hostselect/spiderbg);"
         "margin:0px,0px,0px,10px;"
         "padding-top:1px;"
         "padding-bottom:1px;"
         "}"
         "QScrollBar::handle:vertical"
         "{"
         "width:1px;"
         "background:rgba(0,0,0,25%);"
         "background:url(:/hostselect/spidernomal);"
         "border-radius:1px;"
         "min-height:22;"
         "}"
         "QScrollBar::handle:vertical:pressed"
         "{"
         "width:1px;"
         "background:rgba(0,0,0,25%);"
         "background:url(:/hostselect/spiderclicked);"
         " border-radius:1px;"
         "min-height:22;"
         "}"
         "QScrollBar::handle:vertical:hover"
         "{"
         "width:1px;"
         "background:rgba(0,0,0,50%);"
         " border-radius:1px;"
         "min-height:22;"
         "}"
         "QScrollBar::add-line:vertical"
         "{"
         "height:0px;width:0px;"
         "subcontrol-position:bottom;"
         "}"
         "QScrollBar::sub-line:vertical"
         "{"
         "height:0px;width:0px;"
         "subcontrol-position:top;"
         "}"
         "QScrollBar::add-line:vertical:hover"
         "{"
         "height:0px;width:1px;"
         "border-image:url(:/images/a/4.png);"
         "subcontrol-position:bottom;"
         "}"
         "QScrollBar::sub-line:vertical:hover"
         "{"
         "height:0px;width:1px;"
         "border-image:url(:/images/a/2.png);"
         "subcontrol-position:top;"
         "}"
         "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical"
         "{"
         "background:rgba(0,0,0,10%);"
         "border-radius:1px;"
         "}"
         );
}

bool CMainWindow::eventFilter(QObject *obj, QEvent * ev)
{
   static int i = 1;
   QMouseEvent* even = (QMouseEvent*)ev;
   if (even->type() == QEvent::MouseButtonPress)
   {
      Qt::MouseButton b = even->button();
      if (b == Qt::RightButton)
      {
         return true;
         mainLayout->setCurrentIndex(i);
         someItemOnWidget(mainLayout->currentWidget());
         i++;
         if (i == 8)
         {
            i = 0;
         }
      }

   }

   if (ev->type() == QEvent::KeyRelease)
   {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
      if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
      {
         bool count = true;
         if (NULL != msgbox)
         {
            //printf("++++++++++++++++++++++++++++++++++++++++NULL != msgbox+++++++++++++++++++++++++++++++++++++++++\n");
            if(msgbox->isClose() == false)
            {
               delete msgbox;
               msgbox = NULL;
               count = false;
            }
            else
            {
               count = true;
            }
         }
         if (count)
         {
            //printf("++++++++++++++++++++++++++++++++++++++++NULL == msgbox+++++++++++++++++++++++++++++++++++++++++\n");

            if ((mainLayout->currentWidget() == secondLoginWidget) || (mainLayout->currentWidget() == firstLoginWidget))
            {

               if(!viewer)
               {
                  clicklogin();
               }	
            }
         }
      }
   }

   // 	QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
   // 	if (keyEvent->key() == Qt::Key_Return)
   // 	{
   // 		if ((obj == secondLoginWidget) || (obj == firstLoginWidget))
   // 		{
   // 			clicklogin();
   // 		}
   // 	}


   if (even->type() == QEvent::MouseMove)
   {
      if (isviewshow)
      {
         // 			if (menu)
         // 			{
         // 				if (even->pos().y() > 0 && even->pos().y() < 30)
         // 				{
         // 
         // 					menu->show();
         // 				}
         // 				else
         // 				{
         // 					menu->hide();
         // 				}
         // 			}

      }

   }


   return QMainWindow::eventFilter(obj, ev);
}

#ifdef ZS
void CMainWindow::createLoginWidget()//by xzg
{
	loginWidget = new QWidget(main_widget);
	loginWidget->setFixedSize(currentScreenWidth, currentScreenHeight);

	QPushButton *myDesktop = new QPushButton;
	//myDesktop->setAlignment(Qt::AlignCenter);
	myDesktop->setStyleSheet("QPushButton{border-image:url(:/login_widget/MyDesktopUp);}"
							 "QPushButton:pressed{border-image:url(:/login_widget/MyDesktopDown);}");
	//myDesktop->setStyleSheet("border-image:url(:/login_widget/MyDesktop);");
	myDesktop->setFixedSize(116, 32);
	connect(myDesktop, SIGNAL(clicked()), this, SLOT(showViewer()));

	QLabel *serviceCenter = new QLabel;
	serviceCenter->setAlignment(Qt::AlignCenter);
	serviceCenter->setStyleSheet("border-image:url(:/login_widget/ServiceCenter);");
	serviceCenter->setFixedSize(169, 45);

	QLabel *information = new QLabel;
	information->setAlignment(Qt::AlignCenter);
	information->setStyleSheet("border-image:url(:/login_widget/Information);");
	information->setFixedSize(165, 60);

	QLabel *taxInteraction = new QLabel;
	taxInteraction->setAlignment(Qt::AlignCenter);
	taxInteraction->setStyleSheet("border-image:url(:/login_widget/TaxInteraction);");
	taxInteraction->setFixedSize(165, 60);
	QLabel *tool = new QLabel;
	tool->setAlignment(Qt::AlignCenter);
	tool->setStyleSheet("border-image:url(:/login_widget/Tool);");
	//tool->setStyleSheet("border:1px solid black");
	tool->setFixedSize(165, 60);
	QLabel *appCore = new QLabel;
	appCore->setAlignment(Qt::AlignCenter);
	appCore->setStyleSheet("border-image:url(:/login_widget/ApplicationCore);");
	appCore->setFixedSize(165, 60);
	//appCore->setStyleSheet("border:1px solid black");
	QLabel *train = new QLabel;
	train->setAlignment(Qt::AlignCenter);
	train->setStyleSheet("border-image:url(:/login_widget/Train);");
	train->setFixedSize(165, 60);
	//train->setStyleSheet("border:1px solid black");

	QWidget *widget[5];// = new QWidget;
	for(int i = 0; i < 5; i++){
		widget[i] = new QWidget;
		//widget[i]->setStyleSheet(".QWidget{border:1px solid #4F4F4F; border-radius:5px;}");
		widget[i]->setStyleSheet(".QWidget{border-image:url(:login_widget/Background); border-radius:5px;}");
		widget[i]->setFixedSize(240, 400);
	}
	QLabel *tt[5];
	for(int i = 0; i < 4; i++){
		tt[i] = new QLabel(QString::fromLocal8Bit("正在上线"));
	}
	QPushButton *generTaxBtn = new QPushButton;
	generTaxBtn->setFixedSize(65, 60);
	connect(generTaxBtn, SIGNAL(clicked()), this, SLOT(showViewer()));
	generTaxBtn->setStyleSheet("QPushButton{border-image:url(:/login_widget/GenerTaxUp);}"
							   //"QPushButton:hover{border-image:url(:/login_widget/GenerTaxDown);}"
							   "QPushButton:pressed{border-image:url(:/login_widget/GenerTaxDown);}");
	QPushButton *smallTaxBtn = new QPushButton;
	smallTaxBtn->setFixedSize(65, 60);
	smallTaxBtn->setStyleSheet("QPushButton{border-image:url(:/login_widget/SmallTaxUp);}"
							   //"QPushButton:hover{border-image:url(:/login_widget/SmallTaxDown);}"
							   "QPushButton:pressed{border-image:url(:/login_widget/SmallTaxDown);}");
	connect(smallTaxBtn, SIGNAL(clicked()), this, SLOT(openWebView()));

	QPushButton *billPrintBtn = new QPushButton;
	billPrintBtn->setFixedSize(65, 60);
	connect(billPrintBtn, SIGNAL(clicked()), this, SLOT(showViewer()));
	billPrintBtn->setStyleSheet("QPushButton{border-image:url(:/login_widget/TaxPrintUp);}"
								//"QPushButton:hover{border-image:url(:/login_widget/TaxPrintDown);}"
								"QPushButton:pressed{border-image:url(:/login_widget/TaxPrintDown);}");
	QGridLayout *gridlayout[5];
	for(int i = 0; i < 5; i++){
		gridlayout[i] = new QGridLayout;
		gridlayout[i]->setSpacing(25);
	}
	gridlayout[0]->addWidget(tt[0], 0, 0);
	gridlayout[1]->addWidget(tt[1], 0, 0);
	gridlayout[2]->addWidget(tt[2], 0, 0);
	gridlayout[3]->addWidget(generTaxBtn, 0, 0);
	gridlayout[3]->addWidget(smallTaxBtn, 0, 1);
	gridlayout[3]->addWidget(billPrintBtn, 1, 0);
	gridlayout[4]->addWidget(tt[3], 0, 0);

	QVBoxLayout *vboxlayout[5];
	for(int i = 0; i < 5; i++){
		vboxlayout[i] = new QVBoxLayout;
		vboxlayout[i]->setSpacing(25);
		vboxlayout[i]->setAlignment(Qt::AlignCenter);
	}
	vboxlayout[0]->addWidget(information);
	vboxlayout[1]->addWidget(taxInteraction);
	vboxlayout[2]->addWidget(tool);
	vboxlayout[3]->addWidget(appCore);
	vboxlayout[4]->addWidget(train);
	for(int i = 0; i < 5; i++){
		vboxlayout[i]->addLayout(gridlayout[i]);
		vboxlayout[i]->addStretch();
	}

	for(int i = 0; i < 5; i++){
		widget[i]->setLayout(vboxlayout[i]);
	}
	QVBoxLayout *mainVlayout[5];
	for(int i = 0; i < 5; i++){
		mainVlayout[i] = new QVBoxLayout;
		mainVlayout[i]->setAlignment(Qt::AlignVCenter);
		mainVlayout[i]->setSpacing(50);
		QLabel *labelUp = new QLabel;
		labelUp->setFixedSize(169, 45);
		QLabel *labelDown = new QLabel;
		labelDown->setFixedSize(116, 32);
		if(i == 0){
			mainVlayout[i]->addWidget(serviceCenter);
			delete labelUp;
			labelUp = NULL;
		}else{
			mainVlayout[i]->addWidget(labelUp);
		}
		mainVlayout[i]->addWidget(widget[i]);
		if(i == 4){
			QHBoxLayout *hlay = new QHBoxLayout;
			labelDown->setFixedSize(34, 32);
			hlay->addWidget(labelDown);
			hlay->addWidget(myDesktop);
			//mainVlayout[i]->addWidget(myDesktop);
			mainVlayout[i]->addLayout(hlay);
		}else{
			mainVlayout[i]->addWidget(labelDown);
		}
	}
	QHBoxLayout *hboxlayout = new QHBoxLayout;
	hboxlayout->setSpacing(2);
	for(int i = 0; i < 5; i++){
		//hboxlayout->addWidget(widget[i]);
		hboxlayout->addLayout(mainVlayout[i]);
	}
	hboxlayout->setAlignment(Qt::AlignCenter);
	loginWidget->setLayout(hboxlayout);
	loginWidget->hide();
}
void CMainWindow::setViewerGrabKeyboard()//by xzg
{
	if(viewer != NULL){
		viewer->setGrabKeyboard();
	}
	menu->show();
	logo_widget->show();
}
void CMainWindow::openWebView()//by xzg
{
	webView->setMainPage();
	webView->show();
	menu->hide();
	logo_widget->hide();
}
void CMainWindow::showViewer()//by xzg
{
	loginWidget->hide();
	viewer->show();
}
void CMainWindow::showLoginWidget()//by xzg
{
	viewer->hide();
	center_widget->hide();
	userchangeBtn->hide();
	changsetBtn->hide();
	loginWidget->show();
	menu->show();
	logo_widget->show();
}
void CMainWindow::showCenterWidget()//by xzg
{
	if(menu != NULL)
		menu->hide();
	if(viewer != NULL)
		viewer->hide();
	loginWidget->hide();
	userchangeBtn->show();
	changsetBtn->show();
	center_widget->show();
	logo_widget->show();
}
#endif

void CMainWindow::createsetsvrurlWidget()
{
   setsvrurlWidget = new QWidget;
   setsvrurlWidget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);
   QLabel* txtlabel = new QLabel;
   txtlabel->setFixedSize(270, 30);
   txtlabel->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:15px;border:0px;");
   txtlabel->setText(tr("请初始化配置服务器鉴权Url"));
   urledit = new CMyTextEdit(":/setsvrurl/input", QStringLiteral("URL:"), 270, 28);
   urledit->GetEdit()->setText("http://124.156.130.62:8080/CloudDeskTopMS/interface/connect");

   QPushButton* surebtn = new QPushButton;
   surebtn->setFixedSize(70, 30);
   surebtn->setObjectName("sureBtn");
   surebtn->setStyleSheet("QPushButton#sureBtn{border-image: url(:/setsvrurl/nomal);}"
         "QPushButton#sureBtn:hover{border-image: url(:/setsvrurl/hover);}"
         "QPushButton#sureBtn:pressed{border-image: url(:/setsvrurl/click);}");


   connect(surebtn, SIGNAL(clicked()), this, SLOT(checkurl()));

   QHBoxLayout* surebtn_layout = new QHBoxLayout;
   surebtn_layout->setAlignment(Qt::AlignCenter);
   surebtn_layout->addStretch(64);
   surebtn_layout->addWidget(surebtn);
   surebtn_layout->addStretch(36);

   QHBoxLayout* txtlabel_l = new QHBoxLayout;
   txtlabel_l->setAlignment(Qt::AlignCenter);
   txtlabel_l->addWidget(txtlabel);

   QHBoxLayout* urledit_l = new QHBoxLayout;
   urledit_l->setAlignment(Qt::AlignCenter);
   urledit_l->addWidget(urledit);

   QVBoxLayout* main_layout = new QVBoxLayout;
   main_layout->setAlignment(Qt::AlignCenter);
   main_layout->addLayout(txtlabel_l);
   main_layout->addSpacing(10);
   main_layout->addLayout(urledit_l);
   main_layout->addSpacing(10);
   main_layout->addLayout(surebtn_layout);

   setsvrurlWidget->setLayout(main_layout);
}

void CMainWindow::someItemOnWidget(QWidget* widget)
{

   if (widget == about_netsetwidget)
   {
      if(isadduserretshow)
      {
         adduserretbtn->hide();
      }
      QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
      QString strsvrurl;
      if (settings)
      {
         strsvrurl = settings->value("server/url").toString();

      }

      QStringList list;
      QString strip;
      QString strport;
      if (!strsvrurl.isEmpty())
      {
         strsvrurl.replace("//", "");
         strsvrurl.replace("/", ":");
         list = strsvrurl.split(":");
         strip = list[1];
         strport = list[2];
      }
      uuuurledit->GetEdit()->setText(strip);
      pppportedit->GetEdit()->setText(strport);
      if (!changesetretbtn)
      {
         changesetretbtn = new QPushButton(main_widget);
         changesetretbtn->setFixedSize(28, 28);
         changesetretbtn->setObjectName("changesetretbtn");
         changesetretbtn->setStyleSheet("QPushButton#changesetretbtn{border-image: url(:/first_login_res/adduserretnomal);}"
               "QPushButton#changesetretbtn:hover{border-image: url(:/first_login_res/adduserretmove);}"
               "QPushButton#changesetretbtn:pressed{border-image: url(:/first_login_res/adduserretclick);}");
         changesetretbtn->setGeometry(30, 30, 28, 28);
         connect(changesetretbtn, SIGNAL(clicked()), this, SLOT(changesetret()));

      }
      changesetretbtn->show();
   }

   if (widget == changePswdWidget)
   {
      cpwdusername->setText(uname);
      cpwdusericon->setPixmap(QPixmap(secondwuserpic->GetPixPath()));

      pwdeidt->GetEdit()->setText("");
      newpwdeidt->GetEdit()->setText("");
      surenewpwdeidt->GetEdit()->setText("");
   }

   if (widget == firstLoginWidget)
   {
      if (cs_btn == NULL)
      {
         cs_btn = new QPushButton(main_widget);
         cs_btn->setFixedSize(48, 44);
         cs_btn->setObjectName("cs_btn");
         cs_btn->setStyleSheet("QPushButton#cs_btn{border-image: url(:/first_login_res/csnomal);}"
               "QPushButton#cs_btn:hover{border-image: url(:/first_login_res/cshover);}"
               "QPushButton#cs_btn:pressed{border-image: url(:/first_login_res/csclick);}");
         cs_btn->setGeometry(currentScreenWidth - 90, currentScreenHeight - 98, 48, 44);

         connect(cs_btn, SIGNAL(clicked()), this, SLOT(changesetting()));
      }
      cs_btn->show();

      if(isadduserretshow)
      {
         adduserretbtn->show();
      }

      if (isadduser)
      {
         int size = list_length(userlist);
         if(size > 0)
         {
            if (!adduserretbtn)
            {
               adduserretbtn = new QPushButton(main_widget);
               adduserretbtn->setFixedSize(28, 28);
               adduserretbtn->setObjectName("adduserretbtn");
               adduserretbtn->setStyleSheet("QPushButton#adduserretbtn{border-image: url(:/first_login_res/adduserretnomal);}"
                     "QPushButton#adduserretbtn:hover{border-image: url(:/first_login_res/adduserretmove);}"
                     "QPushButton#adduserretbtn:pressed{border-image: url(:/first_login_res/adduserretclick);}");
               adduserretbtn->setGeometry(30, 30, 28, 28);
               connect(adduserretbtn, SIGNAL(clicked()), this, SLOT(adduserret()));

            }
            adduserretbtn->show();
            isadduserretshow = true;
            if (!adduserretwid)
            {
               adduserretwid = prewdiget;
            }

         }
         else
         {
            if(adduserretbtn)
            {
               adduserretbtn->hide();
               isadduserretshow = false;
            }
         }

      }

   }
   if (widget == secondLoginWidget)
   {
      if (userchangeBtn == NULL)
      {
         userchangeBtn = new QPushButton(main_widget);
         userchangeBtn->setFixedSize(48, 44);
         userchangeBtn->setObjectName("userchangeBtn");
         userchangeBtn->setStyleSheet("QPushButton#userchangeBtn{border-image: url(:/second_logres/changnomal);}"
               "QPushButton#userchangeBtn:hover{border-image: url(:/second_logres/changhover);}"
               "QPushButton#userchangeBtn:pressed{border-image: url(:/second_logres/changclicked);}");
         userchangeBtn->setGeometry(currentScreenWidth - 90 - 90, currentScreenHeight - 98, 48, 44);
         connect(userchangeBtn, SIGNAL(clicked()), this, SLOT(changeuser()));
      }
      if (!changsetBtn)
      {
         changsetBtn = new QPushButton(main_widget);
         changsetBtn->setFixedSize(48, 44);
         changsetBtn->setObjectName("changsetBtn");
         changsetBtn->setStyleSheet("QPushButton#changsetBtn{border-image: url(:/second_logres/changesetnomal);}"
               "QPushButton#changsetBtn:hover{border-image: url(:/second_logres/changesethover);}"
               "QPushButton#changsetBtn:pressed{border-image: url(:/second_logres/changesetclicked);}");
         changsetBtn->setGeometry(currentScreenWidth - 90, currentScreenHeight - 98, 48, 44);

         connect(changsetBtn, SIGNAL(clicked()), this, SLOT(changesetting()));

      }
      userchangeBtn->show();
      changsetBtn->show();
      edit->inputEdit->setText("");

   }

   if (widget != firstLoginWidget)
   {
      if (cs_btn)
      {
         cs_btn->hide();
      }
   }
   if (widget != secondLoginWidget)
   {
      if (changsetBtn)
      {
         changsetBtn->hide();
      }
      if (userchangeBtn)
      {
         userchangeBtn->hide();
      }
   }

}

void CMainWindow::createAbout_NetSettingWidget()
{
   about_netsetwidget = new QWidget;
   about_netsetwidget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);

   MyLabel* syssetLabel = new MyLabel();
   syssetLabel->setFixedSize(80, 30);
   syssetLabel->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   syssetLabel->setText(QStringLiteral("分辨率管理"));
   connect(syssetLabel, SIGNAL(clicked()), this, SLOT(sysclickfunc()));

   MyLabel* netlabel = new MyLabel();
   netlabel->setFixedSize(80, 30);
   netlabel->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   netlabel->setText(QStringLiteral("网络配置"));
   connect(netlabel, SIGNAL(clicked()), this, SLOT(netlclickfunc()));



   MyLabel* netdiagnoselabel = new MyLabel();
   netdiagnoselabel->setFixedSize(80, 30);
   netdiagnoselabel->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   netdiagnoselabel->setText(QStringLiteral("网络诊断"));
   connect(netdiagnoselabel, SIGNAL(clicked()), this, SLOT(netdiaglclickfunc()));


   MyLabel* aboutlabel = new MyLabel();
   aboutlabel->setFixedSize(50, 30);
   aboutlabel->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   aboutlabel->setText(QStringLiteral("关于"));
   connect(aboutlabel, SIGNAL(clicked()), this, SLOT(aboutlclickfunc()));

/***************by xzg****************/
   MyLabel* usbsetlabel = new MyLabel();
   usbsetlabel->setFixedSize(100, 30);
   usbsetlabel->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   usbsetlabel->setText(QStringLiteral("USB透传设置"));
   connect(usbsetlabel, SIGNAL(clicked()), this, SLOT(usbsetlclickfunc()));
/***************by xzg****************/


   spilter = new CNetAboutSpilter;
   spilter->moveSlider(0);
   QVBoxLayout* letf_l = new QVBoxLayout;
   letf_l->setContentsMargins(0, 140, 0, 0);
   letf_l->setAlignment(Qt::AlignTop);
   letf_l->addWidget(netlabel);
   letf_l->addWidget(netdiagnoselabel);
   letf_l->addWidget(syssetLabel);
   letf_l->addWidget(aboutlabel);
   letf_l->addWidget(usbsetlabel);//by xzg



   createNetSettingWidget();
   createAboutWidget();
   createNetDiagnoseWidget();
   createSysSettingWidget();
   createUsbSettingWidget();//by xzg

   QWidget* rightwidget = new QWidget;
   rightwidget->setFixedSize(500, 500);

   netrightglayout = new QStackedLayout;
   netrightglayout->addWidget(netwidget);
   netrightglayout->addWidget(netdiagnosewidget);
   netrightglayout->addWidget(syssetwidget);
   netrightglayout->addWidget(aboutwidget);
   netrightglayout->addWidget(usbsetWidget);//by xzg
   netrightglayout->setCurrentIndex(0);
   rightwidget->setLayout(netrightglayout);

   QVBoxLayout* rblayout = new QVBoxLayout;
   rblayout->setContentsMargins(0, 120, 0, 0);
   rblayout->addWidget(rightwidget);

   QHBoxLayout* left_hl = new QHBoxLayout;
   left_hl->setAlignment(Qt::AlignCenter);
   left_hl->addStretch();
   left_hl->addLayout(letf_l);
   left_hl->addWidget(spilter);
   left_hl->addLayout(rblayout);
   about_netsetwidget->setLayout(left_hl);


}

void CMainWindow::dhcpmode(QString& ipaddr, QString& mask, QString& gateway,QString& mac)
{
    //qDebug()<<__func__<<":"<<__LINE__ ;
#ifndef OS_X86	
   system("service network-manager start");	
#endif
   getmaskAddress(ipaddr, mask,mac);
   char buff[256], ifName[20];
   bzero(buff, sizeof(buff));
   bzero(ifName, sizeof(ifName));
   get_gateway(buff, ifName);
   //qDebug()<<__func__<<":"<<__LINE__ <<"gateway="<<buff<<"ifName="<<ifName;
   QString tmpgateway(buff);
   gateway = tmpgateway;

}

void CMainWindow::usersetmode(const QString& ipaddr, const QString& mask, const QString& gateway)
{
#ifndef OS_X86	
   //set ip
   system("service network-manager start");
#endif

   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   if (settings)
   {
      settings->setValue("network/ip", ipaddr);
      settings->setValue("network/netmask", mask);
      settings->setValue("network/gateway", gateway);
      //settings->setValue("network/dhcp", QString("x"));

      settings->sync();
   }
   delete settings;
   settings = NULL;
   #ifndef OS_X86
     QProcess::startDetached(QString("/home/network"), QStringList());
   #endif
    
   
}

void CMainWindow::dhcpclickfunc()
{
   //qDebug()<<__func__<<":"<<__LINE__ << "isdhcpclick=" <<isdhcpclick;
   usersetbtn->setIcon(QIcon(QString(":/net_about/unselect")));
   dhcpbutton->setIcon(QIcon(QString(":/net_about/select")));
   netsetreadonly(true);
   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   QString isdhcp;
   if (settings)
   {
      isdhcp = settings->value("network/dhcp").toString();      
   }
   //qDebug() << "isdhcpclick:" << isdhcpclick << endl;
   //qDebug() << "isdhcp:" << isdhcp << endl;
   if ((0 == isdhcpclick) && (QString("y") == isdhcp))
   {
      MyZCLog::Instance().WriteToLog(ZCINFO, QString("isdhcp mask is y"));
      QString ip, mask, gateway,mac;
      dhcpmode(ip, mask, gateway,mac);
      ipaddredit->GetEdit()->setText(ip);
      maskedit->GetEdit()->setText(mask);
      gatewayedit->GetEdit()->setText(gateway);
      QString dns;
      readDns(dns);
      dnsedit->GetEdit()->setText(dns);
      return;
   }
   settings->setValue("network/dhcp", QString("y"));
   delete settings;
   settings = NULL;
   if (0 == isdhcpclick)
   {
      //	printf("========================================  if (0 == isdhcpclick)=========================================\n\n\n");
      MyZCLog::Instance().WriteToLog(ZCINFO, QString("isdhcpclick mask is 0"));
      if (hasonceclick)
         return;
      hasonceclick = true;
      isdhcpclick = 1;
      retbtn->setEnabled(false);
      savebtn->setEnabled(false);
      retbtn->setStyleSheet(
            "QPushButton#retbtn{border-image: url(:/net_about/retclick);}");
      savebtn->setStyleSheet(
            "QPushButton#savebtn{border-image: url(:/net_about/sureclick);}");
      QString ip, mask, gateway, dns;
      ip = mask = gateway = dns = QStringLiteral("正在获取中...");
      ipaddredit->GetEdit()->setText(ip);
      maskedit->GetEdit()->setText(mask);
      gatewayedit->GetEdit()->setText(gateway);
      dnsedit->GetEdit()->setText(dns);
//#ifndef XH
//      QProcess::startDetached(QString("/home/dhcpnetwork"), QStringList());
//#else
	 ConfigNetwork("dhcp");
//#endif
#ifndef OS_X86
      system("resolvconf -u");
#endif
     isdhcpclicktimer->start(3000);
   }
}

void CMainWindow::isdhcpclicktimerout()
{
   
   int i = get_netlink_status("eth0");
   //qDebug()<<__func__<<":"<<__LINE__ << "i=" <<i;
   if(0 == i) 
   {  
      printf("***********************************if(i = 0)*********************************\n");
      return;
   }
   QString ip, mask, gateway,mac,dns;
   readDns(dns);
   dnsedit->GetEdit()->setText(dns);
   dhcpmode(ip, mask, gateway,mac);
    //qDebug()<<__func__<<":"<<__LINE__ ;
   if(!ip.isEmpty())	ipaddredit->GetEdit()->setText(ip);
   if(!mask.isEmpty())	maskedit->GetEdit()->setText(mask);
   if(!gateway.isEmpty())	gatewayedit->GetEdit()->setText(gateway);
   if(ip.isEmpty() || mask.isEmpty() || gateway.isEmpty() || mac.isEmpty()) return;
   isdhcpclicktimer->stop();
   MyZCLog::Instance().WriteToLog(ZCINFO, QString("isdhcpclicktimerout"));
   isdhcpclick = 0;
   retbtn->setEnabled(true);
   savebtn->setEnabled(true);
   retbtn->setStyleSheet("QPushButton#retbtn{border-image: url(:/net_about/retnomal);}"
         "QPushButton#retbtn:hover{border-image: url(:/net_about/rethover);}"
         "QPushButton#retbtn:pressed{border-image: url(:/net_about/retclick);}");
   savebtn->setStyleSheet("QPushButton#savebtn{border-image: url(:/setsvrurl/nomal);}"
         "QPushButton#savebtn:hover{border-image: url(:/setsvrurl/hover);}"
         "QPushButton#savebtn:pressed{border-image: url(:/net_about/sureclick);}");
   ipaddredit->GetEdit()->setText(ip);
   maskedit->GetEdit()->setText(mask);
   gatewayedit->GetEdit()->setText(gateway);
    //qDebug()<<__func__<<":"<<__LINE__ ;
}

void CMainWindow::usersetclickfunc()
{
  ////qDebug()<<__func__<<":"<<__LINE__ << "isdhcpclick=" <<isdhcpclick;
   if (1 == isdhcpclick)
   {
      return;
   }
   hasonceclick = false;

   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   QString isdhcp;
   if (settings)
   {
      isdhcp = settings->value("network/dhcp").toString();
   }
   if (QString("y") == isdhcp)
   {
      settings->setValue("network/dhcp", QString("x"));
      QString ip, mask, gateway,broadcast,dns;
      if (settings)
      {
         ip = settings->value("network/ip").toString();
         mask = settings->value("network/netmask").toString();
         gateway = settings->value("network/gateway").toString();
        dns = settings->value("network/dns").toString();	
	QStringList qstrSubStringList = ip.split('.');
        QString broadcast = QString("%1.%2.%3.255").arg(qstrSubStringList[0]).arg(qstrSubStringList[1]).arg(qstrSubStringList[2]);
		

         ipaddredit->GetEdit()->setText(ip);
         maskedit->GetEdit()->setText(mask);
         gatewayedit->GetEdit()->setText(gateway);
         dnsedit->GetEdit()->setText(dns);
         usersetbtn->setIcon(QIcon(QString(":/net_about/select")));
         dhcpbutton->setIcon(QIcon(QString(":/net_about/unselect")));
         netsetreadonly(false);
 #ifndef OS_X86
 #else
 //#ifndef XH
 //     QProcess::startDetached(QString("/home/network"), QStringList());
//#else
	 ConfigNetwork("static",ip,mask,gateway,broadcast,dns);
//#endif
#endif
         delete settings;
         settings = NULL;
      }
   }

   
}


int CMainWindow::netcardstat()
{
   int ret = 0;
   system("mii-tool > netcard.txt");
   sleep(1);
   QFile file("./netcard.txt");
   if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
   {
      //qDebug() << "cant find netcard.txt";
      return 0;
   }

   while (!file.atEnd())
   {
      QByteArray line = file.readLine();
      QString str = QString(line.data());
      //qDebug() << str;
      if (str.contains(QString("link ok")))
      {
         ret = 1;
         //	qDebug() << "net card ok!!!!!";
         return ret;
      }

   }
   //qDebug() << "net card error!!!";
   return ret;
}


int CMainWindow::ping(QString ip)
{
   //qDebug() << ip;

   int ret = 0;
   QString fmt = "ping %1 -c 1 > ping.txt";
   QString param = fmt.arg(ip);
   QByteArray para = param.toLatin1();
   system(para.data());
   QFile file("./ping.txt");
   if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
   {
      //	qDebug() << "cant find ping.txt";
      return 0;
   }

   while (!file.atEnd())
   {
      QByteArray line = file.readLine();
      QString str = QString(line.data());
      if (str.contains(QString("ttl")) || str.contains(QString("icmp_seq")))
      {
         ret = 1;
         return ret;
      }

   }
   return ret;

}

void CMainWindow::netdiagose()
{
   int wwwtong = 0;
   int desktoptong = 0;
   int routetong = 0;
   int servertong = 0;

   char buff[256], ifName[12];
   bzero(buff, sizeof(buff));
   get_gateway(buff, ifName);
   QString getway(buff);

   QNetDiagoseEvent* routeevent = new QNetDiagoseEvent(QEvent::Type(NET_PINGROUTE));
   routetong = ping(getway);
   if (routetong)
   {
      routeevent->SetBoolean(true);
   }
   else
   {
      routeevent->SetBoolean(false);
   }
   QCoreApplication::postEvent(this, routeevent);


   wwwtong = ping("www.baidu.com");
   QNetDiagoseEvent* baiduevent = new QNetDiagoseEvent(QEvent::Type(NET_PINGWWW));
   if (wwwtong)
   {
      baiduevent->SetBoolean(true);
   }
   else
   {
      baiduevent->SetBoolean(false);
   }
   QCoreApplication::postEvent(this, baiduevent);



   QString strip = uuuurledit->GetEdit()->text();
   desktoptong = ping(strip);
   QNetDiagoseEvent* serverevent = new QNetDiagoseEvent(QEvent::Type(NET_PINGSERVER));
   if (desktoptong)
   {
      serverevent->SetBoolean(true);
   }
   else
   {
      serverevent->SetBoolean(false);
   }
   QCoreApplication::postEvent(this, serverevent);


   QNetDiagoseEvent* netcardevent = new QNetDiagoseEvent(QEvent::Type(NET_NETCARD));
   if (netcardstat())
   {
      netcardevent->SetBoolean(true);
   }
   else
   {
      netcardevent->SetBoolean(false);
   }
   QCoreApplication::postEvent(this, netcardevent);


   QNetDiagoseEvent* netuserable = new QNetDiagoseEvent(QEvent::Type(NET_USEABLE));

   if (route2www && wwwtong &&& desktoptong)
   {
      netuserable->SetBoolean(true);
   }
   else
   {
      netuserable->SetBoolean(false);
      QString fmt = "%1%2%3";
      errorstr = fmt.arg(routetong).arg(wwwtong).arg(desktoptong);
   }

   QCoreApplication::postEvent(this, netuserable);
}

void CMainWindow::netdiagnosefunc()
{

   diagnoseload->loadingshow();
   NetDiagnoseThread* thread = new NetDiagnoseThread(this);
   thread->start();
   //diagnoseload->loadinghide();
}


void CMainWindow::createNetDiagnoseWidget()
{
   netdiagnosewidget = new QWidget;
   netdiagnosewidget->setFixedSize(341, 400);


   //netdiagnosewidget->setStyleSheet("QWidget{border-image: url(:/netdiagnose/ddd);}");
   //return;

   QLabel* box = new QLabel;
   box->setFixedSize(85, 79);
   box->setPixmap(QPixmap(QString(":/netdiagnose/iconbox")));

   box2route = new QLabel;
   box2route->setFixedSize(53, 8);
   box2route->setPixmap(QPixmap(QString(":/netdiagnose/arrow0")));

   QLabel* route = new QLabel;
   route->setFixedSize(85, 79);
   route->setPixmap(QPixmap(QString(":/netdiagnose/iconroute")));

   route2www = new QLabel;
   route2www->setFixedSize(53, 8);
   route2www->setPixmap(QPixmap(QString(":/netdiagnose/arrow0")));


   QLabel* www = new QLabel;
   www->setFixedSize(85, 79);
   www->setPixmap(QPixmap(QString(":/netdiagnose/iconearth")));

   www2svr = new QLabel;
   www2svr->setFixedSize(60, 47);
   www2svr->setPixmap(QPixmap(QString(":/netdiagnose/arrow1")));

   www2web = new QLabel;
   www2web->setFixedSize(9, 35);
   www2web->setPixmap(QPixmap(QString(":/netdiagnose/arrow2")));

   QLabel* svr = new QLabel;
   svr->setFixedSize(85,79);
   svr->setPixmap(QPixmap(QString(":/netdiagnose/iconsvr")));

   QLabel* web = new QLabel;
   web->setFixedSize(85, 79);
   web->setPixmap(QPixmap(QString(":/netdiagnose/iconweb")));


   QWidget* itemwidget = new QWidget;
   itemwidget->setFixedSize(200,100);
   QVBoxLayout* iwlay_out = new QVBoxLayout;
   iwlay_out->setContentsMargins(0, 0,0, 0);

   QString szStr[] = {QStringLiteral("网卡状态"),QStringLiteral("网口状态"),QStringLiteral("DHCP状态"),
      QStringLiteral("互联网连通性"),QStringLiteral("云桌面连通性")};
   for (int i = 0; i < 5;i++)
   {
      CDiagnoseItem* item = new CDiagnoseItem;
      item->setText(szStr[i]);
      item->setStatus(true);
      iwlay_out->addWidget(item);
      itemarr[i] = item;
   }
   itemwidget->setLayout(iwlay_out);



   QWidget* totalwid = new QWidget;
   totalwid->setFixedSize(240, 40);

   staticon = new QLabel;
   staticon->setFixedSize(26, 27);
   staticon->setPixmap(QPixmap(QString(":/netdiagnose/iconbright")));

   txt = new QLabel;
   txt->setFixedSize(240, 30);
   txt->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:13px;border:0px;");
   txt->setText(QStringLiteral("网络连接正常,您可以正常使用"));

   QHBoxLayout* statl_yout = new QHBoxLayout;
   statl_yout->addWidget(staticon);
   statl_yout->addWidget(txt);
   totalwid->setLayout(statl_yout);

   QPushButton* diagnosebtn = new QPushButton;
   diagnosebtn->setObjectName("diagnosebtn");
   diagnosebtn->setFixedSize(117, 30);
   diagnosebtn->setStyleSheet("QPushButton#diagnosebtn{border-image: url(:/netdiagnose/btnnomal);}"
         "QPushButton#diagnosebtn:hover{border-image: url(:/netdiagnose/btnmove);}"
         "QPushButton#diagnosebtn:pressed{border-image: url(:/netdiagnose/btnclick);}");

   connect(diagnosebtn, SIGNAL(clicked()), this, SLOT(netdiagnosefunc()));

   QHBoxLayout* btnl_out = new QHBoxLayout;
   btnl_out->setAlignment(Qt::AlignCenter);
   diagnoseload = new CLoading;
   diagnoseload->setText(QStringLiteral("检查中"));
   btnl_out->addStretch();
   btnl_out->addWidget(diagnosebtn);
   btnl_out->addWidget(diagnoseload);


   QHBoxLayout* mid1lay_out = new QHBoxLayout;
   mid1lay_out->addStretch(90);
   mid1lay_out->addWidget(www2svr);
   mid1lay_out->addStretch(10);
   mid1lay_out->addWidget(www2web);
   mid1lay_out->addStretch(10);

   QHBoxLayout* mid2lay_out = new QHBoxLayout;
   mid2lay_out->addStretch();
   mid2lay_out->addWidget(svr);
   mid2lay_out->addWidget(web);



   QHBoxLayout* h_layout = new QHBoxLayout;
   h_layout->addWidget(box);
   h_layout->addWidget(box2route);
   h_layout->addWidget(route);
   h_layout->addWidget(route2www);
   h_layout->addWidget(www);


   QVBoxLayout* main_layout = new QVBoxLayout;
   main_layout->setContentsMargins(10, 0, 0, 0);
   main_layout->addLayout(h_layout);
   main_layout->addLayout(mid1lay_out);
   main_layout->addLayout(mid2lay_out);
   main_layout->addWidget(itemwidget);
   main_layout->addWidget(totalwid);
   main_layout->addLayout(btnl_out);

   netdiagnosewidget->setLayout(main_layout);

}

/***********by xzg******************/
void CMainWindow::usbSetTrans()
{
	int row = model->rowCount();
	int column = model->columnCount();
	int pid = 0;
	int vid = 0;
	list<UsbDeviceInfo > dev_list;
	dev_list.clear();
	m_usbConfig->readfromfile(dev_list);
	//qDebug() << "MODEL COLUMN COUNT:" << model->columnCount();
	//qDebug() << "MODEL ROW COUNT:" << model->rowCount();
	for(int i = 0; i < row; i++){
		/*for(int j = 0; j < column; j++){
			qDebug() << "STANDARD_ITEM::" << model->item(i, j)->text();//model->data(model->index(i, j)).toString();
		}*/
		vid = model->item(i, column - 1)->data(Qt::UserRole + 1).toInt();	
		//qDebug() << "VID===" << vid;	
		pid = model->item(i, column - 1)->data(Qt::UserRole + 2).toInt();	
		//qDebug() << "PID===" << pid;	
		list<UsbDeviceInfo>::iterator it = dev_list.begin();
		for(; it != dev_list.end(); it++){
		//	qDebug() << "dev_list VID=" << it->vid << "PID = " << it->pid << "rule_key=" << it->usbrule_key;
			if(vid == it->vid && pid == it->pid){
				if(model->item(i, column - 1)->checkState() == Qt::Checked)
					//it->enable = 0;
					m_usbConfig->modify_usb_enable(0, vid, pid);
				else
					//it->enable = 1;
					m_usbConfig->modify_usb_enable(1, vid, pid);
				break;
			}	
		}
	}
	//m_usbConfig->setUsbAddOrRemove(true);
	//m_usbConfig->writetofile(dev_list);
	QString rule;
	m_usbConfig->get_usb_rule(rule);
	QString message;	
	if(SpiceMulViewer::Spice_SetUsbTrans(rule)){
		message = QStringLiteral("透传设置成功");
	}else{
		message = QStringLiteral("透传设置失败");
	}
	ZCMessageBox msg(message, this, NULL);
	//qDebug() << "#######RULE =====" << rule;
	//ZCMessageBox msg(QStringLiteral("透传设置成功"), this, NULL);
	msg.exec();
}
void CMainWindow::toLoginFrame()
{
	changesetretbtn->hide();
	netrightglayout->setCurrentIndex(0);
	spilter->moveSlider(0);
	hasonceclick = false;
	if(prewdiget){
		changeWidgetTo(prewdiget);
		prewdiget = NULL;
	}
}

void CMainWindow::createUsbSettingWidget()
{
		usbsetWidget = new QWidget;
		usbsetWidget->setFixedSize(350, 380);
		/*QTableView **/tb = new QTableView(usbsetWidget);
		QPushButton *usbsetBtn = new QPushButton;
		QPushButton *returnBtn = new QPushButton;
   		usbsetBtn->setFixedSize(70, 30);
   		usbsetBtn->setObjectName("usbsetBtn");
   		usbsetBtn->setStyleSheet("QPushButton#usbsetBtn{border-image: url(:/setsvrurl/nomal);}"
        	 "QPushButton#usbsetBtn:hover{border-image: url(:/setsvrurl/hover);}"
         	 "QPushButton#usbsetBtn:pressed{border-image: url(:/net_about/sureclick);}");
		connect(usbsetBtn, SIGNAL(clicked()), this, SLOT(usbSetTrans()));
   		returnBtn->setFixedSize(70, 30);
   		returnBtn->setObjectName("returnBtn");
   		returnBtn->setStyleSheet("QPushButton#returnBtn{border-image: url(:/net_about/retnomal);}"
        	 "QPushButton#returnBtn:hover{border-image: url(:/net_about/rethover);}"
         	 "QPushButton#returnBtn:pressed{border-image: url(:/net_about/retclick);}");
		connect(returnBtn, SIGNAL(clicked()), this, SLOT(toLoginFrame()));
		QHBoxLayout *hlay = new QHBoxLayout;
		hlay->addWidget(returnBtn);
		hlay->addWidget(usbsetBtn);
		QVBoxLayout *vlay = new QVBoxLayout;
		vlay->addWidget(tb);
		vlay->addLayout(hlay);
		
   		tb->setStyleSheet("QTableView{color:white;background-color:#666666;alternate-background-color:#666666; selection-color:white;selection-background-color:#444444;border:2px groove gray;border-radius:0px;padding:2px 4px;}");
		
		tb->verticalHeader()->setVisible(false);
		/*QStandardItemModel **/model = new QStandardItemModel;
		QStringList header;
		header << QStringLiteral("设备名称") << QStringLiteral("设备类型") << QStringLiteral("透传设置");
		model->setHorizontalHeaderLabels(header);
		/*
		*/
		dealUsbHotPlugEvent(true);
		tb->setModel(model);
		tb->setColumnWidth(0, 125);
		tb->setColumnWidth(1, 130);
		tb->setColumnWidth(2, 65);
		/*
		*/
		tb->setEditTriggers(QAbstractItemView::NoEditTriggers);
		usbsetWidget->setLayout(vlay);

		usbMonitorThread = new MonitorUsbDevThread;
		connect(usbMonitorThread, SIGNAL(sendAddOrRemoveUsbDev(bool)), this, SLOT(dealUsbHotPlugEvent(bool)), Qt::QueuedConnection);
		usbMonitorThread->start();
		/*
		*/
}
/***********by xzg******************/
void CMainWindow::createNetSettingWidget()
{
   netwidget = new QWidget;
   netwidget->setFixedSize(350, 380);
   QVBoxLayout* netmainl = new QVBoxLayout;
   netmainl->setContentsMargins(20, 20, 0, 10);

   QLabel* ipaddress = new QLabel;
   ipaddress->setFixedSize(50, 14);
   ipaddress->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   ipaddress->setText(QStringLiteral("本地IP:"));
   dhcpbutton = new QPushButton;
   connect(dhcpbutton, SIGNAL(clicked()), this, SLOT(dhcpclickfunc()));
   dhcpbutton->setFixedSize(14, 14);
   dhcpbutton->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
   dhcpbutton->setIcon(QIcon(QString(":/net_about/select")));
   dhcpbutton->setIconSize(QSize(14, 14));
   QLabel* dhcp = new QLabel;
   dhcp->setFixedSize(50, 14);
   dhcp->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   dhcp->setText(QStringLiteral("DHCP"));
   usersetbtn = new QPushButton;
   connect(usersetbtn, SIGNAL(clicked()), this, SLOT(usersetclickfunc()));
   usersetbtn->setFixedSize(14, 14);
   usersetbtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
   usersetbtn->setIcon(QIcon(QString(":/net_about/unselect")));
   usersetbtn->setIconSize(QSize(14, 14));
   QLabel* userset = new QLabel;
   userset->setFixedSize(70, 14);
   userset->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   userset->setText(QStringLiteral("手动设置"));

   QHBoxLayout* h_toplayout = new QHBoxLayout;
   h_toplayout->setSpacing(10);
   h_toplayout->addWidget(ipaddress);
   h_toplayout->addWidget(dhcpbutton);
   h_toplayout->addWidget(dhcp);
   h_toplayout->addWidget(usersetbtn);
   h_toplayout->addWidget(userset);
   h_toplayout->addStretch();

   QWidget* top = new QWidget;
   top->setFixedSize(250, 30);
   top->setLayout(h_toplayout);

   QWidget* centertop = new QWidget;
   centertop->setFixedSize(250, 160);
   netset_ipaddr = new QLabel;
   netset_ipaddr->setFixedSize(42, 14);
   netset_ipaddr->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   netset_ipaddr->setText(QStringLiteral("IP地址:"));


   QString ip,mac;
   QString netmask;
   getmaskAddress(ip, netmask,mac);


   char buff[256], ifName[12];
   bzero(buff, sizeof(buff));
   get_gateway(buff, ifName);
   QString getway(buff);
   QString dns;

   ipaddredit = new CMyTextEdit(":/setsvrurl/input", "", 150, 30, 1);
   ipaddredit->GetEdit()->setText(ip);
   netset_mask = new QLabel;
   netset_mask->setFixedSize(52, 14);
   netset_mask->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   netset_mask->setText(QStringLiteral("子网掩码:"));
   maskedit = new CMyTextEdit(":/setsvrurl/input", "", 150, 30, 1);
   maskedit->GetEdit()->setText(netmask);
   netset_gateway = new QLabel;
   netset_gateway->setFixedSize(30, 14);
   netset_gateway->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   netset_gateway->setText(QStringLiteral("网关:"));
   gatewayedit = new CMyTextEdit(":/setsvrurl/input", "", 150, 30, 1);
   gatewayedit->GetEdit()->setText(getway);

   QString str_Tmpdns;
   QString str_Tmpdhcp;
   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   if (settings)
   {
      str_Tmpdns = settings->value("network/dns").toString();
      str_Tmpdhcp = settings->value("network/dhcp").toString();
      if ((str_Tmpdhcp == "x") && (str_Tmpdns != NULL) )
      {
         QString cmd_tmp = "echo nameserver %1 > /etc/resolv.conf";
         cmd_tmp = cmd_tmp.arg(str_Tmpdns);
         QByteArray para = cmd_tmp.toLatin1();
         system(para.data());
      }
   }

   readDns(dns);

   netset_dnslable = new QLabel;
   netset_dnslable->setFixedSize(30, 14);
   netset_dnslable->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   netset_dnslable->setText(QStringLiteral("DNS:"));
   dnsedit = new CMyTextEdit(":/setsvrurl/input", "", 150, 30, 1);
   dnsedit->GetEdit()->setText(dns);
   QGridLayout* c_layout = new QGridLayout;

   c_layout->addWidget(netset_ipaddr, 0, 0, Qt::AlignRight);
   c_layout->addWidget(ipaddredit, 0, 1);
   c_layout->addWidget(netset_mask, 1, 0, Qt::AlignRight);
   c_layout->addWidget(maskedit, 1, 1);
   c_layout->addWidget(netset_gateway, 2, 0, Qt::AlignRight);
   c_layout->addWidget(gatewayedit, 2, 1);
   c_layout->addWidget(netset_dnslable, 3, 0, Qt::AlignRight);
   c_layout->addWidget(dnsedit, 3, 1);
   centertop->setLayout(c_layout);

   moresetBtn = new QPushButton;
   connect(moresetBtn, SIGNAL(clicked()), this, SLOT(moresetclickfunc()));
   moresetBtn->setFixedSize(14, 14);
   moresetBtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
   moresetBtn->setIcon(QIcon(QString(":/net_about/unselect")));
   moresetBtn->setIconSize(QSize(14, 14));
   QLabel* moresetLabel = new QLabel;
   moresetLabel->setFixedSize(50, 14);
   moresetLabel->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   moresetLabel->setText(QStringLiteral(" 高级"));
   isMoresetSelect = false;

   QHBoxLayout* h_midlayout = new QHBoxLayout;
   h_midlayout->addSpacing(60);
   h_midlayout->addWidget(moresetBtn);
   h_midlayout->addWidget(moresetLabel);
   h_midlayout->addStretch();

   QWidget* mid = new QWidget;
   mid->setFixedSize(250, 30);
   mid->setLayout(h_midlayout);

   QWidget* centerbottom = new QWidget;
   centerbottom->setFixedSize(250, 130);
   svrurl = new QLabel;
   svrurl->setFixedSize(65, 30);
   svrurl->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   svrurl->setText(QStringLiteral("服务器地址:"));
   svrurl->hide();

   QString strsvrurl;
   QString isdhcp;
   if (settings)
   {
      strsvrurl = settings->value("server/url").toString();
      isdhcp = settings->value("network/dhcp").toString();
      delete settings;
      settings = NULL;
   }

   if (QString("x") == isdhcp)
   {
      usersetbtn->setIcon(QIcon(QString(":/net_about/select")));
      dhcpbutton->setIcon(QIcon(QString(":/net_about/unselect")));
      ipaddredit->GetEdit()->setReadOnly(false);
      maskedit->GetEdit()->setReadOnly(false);
      gatewayedit->GetEdit()->setReadOnly(false);
      //printf("111111111111111111111111111111\n");
      #ifndef OS_X86
          QProcess::startDetached(QString("/home/network"), QStringList());
      #endif

      QString ip,mac;
      QString netmask;
      getmaskAddress(ip, netmask,mac);
      char buff[256], ifName[12];
      bzero(buff, sizeof(buff));
      get_gateway(buff, ifName);
      QString getway(buff);
      ipaddredit->GetEdit()->setText(ip);
      maskedit->GetEdit()->setText(netmask);
      gatewayedit->GetEdit()->setText(getway);
   }

   QStringList list;
   QString strip;
   QString strport;
   if (!strsvrurl.isEmpty())
   {
      strsvrurl.replace("//", "");
      strsvrurl.replace("/", ":");
      list = strsvrurl.split(":");
      strip = list[1];
      strport = list[2];
   }


   uuuurledit = new CMyTextEdit(":/setsvrurl/input", QStringLiteral("服务器地址:"), 150, 30);
   uuuurledit->GetEdit()->setText(strip);
   uuuurledit->hide();
   svrport = new QLabel;
   svrport->setFixedSize(30, 30);
   svrport->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   svrport->setText(QStringLiteral("端口:"));
   svrport->hide();
   pppportedit = new CMyTextEdit(":/setsvrurl/input", QStringLiteral("端口:"), 150, 30);
   pppportedit->GetEdit()->setText(strport);
   pppportedit->hide();
   QGridLayout* cblayout = new QGridLayout;
   cblayout->addWidget(svrurl, 0, 0, Qt::AlignRight);
   cblayout->addWidget(uuuurledit, 0, 1);
   cblayout->addWidget(svrport, 1, 0, Qt::AlignRight);
   cblayout->addWidget(pppportedit, 1, 1);

   retbtn = new QPushButton;
   retbtn->setFixedSize(70, 30);
   retbtn->setObjectName("retbtn");
   retbtn->setStyleSheet("QPushButton#retbtn{border-image: url(:/net_about/retnomal);}"
         "QPushButton#retbtn:hover{border-image: url(:/net_about/rethover);}"
         "QPushButton#retbtn:pressed{border-image: url(:/net_about/retclick);}");


   connect(retbtn, SIGNAL(clicked()), this, SLOT(changesetret()));

   savebtn = new QPushButton;
   savebtn->setFixedSize(70, 30);
   savebtn->setObjectName("savebtn");
   savebtn->setStyleSheet("QPushButton#savebtn{border-image: url(:/setsvrurl/nomal);}"
         "QPushButton#savebtn:hover{border-image: url(:/setsvrurl/hover);}"
         "QPushButton#savebtn:pressed{border-image: url(:/net_about/sureclick);}");


   connect(savebtn, SIGNAL(clicked()), this, SLOT(changesetsave()));

   QHBoxLayout* h_bottonlayout = new QHBoxLayout;
   h_bottonlayout->setAlignment(Qt::AlignCenter);
   h_bottonlayout->addSpacing(82);
   h_bottonlayout->addWidget(retbtn);
   h_bottonlayout->addSpacing(5);
   h_bottonlayout->addWidget(savebtn);

   QVBoxLayout* v_bottonalllaout = new QVBoxLayout;
   v_bottonalllaout->setAlignment(Qt::AlignTop);
   v_bottonalllaout->addLayout(cblayout);
   v_bottonalllaout->addLayout(h_bottonlayout);
   centerbottom->setLayout(v_bottonalllaout);

   netmainl->setAlignment(Qt::AlignTop);
   netmainl->setSpacing(0);
   netmainl->addWidget(top);
   netmainl->addWidget(centertop);
   netmainl->addWidget(mid);
   netmainl->addWidget(centerbottom);
   //	netmainl->addStretch();
   //	netmainl->addLayout(h_bottonlayout);
   netwidget->setLayout(netmainl);
}

void CMainWindow::netsetreadonly(bool isreadonly)
{
   if(isreadonly)
   {
      ipaddredit->setReadOnly(true);
      maskedit->setReadOnly(true);
      gatewayedit->setReadOnly(true);
      dnsedit->setReadOnly(true);
      netset_ipaddr->setStyleSheet("background-color:transparent;color:rgb(102,108,112);font-size:12px;border:0px;");
      netset_mask->setStyleSheet("background-color:transparent;color:rgb(102,108,112);font-size:12px;border:0px;");
      netset_gateway->setStyleSheet("background-color:transparent;color:rgb(102,108,112);font-size:12px;border:0px;");
      netset_dnslable->setStyleSheet("background-color:transparent;color:rgb(102,108,112);font-size:12px;border:0px;");
   }
   else
   {
      ipaddredit->setReadOnly(false);
      maskedit->setReadOnly(false);
      gatewayedit->setReadOnly(false);
      dnsedit->setReadOnly(false);
      netset_ipaddr->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
      netset_mask->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
      netset_gateway->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
      netset_dnslable->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   }
}


void CMainWindow::readDns(QString &dns)
{
   QFile file("/etc/resolv.conf");
   if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
   {
      //qDebug() << "open /etc/resolv.conf failed!" << endl;
      return;
   }

   while (!file.atEnd()) {
      QByteArray line = file.readLine();
      QString str_tmp = QString(line.data());
      if (str_tmp.contains('#'))
      {
         continue;
      }
      else
      {
         str_tmp.replace("nameserver ", "");
         str_tmp.replace("\n", "");
         dns = str_tmp;
         break;
      }
   }
}

void CMainWindow::readMac(QString &mac)
{
   system("ifconfig eth0 > /home/mac.txt");
   QFile file("/home/mac.txt");
   if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
   {
      return;
   }
   while (!file.atEnd()) 
   {
      QByteArray line = file.readLine();
      QString str = QString(line.data());
      if(!str.contains(QString("HWaddr")) && !str.contains(QString("硬件地址")))
      {
         continue;
      }
      else
      {
         mac = str;
         break;
      }

   }
   qDebug() << "mac is :" << mac << endl;
   QStringList list1 = mac.split(" ");
   for(int i = 0; i < list1.size(); i ++)
   {
      if(list1[i] == "HWaddr" || list1[i] == "硬件地址")
      {
         mac = list1[i+1];
         qDebug() << "last mac is:" << mac << endl;
         break;
      }
   }
}


void CMainWindow::moresetclickfunc()
{
   isMoresetSelect = !isMoresetSelect;
   if (isMoresetSelect)
   {
      moresetBtn->setIcon(QIcon(QString(":/net_about/select")));
      svrurl->show();
      uuuurledit->show();
      svrport->show();
      pppportedit->show();
   }
   else
   {
      moresetBtn->setIcon(QIcon(QString(":/net_about/unselect")));
      svrurl->hide();
      uuuurledit->hide();
      svrport->hide();
      pppportedit->hide();

   }  //LXL add for DNS
}

void CMainWindow::createAboutWidget()
{
   aboutwidget = new QWidget;
   aboutwidget->setFixedSize(400, 300);

   QLabel* picl = new QLabel;
   picl->setFixedSize(126, 106);
   picl->setPixmap(QPixmap(QString(":/net_about/pic")));

   QLabel* hardwareverson = new QLabel;
   hardwareverson->setFixedSize(150, 30);
   hardwareverson->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   hardwareverson->setText(QStringLiteral("固件版本: FW1.0.1"));

   QLabel* softwareversion = new QLabel;
   softwareversion->setFixedSize(150, 30);
   softwareversion->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   softwareversion->setText(QStringLiteral("软件版本: ") + custom_type + "_" +main_version);



   QLabel* os = new QLabel;
   os->setFixedSize(300, 30);
   os->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   os->setText(QStringLiteral("Host Require: ") + os_version + "_" + cpu_architecture+ "_" + hardware_company);
   
   QLabel* uuidd = new QLabel;
   uuidd->setFixedSize(300, 30);
   uuidd->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   uuidd->setText(QStringLiteral("UUID: 0552D49838DD11DD90146B8956D89593"));


   QLabel* mac = new QLabel;
   mac->setFixedSize(300, 30);
   mac->setStyleSheet("background-color:transparent;color:rgb(228,228,228);font-size:12px;border:0px;");
   QString ip,netmask,strmac;
   getmaskAddress(ip, netmask,strmac);
   readMac(strmac);
   mac->setText(QStringLiteral("MAC: ") + strmac);
   m_mac = mac;

   QHBoxLayout* hpiclayout = new QHBoxLayout;
   hpiclayout->setAlignment(Qt::AlignCenter);
   hpiclayout->addStretch(30);	
   hpiclayout->addWidget(picl);
   hpiclayout->addStretch(70);


   QHBoxLayout* hversionlayout = new QHBoxLayout;
   hversionlayout->setAlignment(Qt::AlignCenter);
   hversionlayout->addWidget(hardwareverson);
   hversionlayout->addWidget(softwareversion);
   
   QHBoxLayout* hoslayout = new QHBoxLayout;
   hoslayout->setAlignment(Qt::AlignCenter);
   hoslayout->addWidget(os);

   QHBoxLayout* uuidlayout = new QHBoxLayout;
   uuidlayout->setAlignment(Qt::AlignCenter);
   uuidlayout->addWidget(uuidd);


   QHBoxLayout* maclayout = new QHBoxLayout;
   maclayout->setAlignment(Qt::AlignCenter);
   maclayout->addWidget(mac);

   QVBoxLayout* main_layout = new QVBoxLayout;
   main_layout->setContentsMargins(20, 50, 0, 0);
   main_layout->setAlignment(Qt::AlignLeft);
   main_layout->addLayout(hpiclayout);
   main_layout->addLayout(hversionlayout);
   main_layout->addLayout(hoslayout);
   main_layout->addLayout(uuidlayout);
   main_layout->addLayout(maclayout);

   aboutwidget->setLayout(main_layout);

}

void CMainWindow::createSysSettingWidget()
{
   syssetwidget = new fblSetWidget();
}

void CMainWindow::changeToPrewidget()
{
   hpiclayout->setCurrentIndex(0);
   fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/800X600-down);}");
   netrightglayout->setCurrentIndex(0);
   spilter->moveSlider(0);
   for (size_t i = 1; i < fblBtn_Vec.length(); i++)
   {
      if (!(fblBtn_Vec[i]->isHidden()))
      {
         fblBtn_Vec[i]->hide();
      }	
   }
   if (prewdiget)
   {
      changeWidgetTo(prewdiget);
      prewdiget = NULL;
   }
}

void CMainWindow::saveFbl()
{
   QString str_Tmp = "echo %1 >>  /tmp/resolution_fifo";
   switch (fblIndex)
   {
      case 1:
         str_Tmp = str_Tmp.arg(17);
         msgbox = new ZCMessageBox(QStringLiteral("分辨率800X600,即将重启"), this, NULL);
         break;
      case 2:
         str_Tmp = str_Tmp.arg(31);
         msgbox = new ZCMessageBox(QStringLiteral("分辨率1024X768,即将重启"), this, NULL);
         break;
      case 3:
         str_Tmp = str_Tmp.arg(4);
         msgbox = new ZCMessageBox(QStringLiteral("分辨率1280X720,即将重启"), this, NULL);
         break;
      case 4:
         str_Tmp = str_Tmp.arg(2);
         msgbox = new ZCMessageBox(QStringLiteral("分辨率1366X768,即将重启"), this, NULL);
         break;
      case 5:
         str_Tmp = str_Tmp.arg(16);
         msgbox = new ZCMessageBox(QStringLiteral("分辨率1920X1080,即将重启"), this, NULL);
         break;
      default:
         break;
   }
   QByteArray para = str_Tmp.toLatin1();
   system("rm /tmp/resolution_fifo");
   system(para.data());

   msgbox->show();
#ifdef __linux
   sleep(2);
#else
   Sleep(2000);
#endif
   system("sh /home/ResetIo.sh");
}

void CMainWindow::fblBtn1Clicked()
{
   QPushButton * btn_tmp = (QPushButton *)sender();
   if (btn_tmp == fblBtn_Vec[1])
   {
      hpiclayout->setCurrentIndex(0);
      fblIndex = 1;
      fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/800X600-choose);}");
   }
   else
   {
      if (isfblBtnShow)
      {
         for (int i = 1; i < fblBtn_Vec.size(); i++)
         {
            fblBtn_Vec[i]->hide();
         }
      }
      else
      {
         for (int i = 1; i < fblBtn_Vec.size(); i++)
         {
            fblBtn_Vec[i]->show();
         }
      }
      isfblBtnShow = !isfblBtnShow;
   }
}

void CMainWindow::fblBtn2Clicked()
{
   fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1024X768-choose);}");
   fblIndex = 2;
   hpiclayout->setCurrentIndex(1);
}

void CMainWindow::fblBtn3Clicked()
{
   fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1280X720-choose);}");
   fblIndex = 3;
   hpiclayout->setCurrentIndex(2);
}

void CMainWindow::fblBtn4Clicked()
{
   fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1336X768-choose);}");
   fblIndex = 4;
   hpiclayout->setCurrentIndex(3);
}

void CMainWindow::fblBtn5Clicked()
{
   fblBtn->setStyleSheet("QPushButton#fblBtn{border-image: url(:/sysset_res/1920X1080-choose);}");
   fblIndex = 5;
   hpiclayout->setCurrentIndex(4);
}

void CMainWindow::netlclickfunc()
{
   netrightglayout->setCurrentIndex(0);
   spilter->moveSlider(0);
}


void CMainWindow::netdiaglclickfunc()
{
   netrightglayout->setCurrentIndex(1);
   spilter->moveSlider(1);
}

void CMainWindow::aboutlclickfunc()
{
   netrightglayout->setCurrentIndex(3);
   spilter->moveSlider(3);
}
/*******by xzg********/
void CMainWindow::usbsetlclickfunc()
{
   netrightglayout->setCurrentIndex(4);
   spilter->moveSlider(4);
}
/*******by xzg********/

void CMainWindow::changesetting()
{
   QString ip;
   QString netmask;
   QString getway;

   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   QString isdhcp;
   if (settings)
   {
      isdhcp = settings->value("network/dhcp").toString();
      ip = settings->value("network/ip").toString();
      netmask = settings->value("network/netmask").toString();
      getway = settings->value("network/gateway").toString();
      delete settings;
      settings = NULL;
   }
   if (QString("x") == isdhcp)
   {
      usersetbtn->setIcon(QIcon(QString(":/net_about/select")));
      dhcpbutton->setIcon(QIcon(QString(":/net_about/unselect")));
#ifndef OS_X86	
      QProcess::startDetached(QString("/home/network"), QStringList());
      system("/etc/init.d/network-manager stop"); 
#endif
      netsetreadonly(false);
   }
   else/* if (QString("y") == isdhcp)*/
   {
      usersetbtn->setIcon(QIcon(QString(":/net_about/unselect")));
      dhcpbutton->setIcon(QIcon(QString(":/net_about/select")));
      netsetreadonly(true);
      QString mac;	
      getmaskAddress(ip, netmask,mac);
      char buff[256], ifName[12];
      bzero(buff, sizeof(buff));
      get_gateway(buff, ifName);
      getway = QString(buff);

   }

   isMoresetSelect = false;
   moresetBtn->setIcon(QIcon(QString(":/net_about/unselect")));
   svrurl->hide();
   uuuurledit->hide();
   svrport->hide();
   pppportedit->hide(); 	

   ipaddredit->GetEdit()->setText(ip);
   maskedit->GetEdit()->setText(netmask);
   gatewayedit->GetEdit()->setText(getway);
   QString dns;
   readDns(dns);
   dnsedit->GetEdit()->setText(dns);
   changeWidgetTo(about_netsetwidget);
}

void CMainWindow::changeuser()
{
#ifdef ZS
   showCenterWidget();//by xzg
#endif
   if (viewer)
   {
      menu->delBtn();
      delete viewer; // $$$  xzg usb set
      //viewer->hide();//xzg usb set
      viewer = NULL;//xzg usb set
      menu = NULL;//xzg usb set
      isviewshow = 0;
   }

   int i = GetUsers();
   if (i > 3)
   {
      setChangeUserPic(morethan3Widget);
      changeWidgetTo(morethan3Widget);
      return;
   }
   switch (i)
   {
      case 0:
         {
            changeWidgetTo(User0Widget);
            break;
         }
      case 1:
         {
            setChangeUserPic(User1Widget);
            changeWidgetTo(User1Widget);
            break;
         }
      case 2:
         {
            setChangeUserPic(User2Widget);
            changeWidgetTo(User2Widget);
            break;
         }
      case 3:
         {
            setChangeUserPic(User3Widget);
            changeWidgetTo(User3Widget);
            break;
         }
      default:
         break;
   }
}
void CMainWindow::changeWidgetTo(QWidget* witch)
{
   prewdiget = mainLayout->currentWidget();
   mainLayout->setCurrentWidget(witch);
   someItemOnWidget(mainLayout->currentWidget());
}

void CMainWindow::changesetret()
{
   changesetretbtn->hide();
   netrightglayout->setCurrentIndex(0);
   spilter->moveSlider(0);
   hasonceclick = false;

   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   QString isdhcp;
   QString ip, mask, gateway, dns;
   if (settings)
   {
      isdhcp = settings->value("network/dhcp").toString();
      ip = settings->value("network/ip").toString();
      mask = settings->value("network/netmask").toString();
      gateway = settings->value("network/gateway").toString();
      dns = settings->value("network/dns").toString();
   }
   if (QString("x") == isdhcp)
   {
      ipaddredit->GetEdit()->setText(ip);
      maskedit->GetEdit()->setText(mask);
      gatewayedit->GetEdit()->setText(gateway);
      dnsedit->GetEdit()->setText(dns);
      if (dns != NULL)
      {
         QString cmd_tmp = "echo nameserver %1 > /etc/resolv.conf";
         cmd_tmp = cmd_tmp.arg(dns);
         QByteArray para = cmd_tmp.toLatin1();
         system(para.data());
      }
      usersetbtn->setIcon(QIcon(QString(":/net_about/select")));
      dhcpbutton->setIcon(QIcon(QString(":/net_about/unselect")));
      ipaddredit->GetEdit()->setReadOnly(false);
      maskedit->GetEdit()->setReadOnly(false);
      gatewayedit->GetEdit()->setReadOnly(false);
       #ifndef OS_X86
       QProcess::startDetached(QString("/home/network"), QStringList());
	#endif
   }

   if (settings)
   {
      delete settings;
      settings = NULL;
   }
   syssetwidget->initView();
   if (prewdiget)
   {
      changeWidgetTo(prewdiget);
      prewdiget = NULL;
   }
}

void CMainWindow::changepwd()
{
   changeWidgetTo(changePswdWidget);
}

void CMainWindow::changepwdret()
{
   if (prewdiget)
   {
      changeWidgetTo(prewdiget);
      prewdiget = NULL;
   }
}

void CMainWindow::checkurl()
{
   QString str("http://172.16.2.7:8080/CloudDeskTopMS/interface/connect");

   str = urledit->GetEdit()->text();
   m_cmd = Auth;
   doHttpGet(m_cmd, str);
}

void CMainWindow::clicklogin()
{
   /*/
   //test
   //printf("aaaaaaaaaaaaaaa\n");
   char* p = NULL;
   memcpy(p,"111",3);
   delete p;
   printf("bbbbbbbbbbbbb\n");	
   //test
   //*/
   int whichwidget = 1;
   //http://172.16.2.7:8080/CloudDeskTopMS/interface/login?user_name=username&password=password
   QString pwd;
   if (mainLayout->currentWidget() == firstLoginWidget)
   {
      whichwidget = 1;
      uname = userTextEdit->GetEdit()->text();
      pwd = passwordTextEdit->GetEdit()->text();
      upic = photoLabel->GetPixPath();
      if ((uname == NULL) || (pwd == NULL))
      {

         if (NULL != msgbox)
         {
            delete msgbox;
            msgbox = NULL;
         }
         msgbox = new ZCMessageBox(QStringLiteral("用户名或密码不能为空！"), this, NULL);
         msgbox->exec();
         islogining = 0;
         return;
      }
   }
   if (mainLayout->currentWidget() == secondLoginWidget)
   {
      whichwidget = 2;
      pwd = edit->inputEdit->text();
      upic = secondwuserpic->GetPixPath();
      if (NULL == pwd)
      {
         if (NULL != msgbox)
         {
            delete msgbox;
            msgbox = NULL;
         }
         msgbox = new ZCMessageBox(QStringLiteral("密码不能为空！"), this, NULL);
         msgbox->show();
         islogining = 0;
         return;
      }
   }


   if (!islogining)
   {
      islogining = 1;

      if (whichwidget == 1)
      {
         firstLoginBtn->setStyleSheet("QPushButton#firstLoginBtn{border-image: url(:/first_login_res/cnomal);}"
               "QPushButton#firstLoginBtn:hover{border-image: url(:/first_login_res/cmove);}"
               "QPushButton#firstLoginBtn:pressed{border-image: url(:/first_login_res/cclick);}");

         floading->loadingshow();
      }
      if (whichwidget == 2)
      {
         secondLoginBtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
         secondLoginBtn->setStyleSheet("QPushButton#secondLoginBtn{border-image: url(:/second_logres/cnomal);}"
               "QPushButton#secondLoginBtn:hover{border-image: url(:/second_logres/cnomal);}"
               "QPushButton#secondLoginBtn:pressed{border-image: url(:/second_logres/cclick);}");

         sloading->loadingshow();
      }



      QString needmd5 = uname + pwd;

      QString md5;
      QByteArray ba, bb;
      QCryptographicHash md(QCryptographicHash::Md5);
      ba.append(needmd5);
      md.addData(ba);
      bb = md.result();
      md5.append(bb.toHex());
      passwd = md5;


      QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
      QString svrurl,softweare_ver;
      if (settings)
      {
         svrurl = settings->value("server/url").toString();
         softweare_ver = settings->value("version/ver").toString();
      }

      QString ip, mac, subnet_mask, gateway,hardware_version("0.0.0.1"),time("0");
      QString lasttime = "0";
      QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
      if (msgfile)
      {

         lasttime = msgfile->value("messages/lasttime").toString();
      }
      time = lasttime;

     // dhcpmode(ip,subnet_mask,gateway,mac);

      //QString strfmt("login?user_name=%1&password=%2&ip=%3&mac=%4&subnet_mask=%5&gateway=%6&software_version=%7&hardware_version=%8&time=%9");//++++new				
      //QString url = strfmt.arg(uname).arg(md5).arg(ip).arg(mac).arg(subnet_mask).arg(gateway).arg(softweare_ver).arg(hardware_version).arg(time);

      QString strfmt("login?user_name=%1&password=%2");
      QString url = strfmt.arg(uname).arg(md5);
      svrurl.replace("connect", url);
      m_cmd = Login;
      doHttpGet(m_cmd, svrurl);

      //qDebug() << "svrurl:" << svrurl;


      //login
   }
   else
   {
      islogining = 0;

      if (whichwidget == 1)
      {
         firstLoginBtn->setStyleSheet("QPushButton#firstLoginBtn{border-image: url(:/first_login_res/btn_pressed);}"
               "QPushButton#firstLoginBtn:hover{border-image: url(:/first_login_res/btn_hover);}"
               "QPushButton#firstLoginBtn:pressed{border-image: url(:/first_login_res/btn_pressed);}");

         floading->loadinghide();
      }
      if (whichwidget == 2)
      {
         secondLoginBtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
         secondLoginBtn->setStyleSheet("QPushButton#secondLoginBtn{border-image: url(:/second_logres/lnomal);}"
               "QPushButton#secondLoginBtn:hover{border-image: url(:/second_logres/lnomal);}"
               "QPushButton#secondLoginBtn:pressed{border-image: url(:/second_logres/lclick);}");

         sloading->loadinghide();
      }


      if (m_reply)
      {
         m_reply->abort();
      }


      if (viewer)
      {
		// viewer->hide();//xzg usb set
         delete viewer;
         viewer = NULL;
      }

   }
}


void CMainWindow::doHttpGet(int cmd, QString strUrl)
{
   QString ip,mac,mask;
   getmaskAddress(ip,mask,mac);
   MyZCLog::Instance().WriteToLog(ZCERROR, QString("doHttpGet strUrl=") + QString("%1,").arg(strUrl) + QString(" ip:") + QString("%1").arg(ip) + QString("mac:%1").arg(mac));

   QNetworkRequest request;
   request.setUrl(QUrl(strUrl));
   request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
   m_reply = m_manager->get(request);

   dohttptimer.setSingleShot(true);
   connect(&dohttptimer, SIGNAL(timeout()), this, SLOT(timeabort()));
   dohttptimer.start(10000);   // 5 secs. timeout
}

void CMainWindow::replyFinished(QNetworkReply* reply)
{
   QString ip,mac,mask;
   getmaskAddress(ip,mask,mac);
   MyZCLog::Instance().WriteToLog(ZCERROR, QString("replyFinished+++++cmd = %1+++++ip=%2,mac=%3").arg(m_cmd).arg(ip).arg(mac));

   this->setCursor(Qt::ArrowCursor);
   dohttptimer.stop();
   if (reply && reply->error() == QNetworkReply::NoError)
   {
      QByteArray data = reply->readAll();
      int len = data.size();
      //qDebug() << tr(data);
      //qDebug() << m_cmd;
      parseResult(m_cmd, data);
   }
   else
   {
      okButtonIsRet = true;
      if (m_cmd == Auth)
      {
         if (msgbox)
         {
            delete msgbox;
            msgbox = NULL;
         }
         msgbox = new ZCMessageBox(QStringLiteral("请检查服务器IP,端口，鉴权失败"), this, NULL);
         msgbox->show();
      }

      if (m_cmd == Login)
      {
         islogining = 0;

         firstLoginBtn->setStyleSheet("QPushButton#firstLoginBtn{border-image: url(:/first_login_res/btn_pressed);}"
               "QPushButton#firstLoginBtn:hover{border-image: url(:/first_login_res/btn_hover);}"
               "QPushButton#firstLoginBtn:pressed{border-image: url(:/first_login_res/btn_pressed);}");

         floading->loadinghide();

         secondLoginBtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
         secondLoginBtn->setStyleSheet("QPushButton#secondLoginBtn{border-image: url(:/second_logres/lnomal);}"
               "QPushButton#secondLoginBtn:hover{border-image: url(:/second_logres/lnomal);}"
               "QPushButton#secondLoginBtn:pressed{border-image: url(:/second_logres/lclick);}");

         sloading->loadinghide();
         QString errstr;
         if(reply->error() == QNetworkReply::UnknownNetworkError || reply->error() == QNetworkReply::OperationCanceledError )
         {
            errstr = QStringLiteral("网路不通,登陆失败");
         }
         else if(reply->error() == QNetworkReply::ConnectionRefusedError)
         {
            errstr = QStringLiteral("web服务器未找到,登陆失败");
         }
         else
         {
            errstr = QStringLiteral("web服务器返回错误,登陆失败");
         }

         if (msgbox)
         {
            delete msgbox;
            msgbox = NULL;
         }
         msgbox = new ZCMessageBox(errstr, this, NULL);
         msgbox->show();

      }

      if (m_cmd == RestartHost)
      {
         if (msgbox)
         {
            delete msgbox;
            msgbox = NULL;
         }
         msgbox = new ZCMessageBox(QStringLiteral("服务器无返回"), this, NULL);
         msgbox->show();
      }

      QString ip,mac,mask;
      getmaskAddress(ip,mask,mac);

      if (m_cmd == UpdateHostInfo)
      {
         isUpgradeCheckd = false;
      }


      MyZCLog::Instance().WriteToLog(ZCERROR, QString("QNetworkReply::Error cmd=") + QString("%1,").arg(m_cmd) + QString(" errorcode=") + QString("%1").arg(reply->error()) + QString("ip:")+ QString("%1").arg(ip)+QString("mac:")+QString("%1").arg(mac));
   }
   reply->close();
}

bool CMainWindow::parseResult(ZcCommandType type, QByteArray data)
{
   bool ret = false;
   switch (type)
   {
      case Auth:
         {
            ret = parseConnectRet(data);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("auth data =%1").arg(data.data()));
            if (ret)
            {
               QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
               if (settings)
               {
                  if (mainLayout->currentWidget() == setsvrurlWidget)
                     settings->setValue("server/url",urledit->GetEdit()->text());
               }

               if (mainLayout->currentWidget() == about_netsetwidget)
               {
                  if (settings)
                  {
                     settings->setValue("server/url", newwsvrurl);
                  }

                  if (msgbox)
                  {
                     delete msgbox;
                     msgbox = NULL;
                  }
                  if (!ipaddredit->GetEdit()->isReadOnly())
                  {
                     msgbox = new ZCMessageBox(QStringLiteral("配置成功"), this, NULL);
                  }
                  else
                  {
                     msgbox = new ZCMessageBox(QStringLiteral("配置成功"), this, NULL);
                  }
                  if(isKillNetMgr)
                  {
                     system("/etc/init.d/network-manager stop");
                     isKillNetMgr = 0;
                  } 
                  msgbox->exec();
                  isadduser = false;
                  changesetretbtn->hide();
                  changeWidgetTo(prewdiget);
                  prewdiget = NULL;
                  return true;
               }

               changeWidgetTo(firstLoginWidget);
            }
            else
            {
               if (NULL != msgbox)
               {
                  delete msgbox;
                  msgbox = NULL;
               }
               if (!ipaddredit->GetEdit()->isReadOnly())
               {
                  msgbox = new ZCMessageBox(QStringLiteral("配置失败"), this, NULL);
               }
               else
               {
                  msgbox = new ZCMessageBox(QStringLiteral("请检查服务器IP,端口，鉴权失败"), this, NULL);
               }
               msgbox->show();
            }
         }
         break;
      case Login:
         {
            //qDebug() << data;
            ret = parseLoginRet(data);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("Login data =%1").arg(data.data()));
            if (ret)
            {

               if (m_hostinfo.size() != 1)
               {
                  islogining = 0;
                  firstLoginBtn->setStyleSheet("QPushButton#firstLoginBtn{border-image: url(:/first_login_res/btn_pressed);}"
                        "QPushButton#firstLoginBtn:hover{border-image: url(:/first_login_res/btn_hover);}"
                        "QPushButton#firstLoginBtn:pressed{border-image: url(:/first_login_res/btn_pressed);}");
                  floading->loadinghide();
                  secondLoginBtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
                  secondLoginBtn->setStyleSheet("QPushButton#secondLoginBtn{border-image: url(:/second_logres/lnomal);}"
                        "QPushButton#secondLoginBtn:hover{border-image: url(:/second_logres/lnomal);}"
                        "QPushButton#secondLoginBtn:pressed{border-image: url(:/second_logres/lclick);}");
                  sloading->loadinghide();
               }


               if (m_hostinfo.size() < 1)
               {

                  if (NULL != msgbox)
                  {
                     delete msgbox;
                     msgbox = NULL;
                  }
                  msgbox = new ZCMessageBox(QStringLiteral("用户未分配云主机"), this, NULL);
                  msgbox->show();

               }
               else if (m_hostinfo.size() > 1)
               {
                  if(chostselectWdiget)
                  {
                     mainLayout->removeWidget(chostselectWdiget);
                     chostselectWdiget = NULL;
                  }

                  createCloudHostSelectWidget();
                  changeWidgetTo(chostselectWdiget);

               }
               else if (m_hostinfo.size() == 1)
               {
                  map<QString, K_yType>::iterator it = m_hostinfo.begin();
                  if (it != m_hostinfo.end())
                  {
                     m_uuid = (it->second)["uuid"];
                     m_hostisusb = (it->second)["usb"];
                     QString str = (it->second)["running_status"];
                     currenthostname = (it->second)["name"];
                     //                     if ((it->second)["running_status"] = QString("%1").arg(1))
                     if(1)
                     {
                        ismultihost = false;
                        isstarthost = true;
                        //qDebug() << "1111111111222222222222222222333333333333333334444444444444444455555555555555555";
                        menustarthost();
                        okButtonIsRet = true;
                     }
                     else
                     {
                        int ret =   spicelogin((it->second)["ip"], (it->second)["port"], (it->second)["account"], (it->second)["password"]);
                        if(!ret)
                        {
                           okButtonIsRet = true;
                        }
                     }
                  }
               }

               lastsucuname = uname;
               lastsucupwd = passwd;
               savelastusser();
               saveuserpictofile();
            }
            else
            {

               islogining = 0;

               firstLoginBtn->setStyleSheet("QPushButton#firstLoginBtn{border-image: url(:/first_login_res/btn_pressed);}"
                     "QPushButton#firstLoginBtn:hover{border-image: url(:/first_login_res/btn_hover);}"
                     "QPushButton#firstLoginBtn:pressed{border-image: url(:/first_login_res/btn_pressed);}");

               floading->loadinghide();

               secondLoginBtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
               secondLoginBtn->setStyleSheet("QPushButton#secondLoginBtn{border-image: url(:/second_logres/lnomal);}"
                     "QPushButton#secondLoginBtn:hover{border-image: url(:/second_logres/lnomal);}"
                     "QPushButton#secondLoginBtn:pressed{border-image: url(:/second_logres/lclick);}");

               sloading->loadinghide();

               if (NULL != msgbox)
               {
                  delete msgbox;
                  msgbox = NULL;
               }
               if (QString("user ban...") == jsonmsg)
               {
                  msgbox = new ZCMessageBox(QStringLiteral("用户已经被禁用"), this, NULL);
               }
               else
               {
                  msgbox = new ZCMessageBox(QStringLiteral("用户名或密码错误"), this, NULL);
               }
               msgbox->show();

            }
         }
         break;
      case ChangePWD:
         {
            ret = parseLoginRet(data);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("ChangePWD data =%1").arg(data.data()));
            if (ret)
            {
               if (NULL != msgbox)
               {
                  delete msgbox;
                  msgbox = NULL;
               }
               msgbox = new ZCMessageBox(QStringLiteral("修改成功"), this, NULL);
               msgbox->show();
            }
            else
            {
               if (NULL != msgbox)
               {
                  delete msgbox;
                  msgbox = NULL;
               }
               if (QString("not found user info") == jsonmsg)
               {
                  msgbox = new ZCMessageBox(QStringLiteral("原密码错误"), this, NULL);
               }
               else
               {
                  msgbox = new ZCMessageBox(jsonmsg, this, NULL);
               }
               msgbox->show();
            }
            okButtonIsRet = true;
         }
         break;
      case GetHostInfo:
         break;
      case UpdateHostInfo:
         {
            ret =   parseGetVersion(data);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("UpdateHostInfo data =%1").arg(data.data()));
            if (ret)
            {
               QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
               QString svrurl;
               if (settings)
               {
                  svrurl = settings->value("server/url").toString();
               }
               QString fmt = "download?download_id=%1";
               QString str = fmt.arg(download_id);
               svrurl.replace("connect", str);

               if (!viewer)
               {
                  CUpdateWidget* wid = new CUpdateWidget(main_widget);
                  wid->setDownLoadUrl(svrurl,version,update_info);

                  QDesktopWidget* desktopWidget = QApplication::desktop();
                  // 	//获取设备屏幕大小
                  QRect screenRect = desktopWidget->screenGeometry();
                  wid->setGeometry((screenRect.width() - 315) / 2, (screenRect.height() - 257) / 2 - 39,315,257);
                  wid->show();
               }
               else
               {
                  isUpgradeCheckd = false;
               }
            }

         }
         break;
      case StartHost:
         {

            //qDebug() << "StartHost ret!!!!!!!!!!!!!!!!@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@################$$$$$$$$$$$$$$$$$";
            ret = parseOperateRet(data);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("StartHost data =%1").arg(data.data()));
            printf(data);
            ret = 1;
            if (ret)
            {
               if (NULL != msgbox)
               {
                  delete msgbox;
                  msgbox = NULL;
               }
               if (ismultihost)
               {
                  K_yType val = hostlistwd->GetSelectItemInfo();

                  if (spicelogin(val["ip"], val["port"], val["account"], val["password"]))
                  {
                     saveuserpictofile();
                  }
                  else
                  {
                     okButtonIsRet = true;
                  }
               }
               else
               {
                  map<QString, K_yType>::iterator it = m_hostinfo.begin();
                  if (it != m_hostinfo.end())
                  {
                     int ret = spicelogin((it->second)["ip"], (it->second)["port"], (it->second)["account"], (it->second)["password"]);
                     if(!ret)
                     {
                        okButtonIsRet = true;
                     }
                  }
               }
            }
            else
            {

               islogining = 0;
               firstLoginBtn->setStyleSheet("QPushButton#firstLoginBtn{border-image: url(:/first_login_res/btn_pressed);}"
                     "QPushButton#firstLoginBtn:hover{border-image: url(:/first_login_res/btn_hover);}"
                     "QPushButton#firstLoginBtn:pressed{border-image: url(:/first_login_res/btn_pressed);}");
               floading->loadinghide();
               secondLoginBtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
               secondLoginBtn->setStyleSheet("QPushButton#secondLoginBtn{border-image: url(:/second_logres/lnomal);}"
                     "QPushButton#secondLoginBtn:hover{border-image: url(:/second_logres/lnomal);}"
                     "QPushButton#secondLoginBtn:pressed{border-image: url(:/second_logres/lclick);}");
               sloading->loadinghide();

               if(hloading)
               {
                  hloading->loadinghide();
               }
               if(m_okBtn)
               {	
                  m_okBtn->setStyleSheet("QPushButton#okBtn{border-image: url(:/hostselect/surenomal);}"
                        "QPushButton#okBtn:hover{border-image: url(:/hostselect/surehover);}"
                        "QPushButton#okBtn:pressed{border-image: url(:/hostselect/sureclicked);}");
               }



               if (NULL != msgbox)
               {
                  delete msgbox;
                  msgbox = NULL;
               }
               msgbox = new ZCMessageBox(jsonmsg, this, NULL);
               msgbox->show();

            }
            okButtonIsRet = true;

         }
         break;
      case StopHost:
         {
            ret = parseOperateRet(data);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("StopHost data =%1").arg(data.data()));
            if (!ret)
            {
               if (NULL != msgbox)
               {
                  delete msgbox;
                  msgbox = NULL;
               }
               if (QStringLiteral("关机失败") == jsonmsg)
                  msgbox = new ZCMessageBox(jsonmsg, this, NULL);
               else
                  msgbox = new ZCMessageBox(QStringLiteral("关机失败"), this, NULL);
               msgbox->show();
            }
         }
         break;
      case RestartHost:
         {
            ret = parseOperateRet(data);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("RestartHost data =%1").arg(data.data()));
            if (ret)
            {
               if (NULL != msgbox)
               {
                  delete msgbox;
                  msgbox = NULL;
               }
               //msgbox = new ZCMessageBox(QStringLiteral("重启命令发送成功！"), this, NULL);
               //msgbox->show();
            }
            else
            {
               if (NULL != msgbox)
               {
                  delete msgbox;
                  msgbox = NULL;
               }
               msgbox = new ZCMessageBox(jsonmsg, this, NULL);
               msgbox->show();

            }
            okButtonIsRet = true;
         }
         break;
      case ModifyHostConfig:
         break;
      case ForceStopHost:
         break;
      case QueryHostPrice:
         break;
      case QueryLastVersion:
         break;
      case FirstQueryHostPrice:
         break;
      case UpdateHostInfo2:
         break;
      case LogOut:
         //qDebug() << "logout !!! fuckkkkkkkkkkkkkkkkkkkkk";
         break;
      case HeartBeat:
         //qDebug() << "HeartBeat !!! fuckkkkkkkkkkkkkkkkkkkkk";
         ret = parseHeartBRet(data);
         break;
      default:
         break;
   }

   return ret;
}

bool CMainWindow::parseConnectRet(QByteArray data)
{
   int ret = false;
   // 1. 创建 QJsonParseError 对象，用来获取解析结果
   QJsonParseError error;
   // 2. 使用静态函数获取 QJsonDocument 对象
   QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &error);
   // 3. 根据解析结果进行处理
   if (error.error == QJsonParseError::NoError) {
      if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
      {
         if (jsonDocument.isObject())
         {
            // ...
            QVariantMap result = jsonDocument.toVariant().toMap();
            QString status = result["status"].toString();
            QString message = result["message"].toString();

            if (status == "success" && message == "success")
            {
               ret = true;
            }


         }
         else if (jsonDocument.isArray())
         {
            // ...
         }

      }
   }
   else {
      // 检查错误类型
   }
   return ret;

}




bool CMainWindow::parseLoginRet(QByteArray data)
{
   int ret = false;
   m_hostinfo.clear();
   QJsonParseError jsonError;
   QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
   if (jsonError.error == QJsonParseError::NoError)
   {
      if (jsonDoc.isObject())
      {


         QJsonObject jsonObj = jsonDoc.object();
         QStringList str = jsonObj.keys();

         // 			QJsonValue vale = jsonObj.value("message").type();
         // 			vale = jsonObj.value("status").type();
         // 			vale = jsonObj.value("usb").type();
         // 			vale = jsonObj.value("hosts").type();


         QString  status = jsonObj.value("status").toString();
         if (status == "success")
         {
            ret = true;
         }
         else if (status == "fail")
         {
            ret = false;
         }
         else
         {
            ret = false;
            jsonmsg = "status not define";
         }

         int  user_status = jsonObj.value("user_status").toInt();
         if (user_status)
         {
            ret = false;
            jsonmsg = "user ban...";
            return ret;
         }
         else if (!user_status)
         {
            if (ret)
            {
               jsonmsg = "nomal";
            }
         }
         else
         {
            jsonmsg = "user_status not define";
         }

         int usb = jsonObj.value("usb").toInt();
         QString name = jsonObj.value("name").toString();

         QString message = jsonObj.value("message").toString();
         jsonmsg = message;
         QJsonArray connArray = jsonObj.value("hosts").toArray();//hosts
         for (int i = 0; i < connArray.size(); i++)
         {
            QJsonValue arrayValue = connArray.at(i);
            if (arrayValue.isObject())
            {
               QJsonObject connObject = arrayValue.toObject();
               K_yType kval;
               char szbuf[10] = { 0 };
               sprintf(szbuf, "%d", connObject["port"].toInt());
               kval.insert(make_pair(QString("ip"), connObject["ip"].toString()));
               kval.insert(make_pair(QString("port"), szbuf));
               kval.insert(make_pair(QString("account"), connObject["account"].toString()));
               kval.insert(make_pair(QString("password"), connObject["password"].toString()));
               kval.insert(make_pair(QString("host_name"), connObject["host_name"].toString()));
               kval.insert(make_pair(QString("name"), name));
               kval.insert(make_pair(QString("uuid"), connObject["uuid"].toString()));
               int running_status = connObject["running_status"].toInt();
               kval.insert(make_pair(QString("running_status"), QString("%1").arg(running_status)));
               kval.insert(make_pair(QString("usb"), QString("%1").arg(usb)));
               m_hostinfo[connObject["uuid"].toString()] = kval;
            }
         }

         QString strfmt = "%1,%2,%3,%4";//message
         QString msgfmt = "messages/%1";
         QJsonArray msgArray = jsonObj.value("messages").toArray();
         int isize = msgArray.size();
         if (isize > 0)
         {
            ishasmail = 1;//检测有邮件标志，后面有邮件图标提示要用此值.
         }

         for (int i = 0; i < msgArray.size(); i++)
         {
            QJsonValue arrayValue = msgArray.at(i);
            if (arrayValue.isObject())
            {
               QJsonObject connObject = arrayValue.toObject();
               QString strtowrite = strfmt.arg(connObject["title"].toString()).arg(connObject["content"].toString()).arg(connObject["time"].toString()).arg("unread");
               QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
               if (msgfile)
               {
                  if (i == 0)
                  {
                     msgfile->setValue("messages/lasttime", connObject["time"].toString());
                  }
                  QString para1 = msgfmt.arg(connObject["time"].toString());
                  msgfile->setValue(para1, strtowrite);
               }
            }
         }
      }
   }
   return ret;
}

bool CMainWindow::parseOperateRet(QByteArray data)
{
   int ret = false;
   // 1. 创建 QJsonParseError 对象，用来获取解析结果
   QJsonParseError error;
   // 2. 使用静态函数获取 QJsonDocument 对象
   QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &error);
   // 3. 根据解析结果进行处理
   if (error.error == QJsonParseError::NoError) {
      if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
      {
         if (jsonDocument.isObject())
         {
            // ...
            QVariantMap result = jsonDocument.toVariant().toMap();
            QString status = result["status"].toString();
            QString message = result["message"].toString();

            if (status == "success")
            {
               ret = true;
               jsonmsg = "";
            }
            else
            {
               ret = false;
               jsonmsg = message;
            }


         }
         else if (jsonDocument.isArray())
         {
            // ...
         }

      }
   }
   else {
      // 检查错误类型
   }
   return ret;

}

void CMainWindow::logintohost()
{
   if (!islogining)
   {
      islogining = 1;
      m_okBtn->setStyleSheet("QPushButton#okBtn{border-image: url(:/hostselect/cnomal);}"
            "QPushButton#okBtn:hover{border-image: url(:/hostselect/cmove);}"
            "QPushButton#okBtn:pressed{border-image: url(:/hostselect/cclick);}");
      hloading->loadingshow();



      K_yType val = hostlistwd->GetSelectItemInfo();
      m_uuid = val["uuid"];
      m_hostisusb = val["usb"];
      currenthostname = val["name"];
      if (QString("%1").arg(1) == val["running_status"])
      {
         ismultihost = true;
         isstarthost = true;
         menustarthost();
      }
      else
      {
         if (spicelogin(val["ip"], val["port"], val["account"], val["password"]))
         {
            saveuserpictofile();
         }
      }

   }
   else
   {
      islogining = 0;
      m_okBtn->setStyleSheet("QPushButton#okBtn{border-image: url(:/hostselect/surenomal);}"
            "QPushButton#okBtn:hover{border-image: url(:/hostselect/surehover);}"
            "QPushButton#okBtn:pressed{border-image: url(:/hostselect/sureclicked);}");
      hloading->loadinghide();

      if (viewer)
      {
		 //viewer->hide();//xzg usb set
         delete viewer;
         viewer = NULL;
      }

      if (m_reply)
      {
         m_reply->abort();
      }

   }
}

void CMainWindow::hostret()
{

   mainLayout->removeWidget(chostselectWdiget);

   if (prewdiget)
   {
      changeWidgetTo(prewdiget);
      prewdiget = NULL;
   }

}

void CMainWindow::turnleft()
{
   elemtype elem;
   struct dulnode *next, *prior;
   cur = list_priorelem(userlist, cur->data, &elem);
   prior = list_priorelem(userlist, cur->data, &elem);
   next = list_nextelem(userlist, cur->data, &elem);;
   cardarray[0]->setPixmap(*(*(USERCARDINFO*)prior->data).logopath);
   cardarray[1]->setPixmap(*(*(USERCARDINFO*)cur->data).logopath);
   cardarray[2]->setPixmap(*(*(USERCARDINFO*)next->data).logopath);

   cardarray[0]->SetUname(*(*(USERCARDINFO*)prior->data).username,(*(*(USERCARDINFO*)prior->data).nickname));
   cardarray[1]->SetUname(*(*(USERCARDINFO*)cur->data).username,(*(*(USERCARDINFO*)cur->data).nickname));
   cardarray[2]->SetUname(*(*(USERCARDINFO*)next->data).username,(*(*(USERCARDINFO*)next->data).nickname));
}

void CMainWindow::turnright()
{
   elemtype elem;
   struct dulnode *next, *prior;
   cur = list_nextelem(userlist, cur->data, &elem);
   prior = list_priorelem(userlist, cur->data, &elem);
   next = list_nextelem(userlist, cur->data, &elem);
   cardarray[0]->setPixmap(*(*(USERCARDINFO*)prior->data).logopath);
   cardarray[1]->setPixmap(*(*(USERCARDINFO*)cur->data).logopath);
   cardarray[2]->setPixmap(*(*(USERCARDINFO*)next->data).logopath);

   cardarray[0]->SetUname(*(*(USERCARDINFO*)prior->data).username,(*(*(USERCARDINFO*)prior->data).nickname));
   cardarray[1]->SetUname(*(*(USERCARDINFO*)cur->data).username,(*(*(USERCARDINFO*)cur->data).nickname));
   cardarray[2]->SetUname(*(*(USERCARDINFO*)next->data).username,(*(*(USERCARDINFO*)next->data).nickname));
}

void CMainWindow::deletecard(CUserCard* card)
{
   elemtype elem;
   QString needremove;
   QString nickremove;
   struct dulnode *next, *prior;
   if (card == cardarray[1])
   {
      needremove = *(*(USERCARDINFO*)cur->data).username;
      nickremove = *(*(USERCARDINFO*)cur->data).nickname;
      next = list_nextelem(userlist, cur->data, &elem);
      free(cur->data);
      list_delete(userlist, cur->data);
      cur = next;
      next = list_nextelem(userlist, cur->data, &elem);
      cardarray[1]->setPixmap(*(*(USERCARDINFO*)cur->data).logopath);
      cardarray[2]->setPixmap(*(*(USERCARDINFO*)next->data).logopath);

      cardarray[1]->SetUname(*(*(USERCARDINFO*)cur->data).username,(*(*(USERCARDINFO*)cur->data).nickname));
      cardarray[2]->SetUname(*(*(USERCARDINFO*)next->data).username,(*(*(USERCARDINFO*)next->data).nickname));

   }
   if (card == cardarray[0])
   {
      prior = list_priorelem(userlist, cur->data, &elem);
      needremove = *(*(USERCARDINFO*)prior->data).username;
      nickremove = *(*(USERCARDINFO*)prior->data).nickname;
      free(prior->data);
      list_delete(userlist, prior->data);
      prior = list_priorelem(userlist, cur->data, &elem);
      card->setPixmap(*(*(USERCARDINFO*)prior->data).logopath);
      card->SetUname(*(*(USERCARDINFO*)prior->data).username,(*(*(USERCARDINFO*)prior->data).nickname));
   }
   if (card == cardarray[2])
   {
      next = list_nextelem(userlist, cur->data, &elem);
      needremove = *(*(USERCARDINFO*)next->data).username;
      nickremove = *(*(USERCARDINFO*)next->data).nickname;
      free(next->data);
      list_delete(userlist, next->data);
      next = list_nextelem(userlist, cur->data, &elem);
      card->setPixmap(*(*(USERCARDINFO*)next->data).logopath);
      card->SetUname(*(*(USERCARDINFO*)next->data).username,(*(*(USERCARDINFO*)next->data).nickname));
   }
   if (card == cardarray[3] || cardarray[6] == card)
   {
      prior = list_priorelem(userlist, cur->data, &elem);
      needremove = *(*(USERCARDINFO*)prior->data).username;
      nickremove = *(*(USERCARDINFO*)prior->data).nickname;
      free(prior->data);
      list_delete(userlist, prior->data);
   }
   if (card == cardarray[4])
   {
      next = list_nextelem(userlist, cur->data, &elem);
      needremove = *(*(USERCARDINFO*)cur->data).username;
      nickremove = *(*(USERCARDINFO*)cur->data).nickname;
      free(cur->data);
      list_delete(userlist, cur->data);
      cur = next;
   }
   if (card == cardarray[5])
   {
      next = list_nextelem(userlist, cur->data, &elem);
      needremove = *(*(USERCARDINFO*)next->data).username;
      nickremove = *(*(USERCARDINFO*)next->data).nickname;
      free(next->data);
      list_delete(userlist, next->data);
   }
   if (card == cardarray[7])
   {
      prior = list_priorelem(userlist, cur->data, &elem);
      needremove = *(*(USERCARDINFO*)cur->data).username;
      nickremove = *(*(USERCARDINFO*)cur->data).nickname;
      free(cur->data);
      list_delete(userlist, cur->data);
      cur = prior;
   }
   if (card == cardarray[8])
   {
      needremove = *(*(USERCARDINFO*)cur->data).username;
      nickremove = *(*(USERCARDINFO*)cur->data).nickname;
      free(cur->data);
      list_delete(userlist, cur->data);
   }

   int size = list_length(userlist);
   if (size == 3)
   {
      setChangeUserPic(User3Widget);
      changeWidgetTo(User3Widget);
   }
   if (size == 2)
   {
      setChangeUserPic(User2Widget);
      changeWidgetTo(User2Widget);
   }
   if (size == 1)
   {
      setChangeUserPic(User1Widget);
      changeWidgetTo(User1Widget);
   }
   if (size == 0)
   {
      QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
      if (settings)
      {
         settings->remove("lastsucuser/path");
      }

      changeWidgetTo(User0Widget);
   }


   //qDebug() << "nickremove=================================" << nickremove << endl;
   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   if (settings)
   {
      settings->beginReadArray("users");
      settings->remove(needremove);
      settings->endArray();
      settings->beginReadArray("usersname");
      settings->remove(needremove);
      settings->endArray();
   }

}

void CMainWindow::create3UserWdiget()
{
   User3Widget = new QWidget;
   User3Widget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);


   QHBoxLayout* h_layout = new QHBoxLayout;
   h_layout->setAlignment(Qt::AlignCenter);
   h_layout->setContentsMargins(0, 0, 0, 150);
   h_layout->setSpacing(30);

   CUserCard* carduser1 = new CUserCard(1, 0, 1, 0, 0, 1,0,1);
   carduser1->SetMainW(this);
   cardarray[3] = carduser1;

   CUserCard* carduser2 = new CUserCard(1, 0, 1, 0, 0, 1,0,1);
   carduser2->SetMainW(this);
   cardarray[4] = carduser2;

   CUserCard* carduser3 = new CUserCard(1, 0, 1, 0, 0, 1,0,1);
   carduser3->SetMainW(this);
   cardarray[5] = carduser3;

   CUserCard* cardadd = new CUserCard(1, 0, 1, 1, 1, 1,1);
   cardadd->setPixmap((QString(":/changeuser/addusernomal")), 110, 110);
   cardadd->SetMainW(this);

   h_layout->addWidget(carduser1);
   h_layout->addWidget(carduser2);
   h_layout->addWidget(carduser3);
   h_layout->addWidget(cardadd);
   User3Widget->setLayout(h_layout);

   mainLayout->addWidget(User3Widget);
}

void CMainWindow::create2UserWidget()
{
   User2Widget = new QWidget;
   User2Widget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);


   QHBoxLayout* h_layout = new QHBoxLayout;
   h_layout->setAlignment(Qt::AlignCenter);
   h_layout->setContentsMargins(0, 0, 0, 150);
   h_layout->setSpacing(30);

   CUserCard* carduser1 = new CUserCard(1, 0, 1, 0, 0, 1,0,1);
   carduser1->SetMainW(this);
   cardarray[6] = carduser1;

   CUserCard* carduser2 = new CUserCard(1, 0, 1, 0, 0, 1,0,1);
   carduser2->SetMainW(this);
   cardarray[7] = carduser2;

   CUserCard* cardadd = new CUserCard(1, 0, 1, 1, 1, 1,1);
   cardadd->setPixmap((QString(":/changeuser/addusernomal")), 110, 110);
   cardadd->SetMainW(this);

   h_layout->addWidget(carduser1);
   h_layout->addWidget(carduser2);
   h_layout->addWidget(cardadd);
   User2Widget->setLayout(h_layout);

   mainLayout->addWidget(User2Widget);
}

void CMainWindow::create1UserWidget()
{
   User1Widget = new QWidget;
   User1Widget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);


   QHBoxLayout* h_layout = new QHBoxLayout;
   h_layout->setAlignment(Qt::AlignCenter);
   h_layout->setContentsMargins(0, 0, 0, 150);
   h_layout->setSpacing(30);

   CUserCard* carduser = new CUserCard(1, 0, 1, 0, 0, 1,0,1);
   carduser->SetMainW(this);
   cardarray[8] = carduser;
   CUserCard* cardadd = new CUserCard(1, 0, 1, 1, 1, 1,1);
   cardadd->setPixmap((QString(":/changeuser/addusernomal")), 110, 110);
   connect(cardadd, SIGNAL(clicked()), this, SLOT(adduser()));
   cardadd->SetMainW(this);

   h_layout->addWidget(carduser);
   h_layout->addWidget(cardadd);
   User1Widget->setLayout(h_layout);

   mainLayout->addWidget(User1Widget);
}

void CMainWindow::create0userWidget()
{
   User0Widget = new QWidget;
   User0Widget->setFixedSize(CENTER_WIDGET_W, CENTER_WIDGET_H);


   QHBoxLayout* h_layout = new QHBoxLayout;
   h_layout->setAlignment(Qt::AlignCenter);
   h_layout->setContentsMargins(0, 0, 0, 150);
   h_layout->setSpacing(30);

   CUserCard* card = new CUserCard(1, 0, 1, 1, 1, 1,1);
   card->setPixmap((QString(":/changeuser/addusernomal")), 110, 110);
   card->SetMainW(this);

   h_layout->addWidget(card);
   User0Widget->setLayout(h_layout);

   mainLayout->addWidget(User0Widget);

}

int CMainWindow::GetUsers()
{
   int size = 0;
   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   QSettings* settings2 = new QSettings(INIFILE, QSettings::IniFormat);
   if (settings && settings2)
   {
      settings->beginReadArray("users");
      QStringList keys = settings->allKeys();
      size = keys.size();
      if (userlist)
      {
         list_destory(&userlist);
      }
      list_init(&userlist);
      for (int i = 0; i < size; i++)
      {
         QString strVal = settings->value(keys.at(i)).toString();
         QString strNickname = "usersname/" + keys.at(i);
         //qDebug() << "QString strNickname = "<<strNickname << endl;
         QString str_tmp = settings2->value(strNickname).toString();
         //qDebug() << "strNickname ====================="<<str_tmp << endl;
         USERCARDINFO* info = (USERCARDINFO*)malloc(sizeof(USERCARDINFO));
         if (info)
         {
            info->username = new QString(keys.at(i));
            info->logopath = new QString(strVal);
            info->nickname = new QString(str_tmp);
            list_insert(userlist, i + 1, info);

         }

      }
   }
   return  size;;
}

void CMainWindow::setChangeUserPic(QWidget* widget)
{
   if (widget == morethan3Widget)
   {
      cur = userlist->next->next;
      cardarray[0]->setPixmap(*(*(USERCARDINFO*)cur->prior->data).logopath);
      cardarray[0]->SetUname(*(*(USERCARDINFO*)cur->prior->data).username,(*(*(USERCARDINFO*)cur->prior->data).nickname));
      cardarray[1]->setPixmap(*(*(USERCARDINFO*)cur->data).logopath);
      cardarray[1]->SetUname(*(*(USERCARDINFO*)cur->data).username,(*(*(USERCARDINFO*)cur->data).nickname));
      cardarray[2]->setPixmap(*(*(USERCARDINFO*)cur->next->data).logopath);
      cardarray[2]->SetUname(*(*(USERCARDINFO*)cur->next->data).username,(*(*(USERCARDINFO*)cur->next->data).nickname));

   }
   if (widget == User1Widget)
   {
      cur = userlist->next;
      cardarray[8]->setPixmap(*(*(USERCARDINFO*)cur->data).logopath);
      cardarray[8]->SetUname(*(*(USERCARDINFO*)cur->data).username,(*(*(USERCARDINFO*)cur->data).nickname));
   }
   if (widget == User2Widget)
   {
      cur = userlist->next->next;
      cardarray[6]->setPixmap(*(*(USERCARDINFO*)cur->prior->data).logopath);
      cardarray[6]->SetUname(*(*(USERCARDINFO*)cur->prior->data).username,(*(*(USERCARDINFO*)cur->prior->data).nickname));
      cardarray[7]->setPixmap(*(*(USERCARDINFO*)cur->data).logopath);
      cardarray[7]->SetUname(*(*(USERCARDINFO*)cur->data).username,(*(*(USERCARDINFO*)cur->data).nickname));

   }
   if (widget == User3Widget)
   {
      cur = userlist->next->next;
      cardarray[3]->setPixmap(*(*(USERCARDINFO*)cur->prior->data).logopath);
      cardarray[3]->SetUname(*(*(USERCARDINFO*)cur->prior->data).username,(*(*(USERCARDINFO*)cur->prior->data).nickname));
      cardarray[4]->setPixmap(*(*(USERCARDINFO*)cur->data).logopath);
      cardarray[4]->SetUname(*(*(USERCARDINFO*)cur->data).username,(*(*(USERCARDINFO*)cur->data).nickname));
      cardarray[5]->setPixmap(*(*(USERCARDINFO*)cur->next->data).logopath);
      cardarray[5]->SetUname(*(*(USERCARDINFO*)cur->next->data).username,(*(*(USERCARDINFO*)cur->next->data).nickname));

   }
}

void CMainWindow::adduser()
{
   isadduser = 1;
   qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
   QString randpic = QString(":/userpic/p%1").arg((qrand() % 11) + 1);
   photoLabel->setPixmap(randpic);
   userTextEdit->GetEdit()->setText("");
   passwordTextEdit->GetEdit()->setText("");
   changeWidgetTo(firstLoginWidget);
}

void CMainWindow::saveuserpictofile()
{
   QSettings* settings = new QSettings("user.ini", QSettings::IniFormat);
   if (settings)
   {
      QString fmt = "users/%1";
      QString str = fmt.arg(uname);
      QString nicknamefmt = "usersname/" + uname;
      QString strval = settings->value(str).toString();
      QString path;

      if (mainLayout->currentWidget() == firstLoginWidget)
      {
         path = photoLabel->GetPixPath();
      }
      if (mainLayout->currentWidget() == secondLoginWidget)
      {
         path = secondwuserpic->GetPixPath();
      }
      if (mainLayout->currentWidget() == chostselectWdiget)
      {
         path = upic;
      }

      if (strval.isEmpty())
      {
         settings->setValue(str, path);
         settings->setValue(nicknamefmt, currenthostname);
      }
      else
      {
         settings->remove(str);
         settings->setValue(str, path);
         settings->remove(nicknamefmt);
         settings->setValue(nicknamefmt, currenthostname);
      }

   }
}

int CMainWindow::spicelogin(QString ip, QString port, QString acc, QString pwd)
{
   int iret = 0;

   bool ret = SpiceMulViewer::Spice_Init();
   if (ret)
   {
      if (viewer)
      {
			//xzg usb set
         delete viewer;
         viewer = NULL;
      }
      spiceip = ip;
      spiceport = port;
      spicepwd = pwd;	

      if(viewer == NULL)
      	viewer = new SpiceMulViewer(main_widget, currentScreenWidth, currentScreenHeight);
      viewer->setMultVExParent(this);
      viewer->setHostUsb(m_hostisusb);
      viewer->setStartHost(isstarthost);
      char *cip,*cport,*password;
      QByteArray baip = ip.toLatin1();
      cip = baip.data();
      QByteArray baport = port.toLatin1();
      cport = baport.data();
      QByteArray bapwd = pwd.toLatin1();
      password = bapwd.data();
      int iR = viewer->OpenSpice(cip, cport,NULL,password);
      if(iR == true)
      {
         iret = 1;
      }
      MyZCLog::Instance().WriteToLog(ZCERROR, QString("spicelogin ip=%1,port=%2,pwd=%3").arg(ip).arg(port).arg(pwd));


   }
   return iret;
}

void CMainWindow::savelastusser()
{
   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   if (settings)
   {
      QString str = settings->value("lastsucuser/path").toString();
      if (!str.isEmpty())
      {
         settings->remove("lastsucuser/path");
      }
      settings->setValue("lastsucuser/path", lastsucuname);
   }
}

void CMainWindow::changepwdsure()
{
   QString oldpwd = pwdeidt->GetEdit()->text();
   QString newpwd = newpwdeidt->GetEdit()->text();
   QString newpwd2 = surenewpwdeidt->GetEdit()->text();

   if (newpwd != newpwd2)
   {
      msgbox = new ZCMessageBox(QStringLiteral("两次输入密码不一致"), this, NULL);
      msgbox->show();
      return;
   }

   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   QString svrurl;
   if (settings)
   {
      svrurl = settings->value("server/url").toString();
   }


   QString strfmt("changepassword?user_name=%1&password=%2&new_password=%3");

   QString md5old,md5new;
   QByteArray ba, bb;
   QCryptographicHash md(QCryptographicHash::Md5);
   ba.append(uname + oldpwd);
   md.addData(ba);
   bb = md.result();
   md5old.append(bb.toHex());
   ba.clear();
   bb.clear();
   md.reset();
   ba.append(uname + newpwd2);
   md.addData(ba);
   bb = md.result();
   md5new.append(bb.toHex());

   QString url = strfmt.arg(uname).arg(md5old).arg(md5new);
   svrurl.replace("connect", url);
   m_cmd = ChangePWD;
   doHttpGet(ChangePWD, svrurl);

   MyZCLog::Instance().WriteToLog(ZCERROR, QString("ChangePWD url=%1").arg(svrurl));

}

void CMainWindow::menuexit()
{
   if (reconnecting)
   {
      reconnecting = 0;
      QString ip,mac,mask;
      getmaskAddress(ip,mask,mac);

      MyZCLog::Instance().WriteToLog(ZCERROR, QString("menuexit reconnecting, set reconnecting to be 0,ip=%1,mac=%2;").arg(ip).arg(mac));
   }

   netdicwidget->whide();		

   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   QString name = settings->value("lastsucuser/path").toString();
   QString nickname = "usersname/" + name;
   nickname = settings->value(nickname).toString();
   QString path;
   if (settings)
   {
      QString fmt = "users/%1";
      QString tmp = fmt.arg(name);
      path = settings->value(tmp).toString();
   }



   secondwuserpic->setPixmap(path);
   secondwuserpic->setText(nickname);
#ifdef ZS
   showCenterWidget();//by xzg
#endif
   changeWidgetTo(secondLoginWidget);

   if (viewer)
   {
      menu->delBtn();//xzg usb set
      menu = NULL;

      delete viewer; // $$$
      //viewer->hide();
      viewer = NULL;
      isviewshow = 0;
   }

}

void CMainWindow::menureboot()
{
   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   QString svrurl;
   if (settings)
   {
      svrurl = settings->value("server/url").toString();
   }
#ifdef ZS
   center_widget->show();//by xzg
   loginWidget->hide();//by xzg
   showCenterWidget();//by xzg
#endif

   QString strfmt("hostoperator?cloud_host_id=%1&operator_type=%2");
   // 
   QString url = strfmt.arg(m_uuid).arg(3);
   svrurl.replace("connect", url);
   m_cmd = RestartHost;
   doHttpGet(RestartHost, svrurl);
   MyZCLog::Instance().WriteToLog(ZCERROR, QString("menureboot RestartHost url = %1").arg(svrurl));
   

   // 	QString url = strfmt.arg(m_uuid).arg(1);
   // 	svrurl.replace("connect", url);
   // 	m_cmd = StartHost;
   // 	doHttpGet(StartHost, svrurl);
}

void CMainWindow::menushutdown()
{
   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   QString svrurl;
   if (settings)
   {
      svrurl = settings->value("server/url").toString();
   }


   QString strfmt("hostoperator?cloud_host_id=%1&operator_type=%2");
#ifdef ZS
   showCenterWidget();//by xzg
#endif

   QString url = strfmt.arg(m_uuid).arg(2);
   svrurl.replace("connect", url);
   m_cmd = StopHost;
   doHttpGet(StopHost, svrurl);

   MyZCLog::Instance().WriteToLog(ZCERROR, QString("menushutdown StopHost url = %1").arg(svrurl));
}

void CMainWindow::menustarthost()
{
   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   QString svrurl;
   if (settings)
   {
      svrurl = settings->value("server/url").toString();
   }


   QString strfmt("hostoperator?cloud_host_id=%1&operator_type=%2");

   QString url = strfmt.arg(m_uuid).arg(1);
   svrurl.replace("connect", url);
   m_cmd = StartHost;
   //qDebug() << svrurl;
   doHttpGet(StartHost, svrurl);
   //qDebug() << svrurl;

   MyZCLog::Instance().WriteToLog(ZCERROR, QString("menustarthost StartHost url = %1").arg(svrurl));
}

//************************************
// Method:    dologintohost
// FullName:  CMainWindow::dologintohost
// Access:    public 
// Returns:   void
// Qualifier:
// Parameter: K_yType val
//************************************
void CMainWindow::dologintohost(K_yType val)
{
   if (spicelogin(val["ip"], val["port"], val["account"], val["password"]))
   {
      saveuserpictofile();
   }
   else
   {	
      okButtonIsRet = true;
   }
}

void CMainWindow::customEvent(QEvent *event)
{
   int type = event->type();

   switch (type)
   {

      case NET_PINGROUTE:
         {
            QNetDiagoseEvent* ev = (QNetDiagoseEvent*)event;
            bool bl = ev->GetBoolean();
            if (bl)
            {
               itemarr[DHCPSTAT]->setStatus(true);
               box2route->setPixmap(QPixmap(QString(":/netdiagnose/arrow0")));//ping
            }
            else
            {
               itemarr[DHCPSTAT]->setStatus(false);
               box2route->setPixmap(QPixmap(QString(":/netdiagnose/conerror1")));//ping
            }
            return;
         }
      case NET_PINGWWW:
         {
            QNetDiagoseEvent* ev = (QNetDiagoseEvent*)event;
            bool bl = ev->GetBoolean();
            if (bl)
            {
               itemarr[WWWSTAT]->setStatus(true);
               itemarr[CLOUDDESTSTAT]->setStatus(true);
               route2www->setPixmap(QPixmap(QString(":/netdiagnose/arrow0")));//ping
               www2web->setPixmap(QPixmap(QString(":/netdiagnose/arrow2")));
            }
            else
            {
               itemarr[WWWSTAT]->setStatus(false);
               itemarr[CLOUDDESTSTAT]->setStatus(false);
               route2www->setPixmap(QPixmap(QString(":/netdiagnose/conerror1"))); //ping
               www2web->setPixmap(QPixmap(QString(":/netdiagnose/conerror3")));
            }
            return;
         }
      case NET_PINGSERVER:
         {
            QNetDiagoseEvent* ev = (QNetDiagoseEvent*)event;
            bool bl = ev->GetBoolean();
            if (bl)
            {
               itemarr[CLOUDDESTSTAT]->setStatus(true);
               www2svr->setPixmap(QPixmap(QString(":/netdiagnose/arrow1")));//ping
            }
            else
            {
               itemarr[CLOUDDESTSTAT]->setStatus(false);
               www2svr->setPixmap(QPixmap(QString(":/netdiagnose/conerror2")));//ping
            }
            return;
         }
      case NET_NETCARD:
         {
            QNetDiagoseEvent* ev = (QNetDiagoseEvent*)event;
            bool bl = ev->GetBoolean();
            if (bl)
            {
               itemarr[NETCARDSTAT]->setStatus(true);
               itemarr[NETKOUSTAT]->setStatus(true);
            }
            else
            {
               itemarr[NETCARDSTAT]->setStatus(false);
               itemarr[NETKOUSTAT]->setStatus(false);
            }
            return;
         }
      case NET_USEABLE:
         {
            QNetDiagoseEvent* ev = (QNetDiagoseEvent*)event;
            bool bl = ev->GetBoolean();
            if (bl)
            {
               staticon->setPixmap(QPixmap(QString(":/netdiagnose/iconbright")));
               txt->setText(QStringLiteral("网络连接正常,您可以正常使用"));
            }
            else
            {
               staticon->setPixmap(QPixmap(QString(":/netdiagnose/iconbwrong")));
               txt->setText(QStringLiteral("网络连接异常,请检查网络(%1)").arg(errorstr));
            }

            diagnoseload->loadinghide();

            return;
         }
         //netdianose end
      case ORI_DIS_EVENT:
         {

            if (reconnecting)
            {
               reconnecting = 0;
               netdicwidget->whide();
               //qDebug() << "reconnect suc,timer stop,dis widget hide";
               QString ip,mac,mask;
               getmaskAddress(ip,mask,mac);

               MyZCLog::Instance().WriteToLog(ZCERROR, QString("reconnect suc,timer stop,dis widget hide,ip=%1,mac=%2").arg(ip).arg(mac));
               break;

            }


            floading->loadinghide();
            sloading->loadinghide();
            if (isadduser)
            {
               isadduser = 0;
               if (adduserretbtn)
               {
                  adduserretbtn->hide();
               }

            }
            QString ip,mac,mask;
            getmaskAddress(ip,mask,mac);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("ORI_DIS_EVENT viewer show now ip=%1,mac=%2").arg(ip).arg(mac));
            if (!viewer)
            {
               break;
            }
            QDesktopWidget* desktopWidget = QApplication::desktop();
            //获取设备屏幕大小
            QRect screenRect = desktopWidget->screenGeometry();
            viewer->setGeometry(0, 0, screenRect.width(), screenRect.height());
            //viewer->ChangeResolution(screenRect.width(), screenRect.height());
            //	viewer->SetUsbEnable(true);
#ifdef ZS
            menu = new CMenuWidget(this/*viewer*/, this);                       //by xzg
#else
            menu = new CMenuWidget(viewer, this);                       //by xzg
#endif
            menu->setInfo(currenthostname, upic);
            menu->setGeometry((currentScreenWidth - 500) / 2, 0, 500, 52);  //by xzg
#ifdef ZS	
			showLoginWidget();          //by xzg
#else
            menu->show();             //by xzg
#endif
            viewer->setMenu(menu);
#ifndef ZS
            viewer->show();           //by xzg
#endif
            isviewshow = 1;
            event->accept();
            break;
         }
      case SPICE_CHANNEL_ERROR_LINK:
         {
            if (reconnecting)
            {
               //qDebug() << "reconnect channel error break";
               QString ip,mac,mask;
               getmaskAddress(ip,mask,mac);
               MyZCLog::Instance().WriteToLog(ZCERROR, QString("SPICE_CHANNEL_ERROR_LINK  reconnecting = 1 break go on,ip=%1,mac=%2").arg(ip).arg(mac));
               break;
            }
            //qDebug() << "reconnecting is not 1";
            QString ip,mac,mask;
            getmaskAddress(ip,mask,mac);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("SPICE_CHANNEL_ERROR_LINK spice channel error link,ip=%1,mac=%2").arg(ip).arg(mac));
            if (NULL != msgbox)
            {
               delete msgbox;
               msgbox = NULL;
            }
            msgbox = new ZCMessageBox(QStringLiteral("连接失败"), this, NULL);
            msgbox->show();
            event->accept();
            if (NULL != viewer)
            {

               delete viewer; // $$$
               viewer = NULL;
		//	   viewer->hide();//xzg usb set
               isviewshow = 0;
            }
            break;
         }
      case SPICE_CHANNEL_CLOSED:
         {
            QString ip,mac,mask;
            getmaskAddress(ip,mask,mac);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("SPICE_CHANNEL_CLOSED spice channel close link change to secondlogin,ip=%1,mac=%2").arg(ip).arg(mac));
            if (NULL != viewer)
            {
               delete viewer; // $$$
               viewer = NULL;
		//		viewer->hide();//xzg usb set
               isviewshow = 0;
            }
            changeWidgetTo(secondLoginWidget);
            event->accept();
            break;
         }
      case SPICE_CHANNEL_ERROR_AUTH:
         {
            if(reconnecting)
            {
               break;
            }
            if (NULL != viewer)
            {
               delete viewer; // $$$
               viewer = NULL;
		//		viewer->hide();//xzg usb set
               isviewshow = 0;
            }
            QString ip,mac,mask;
            getmaskAddress(ip,mask,mac);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("SPICE_CHANNEL_ERROR_AUTH spice channel error auth acc or pwd error,ip=%1,mac=%2").arg(ip).arg(mac));
            if (NULL != msgbox)
            {
               delete msgbox;
               msgbox = NULL;
            }
            msgbox = new ZCMessageBox(QStringLiteral("认证出错"), this, NULL);
            msgbox->show();
            event->accept();
            break;
         }
      case SPICE_RECONNECT:
         {
            //qDebug() << "SPICE_RECONNECT+++++++++++++++++++++++++++++++++++++++++++++++++++++";	
            QString ip,mac,mask;
            getmaskAddress(ip,mask,mac);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("SPICE_RECONNECT!!!!!!!!++++++++++++++++++++,ip=%1,mac=%2").arg(ip).arg(mac));
            reconnectfunc();
            break;
         }
      case ERROR_IO_SHOW_WIDGET:
         {
            if(!isviewshow)
            {
               QString ip,mac,mask;
               getmaskAddress(ip,mask,mac);
               MyZCLog::Instance().WriteToLog(ZCERROR, QString("case :ERROR_IO_SHOW_WIDGET  but isviewshow=0 viewer not show!!!!!!!!!!!!!!,ip=%1,mac=%2").arg(ip).arg(mac));

               break;
            }
            if(!reconnecting)
            {
               reconnecting = 1;
               netdicwidget->wshow();
               //qDebug() << "disconwidget show +++++++++++++++++++++++++++++++++++++++++++++++";
               QString ip,mac,mask;
               getmaskAddress(ip,mask,mac);
               MyZCLog::Instance().WriteToLog(ZCERROR, QString("ERROR_IO_SHOW_WIDGET  start reconect!!!!!!!!!!!!!!,ip=%1,mac=%2").arg(ip).arg(mac));
               break;
            }
            QString ip,mac,mask;
            getmaskAddress(ip,mask,mac);
            MyZCLog::Instance().WriteToLog(ZCERROR, QString("reconecting not reconnect again!!!!!!!!!!!!!!,ip=%1,mac=%2").arg(ip).arg(mac));
         }

      default:
         event->accept();
         break;
   }
   okButtonIsRet = true;

   //resume login btn and loading

   islogining = 0;
   if (firstLoginBtn)
   {
      firstLoginBtn->setStyleSheet("QPushButton#firstLoginBtn{border-image: url(:/first_login_res/btn_pressed);}"
            "QPushButton#firstLoginBtn:hover{border-image: url(:/first_login_res/btn_hover);}"
            "QPushButton#firstLoginBtn:pressed{border-image: url(:/first_login_res/btn_pressed);}");
   }
   if (floading)
   {
      floading->loadinghide();
   }

   if (secondLoginBtn)
   {
      secondLoginBtn->setStyleSheet("border:2px groove gray;border-radius:6px;padding:2px 4px;");
      secondLoginBtn->setStyleSheet("QPushButton#secondLoginBtn{border-image: url(:/second_logres/lnomal);}"
            "QPushButton#secondLoginBtn:hover{border-image: url(:/second_logres/lnomal);}"
            "QPushButton#secondLoginBtn:pressed{border-image: url(:/second_logres/lclick);}");
   }

   if (sloading)
   {
      sloading->loadinghide();
   }

   if (hloading)
   {
      hloading->loadinghide();
   }
   if (m_okBtn)
   {
      m_okBtn->setStyleSheet("QPushButton#okBtn{border-image: url(:/hostselect/surenomal);}"
            "QPushButton#okBtn:hover{border-image: url(:/hostselect/surehover);}"
            "QPushButton#okBtn:pressed{border-image: url(:/hostselect/sureclicked);}");
   }


}

void CMainWindow::changesetsave()
{
   this->setCursor(Qt::BusyCursor);
   hasonceclick = false;
   retbtn->setEnabled(false);
   savebtn->setEnabled(false);
   //qDebug()<<__func__<<":"<<__LINE__;

   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   QString svrurl;
   QString isdhcp;
   if (settings)
   {
      svrurl = settings->value("server/url").toString();
      isdhcp = settings->value("network/dhcp").toString();
   }
   if (!ipaddredit->GetEdit()->isReadOnly())
   {
      QString ip = ipaddredit->GetEdit()->text();
      QString mask = maskedit->GetEdit()->text();
      QString gateway = gatewayedit->GetEdit()->text();
      QString dns = dnsedit->GetEdit()->text();
 #ifndef OS_X86        
      isKillNetMgr = 1;
 #endif
      usersetmode(ip, mask, gateway);
      if (settings)
      {
         settings->setValue("network/dns", dns);
      }
//dns && ip set
#ifndef OS_X86     
      QString cmd_tmp = QString("echo nameserver %1 > /etc/resolv.conf").arg(dns);
      QByteArray para = cmd_tmp.toLatin1();
      system(para.data());
#else

      //qDebug()<<"[info]"<<"IP:"<<ip<<"Mask:"<<mask<<"Gateway:"<<gateway<<"DNS:"<<dns;
       QStringList qstrSubStringList = ip.split('.');
      QString broadcast = QString("%1.%2.%3.255").arg(qstrSubStringList[0]).arg(qstrSubStringList[1]).arg(qstrSubStringList[2]);
#ifndef XH 
      QString exec_cmdline = QString("seadee-network-config -t static -s --address=%1 --netmask=%2 --gateway=%3 --dns=%4 --broadcast=%5")
                              .arg(ip).arg(mask).arg(gateway).arg(dns).arg(broadcast);
      system(exec_cmdline.toStdString().c_str());
#else
    ConfigNetwork("static",ip,mask,gateway,broadcast,dns);
#endif
#endif   
   }
   /*else if (QString("x") == isdhcp)
   {
      if (settings)
      {
         settings->setValue("network/dhcp", QString("y"));
      }

#ifdef OS_X86
#ifndef XH 
   system("seadee-network-config -t dhcp -s");
#endif
#endif
   }*/
   
    delete settings;
 
   //url
   QString strip = uuuurledit->GetEdit()->text();
   QString strport = pppportedit->GetEdit()->text();
   QString newstr = strip + ":" + strport;

   QStringList list = svrurl.split("/");

   QString fmt = "%1//%2/%3/%4/%5";
   newwsvrurl = fmt.arg(list[0], newstr, list[3], list[4], list[5]);

   m_cmd = Auth;
   doHttpGet(m_cmd, newwsvrurl);  
   {
      QString ip,mask,mac;
      getmaskAddress(ip,mask,mac);
      MyZCLog::Instance().WriteToLog(ZCERROR, QString("changesetsave Auth   newwsvrurl = %1!!!!!!!!!!!!!!ip=%2,mac=%3").arg(newwsvrurl).arg(ip).arg(mac));
   } 
   retbtn->setEnabled(true);
   savebtn->setEnabled(true);
}

bool CMainWindow::CheckNeedUpgrade()
{
   QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
   //QString version = "0.0.0.0";
   QString svrurl;
   //int filesize;
   if (settings)
   {
      svrurl = settings->value("server/url").toString();
      //version = settings->value("version/ver").toString();
      //filesize = settings->value("version/size").toInt();
      delete settings;
   }else{
   	  return false;
   }
   //	m_isAppComplete = checkComplete(filesize);
   //	if (version.isEmpty() || !m_isAppComplete)
   //	{
   //		version = "0.0.0.0";
   //	}

   /*if (version.isEmpty())
   {
      version = "0.0.0.0";
   }*/	
   m_cmd = UpdateHostInfo;
   QString fmt = "version?version_number=%1_%2_%3_%4&platform_type=%5&os_type=%6&hardware_company=%7";
   QString str = fmt.arg(custom_type).arg(main_version).arg(cpu_architecture).arg(hardware_company).arg(cpu_architecture).arg(os_version).arg(hardware_company);
   svrurl.replace("connect", str);
   MyZCLog::Instance().WriteToLog(ZCINFO, svrurl);
   qDebug() << svrurl;
   doHttpGet(m_cmd, svrurl);
   QString ip,mac,mask;
   getmaskAddress(ip,mask,mac);
   MyZCLog::Instance().WriteToLog(ZCERROR, QString("CheckNeedUpgrade   svrurl = %1!!!!!!!!!!!!!!ip=%2,mac=%3").arg(svrurl).arg(ip).arg(mac));
   return true;
}

bool CMainWindow::checkComplete(int & filesize)
{
#ifdef WIN32
   _savePackagePath = "download/";
   _savePackagePath += QString("cloud_teminal.exe");
#else
   _savePackagePath = "/home/cloud_teminal/";
   _savePackagePath += QString("cloud_teminal");
#endif // WIN32

   //	_savePackagePath = "./cloud_teminal";
   if (QFile::exists(_savePackagePath))
   {
      _file = new QFile(_savePackagePath);
      if (!_file->open(QIODevice::ReadOnly))
      {
         delete _file;
         _file = 0;
         return false;
      }
      else
      {
         QByteArray fileArray = _file->readAll();
         int truefileSize = fileArray.size();
         //		qDebug() << "now program size is :" << truefileSize << endl;
         //qDebug() << "right program size is :" << filesize << endl;
         if (filesize == truefileSize)
         {
            return true;
         }
         else
         {
            //QFile::remove(_savePackagePath);
            return false;
         }
      }
   }
   else
   {
      //qDebug() << "cloud_teminal   does    not   exist" << endl;
      return false;
   }
}

bool CMainWindow::parseGetVersion(QByteArray data)
{
   MyZCLog::Instance().WriteToLog(ZCERROR, QString(data));
   int ret = false;
   // 1. 创建 QJsonParseError 对象，用来获取解析结果
   QJsonParseError error;
   // 2. 使用静态函数获取 QJsonDocument 对象
   QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &error);
   // 3. 根据解析结果进行处理
   if (error.error == QJsonParseError::NoError) {
      if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
      {
         if (jsonDocument.isObject())
         {
            // ...
            QVariantMap result = jsonDocument.toVariant().toMap();
            QString status = result["status"].toString();
            QString message = result["message"].toString();
            QString need_update = result["need_update"].toString();
            download_id = result["download_id"].toString();
            update_info = result["update_info"].toString();
            version = result["latest_version_number"].toString();

            if (status == "success" && need_update == "true")
            {
               ret = true;
            }
            if (version.isEmpty())
            {
               ret = false;
            }



         }
         else if (jsonDocument.isArray())
         {
            // ...
            isUpgradeCheckd = false;
         }
         else
         {
            isUpgradeCheckd = false;
         }

      }
   }
   else {
      // 检查错误类型
      isUpgradeCheckd = false;
   }
   return ret;
}



void CMainWindow::timeabort()
{	
   m_reply->abort();
   okButtonIsRet = true;
}

void CMainWindow::netcheckfunc()
{
   int i = get_netlink_status("eth0");
   if (i == 0)
   {
      //disc
      /*	if (viewer)
         {
         netdicwidget->wshow();
         return;
         }*/
      if (mainLayout->currentWidget() == firstLoginWidget ||
            mainLayout->currentWidget() == secondLoginWidget)
      {
         if(!viewer)
         {
            neterror->show();
         }
      }

   }
   if (i == 1)
   {
      if(!ismacright)
      {
         QString str_Mac;
         readMac(str_Mac);
         m_mac->setText(QStringLiteral("MAC: ") + str_Mac);
         ismacright = true;
      }
      //c
      /*(if (viewer)
        {
        if (netdicwidget->getState() == 1 && (reconnecting != 1))//只有这种情况下重连
        {
        netdicwidget->whide();
        viewer->CloseSpice();
        sleep(2);
        char *cip, *cport, *password;
        QByteArray baip = spiceip.toLatin1();
        cip = baip.data();
        QByteArray baport = spiceport.toLatin1();
        cport = baport.data();
        QByteArray bapwd = spicepwd.toLatin1();
        password = bapwd.data();
        viewer->OpenSpice(cip, cport, NULL, password);
        }
        return;
        }*/
      if (mainLayout->currentWidget() == firstLoginWidget ||
            mainLayout->currentWidget() == secondLoginWidget)
      {
         neterror->hide();
      }
      //add by lcx
      if (this->isUpgradeCheckd == false)
      {
         if(!viewer)
         {
            CheckNeedUpgrade();
            this->isUpgradeCheckd = true;
         }
      }
   }

}


void CMainWindow::sysclickfunc()
{
   netrightglayout->setCurrentIndex(2);
   spilter->moveSlider(2);
}


bool CMainWindow::parseHeartBRet(QByteArray data)
{
   int ret = false;
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
         }
         else
         {
            ret = false;
            jsonmsg = "status not define";
         }

         QString strfmt = "%1,%2,%3,%4";//message
         QString msgfmt = "messages/%1";
         QJsonArray msgArray = jsonObj.value("messages").toArray();
         if (msgArray.size() > 0)
         {
            if (g_menu_ptr)
            {
               g_menu_ptr->mail->setStyleSheet("QPushButton#mail{border-image: url(:/menu/msghave);}"
                     "QPushButton#mail:hover{border-image: url(:/menu/msghave);}"
                     "QPushButton#mail:pressed{border-image: url(:/menu/msghaveclick);}");
            }


         }
         for (int i = 0; i < msgArray.size(); i++)
         {
            QJsonValue arrayValue = msgArray.at(i);
            if (arrayValue.isObject())
            {
               QJsonObject connObject = arrayValue.toObject();
               QString strtowrite = strfmt.arg(connObject["title"].toString()).arg(connObject["content"].toString()).arg(connObject["time"].toString()).arg("unread");
               QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
               if (msgfile)
               {
                  if (i == 0)
                  {
                     msgfile->setValue("messages/lasttime", connObject["time"].toString());
                  }
                  QString para1 = msgfmt.arg(connObject["time"].toString());
                  msgfile->setValue(para1, strtowrite);
               }
            }
         }
      }
   }
   return ret;
}

void CMainWindow::adduserret()
{
   //    changeWidgetTo(adduserretwid);
   changeuser();
   adduserretbtn->hide();
   isadduser = 0;
}

void CMainWindow::setResolution(int index)
{
   hpiclayout->setCurrentIndex(index);
}
void CMainWindow::reconnectfunc()
{
   if (reconnecting)//
   {
      //qDebug() << "reconnect ........";
      viewer->CloseSpice();
      //sleep(1);		
      char *cip, *cport, *password;
      QByteArray baip = spiceip.toLatin1();
      cip = baip.data();
      QByteArray baport = spiceport.toLatin1();
      cport = baport.data();
      QByteArray bapwd = spicepwd.toLatin1();
      password = bapwd.data();
      viewer->OpenSpice(cip, cport, NULL, password);

      QString ip,mac,mask;
      getmaskAddress(ip,mask,mac);
      MyZCLog::Instance().WriteToLog(ZCERROR, QString("reconnectfunc OpenSpice ip= %1,port = %2,pwd=%3!!!!!!!!!!!!!!local ip = %4,mac=%5").arg(cip).arg(cport).arg(password).arg(ip).arg(mac));
   }
}
