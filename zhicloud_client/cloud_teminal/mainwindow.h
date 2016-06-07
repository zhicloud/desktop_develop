#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "cusercard.h"
#include "fblsetwidget.h"
#include <QTimer>
#include <QMainWindow>
#include <QWidget>
#include <QStackedLayout>
#include <QListWidget>
#include <QListView>
#include <QSlider>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPixmap>
#include <QPushButton>
#include <QStringList>
#include <QTableView> //by xzg
#include <QStandardItemModel>//by xzg
#include "mytextedit.h"
#include "zc_message_box.h"
#include "log.h"
#include "togglebutton.h"
#include <QScrollArea>
#include "cnetaboutspilter.h"
#include "cleftarrawpwdedit.h"
#include "common.h"
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QByteArray>
#include <map>
#include <vector>
#include <QProcess>
#include "cloading.h"
#include "getway.h"
#include <strings.h>
#include "chostlist.h"
#include "ringlist.h"
#include "SpiceMulViewer.h"
#include "cnetdiswidget.h"
#include "cdiagnoseitem.h"

#ifdef ZS
#include "webview.h"  //by xzg
#endif

#include "monitorusbdevthread.h" //by xzg
#include <QThread>
#ifdef __linux 
#include <unistd.h>
#else
#endif

using namespace std;

#define  INIFILE "./user.ini"
#define  MSGFILE "./msg.ini"

typedef map<QString, QString> K_yType;

class CMenuWidget;
class SpiceMulViewer;
class usbonfig; // by xzg
class UsbDeviceInfo;//by xzg

typedef struct MyStruct
{
   QString* username;
   QString* logopath;
   QString* nickname;
}USERCARDINFO;


#define NET_PINGROUTE QEvent::User+110
#define NET_PINGWWW QEvent::User+111
#define NET_PINGSERVER QEvent::User+112
#define NET_NETCARD QEvent::User+113
#define NET_USEABLE QEvent::User+114



class QNetDiagoseEvent : public QEvent
{
   public:
      QNetDiagoseEvent(Type type)
         :QEvent(type)
      {

      }
      ~QNetDiagoseEvent()
      {

      }
      void SetBoolean(bool torf)
      {
         trueorfalse = torf;
      }
      bool GetBoolean(){ return trueorfalse; }
   private:
      bool  trueorfalse;
};


class CMainWindow : public QMainWindow
{
   Q_OBJECT

   public:
      CMainWindow(QWidget *parent=0);
      ~CMainWindow();

   public:
#ifdef ZS
      void createLoginWidget();//by xzg
#endif
      void createsetsvrurlWidget();
      void createFirstLoginWidget();
      void createSecondLoginWidget();
      //
      void createMoreThan3Widget();
      void create3UserWdiget();
      void create2UserWidget();
      void create1UserWidget();
      void create0userWidget();
      int GetUsers();
      //

      int netcardstat();
      int ping(QString ip);

      void createChangePassWidget();
      void createCloudHostSelectWidget();
      void createChangeUserWidget();
      void createAbout_NetSettingWidget();
   public:

      void netdiagose();
      void dhcpmode(QString& ipaddr, QString& mask, QString& gateway,QString &mac);
      void usersetmode(const QString& ipaddr, const QString& mask, const QString& gateway);

      bool CheckNeedUpgrade();

      void setScrollSheet(QScrollArea* scroll);

      void someItemOnWidget(QWidget* widget);
      void createAboutWidget();
      void createNetSettingWidget();
	  void createUsbSettingWidget();//by xzg
      void createNetDiagnoseWidget();
      void createSysSettingWidget();

      void changeWidgetTo(QWidget* witch);

      void doHttpGet(int cmd, QString strUrl);
      void savelastusser();
      bool parseResult(ZcCommandType type, QByteArray data);
      bool parseConnectRet(QByteArray data);
      bool parseGetVersion(QByteArray data);
      bool parseHeartBRet(QByteArray data);
      bool parseOperateRet(QByteArray data);
      bool parseLoginRet(QByteArray data);
      void deletecard(CUserCard* card);
      void setChangeUserPic(QWidget* widget);
      void adduser();
      void saveuserpictofile();
      int spicelogin(QString ip = "172.16.6.1", QString port = "5926", QString uname = "", QString pwd = "");

      void menuexit();
      void menureboot();
      void menushutdown();
      void menustarthost();
      bool checkComplete(int & filesize);
      void readDns(QString &dns);
      void readMac(QString &mac);
   public:

      enum 
      {
         NETCARDSTAT = 0,
         NETKOUSTAT = 1,
         DHCPSTAT = 2,
         WWWSTAT = 3,
         CLOUDDESTSTAT = 4,
      };

      //netdiagnoseitem
      QLabel* box2route;
      QLabel* route2www;
      QLabel* www2svr;
      QLabel* www2web;
      CDiagnoseItem* itemarr[5];
      QLabel* staticon;
      QLabel* txt;
      QLabel* m_mac;
      QString errorstr;

      CLoading* diagnoseload;

      QString spiceip;	
      QString spiceport;
      QString spicepwd;

      cnetdiswidget* netdicwidget;

      QPushButton* neterror;

      int islogining;
      QString download_id;
      QString update_info;
      QString version;

      CMyTextEdit* ipaddredit;
      CMyTextEdit* maskedit;
      CMyTextEdit* gatewayedit;

      QPushButton* dhcpbutton;
      QPushButton* usersetbtn;
      //LXL add for DNS
      QPushButton* moresetBtn;
      CMyTextEdit* dnsedit;
      bool isMoresetSelect;
      QLabel* svrurl;
      QLabel* svrport;
      //LXL add for DNS
      QNetworkAccessManager* m_manager;
      QNetworkAccessManager m_downloadmgr;
      //QJson::Parser parser;
      QNetworkReply	*_reply;

      QWidget				*morethan3Widget;
      QWidget				*User0Widget;
      QWidget				*User1Widget;
      QWidget				*User2Widget;
      QWidget				*User3Widget;
      QWidget				*secondLoginWidget;
   private:
      int 				isKillNetMgr;
#ifdef ZS
      QWidget				*loginWidget;// by xzg
#endif
      QWidget				*setsvrurlWidget;
      QWidget				*firstLoginWidget;
      QWidget				*changePswdWidget;
      QWidget				*chostselectWdiget;
      QWidget				*changeuserwidget;
      QWidget				*about_netsetwidget;
#ifdef ZS
      myWebView                         *webView; //by xzg
#endif
	  MonitorUsbDevThread   *usbMonitorThread;//by xzg
	  usbonfig 				*m_usbConfig;//by xzg
	  //list<UsbDeviceInfo>          dev_list;//by xzg
      QStackedLayout		*mainLayout;
      QStackedLayout		*hpiclayout;
      fblSetWidget		*syssetwidget;
      QWidget* dddd;
      QVector<QPushButton *> fblBtn_Vec;
      int					currentScreenWidth;
      int					currentScreenHeight;
      bool                ismultihost;
      bool 				m_isAppComplete;
      bool				isfblBtnShow;
      //add by lcx
      bool           isUpgradeCheckd;
      double				widthRatio;
      double				heightRatio;
      QString				_savePackagePath;
      QFile				*_file;
      CMyTextEdit			*userTextEdit;
      CMyTextEdit			*passwordTextEdit;
      QWidget				*main_widget;
      QWidget				*center_widget;
#ifdef ZS
      QWidget				*logo_widget;//by xzg
#endif
      QWidget				*netwidget;
	  QWidget               *usbsetWidget;//by xzg
	  QStandardItemModel    *model;//by xzg
	  QTableView            *tb;//by xzg
      QWidget				*aboutwidget;
      QWidget				*netdiagnosewidget;
      QLabel* netset_ipaddr;
      QLabel* netset_mask;
      QLabel* netset_gateway;
      QLabel* netset_dnslable;
   protected:
      virtual void customEvent(QEvent *event);
      public slots:
         bool eventFilter(QObject *, QEvent *);
	  void dealUsbHotPlugEvent(bool f);// by xzg
      void netdiaglclickfunc();
      void sysclickfunc();
      void netlclickfunc();
      void aboutlclickfunc();
      void usbsetlclickfunc();//by xzg
	  void toLoginFrame();//by xzg
	  void usbSetTrans();//by xzg
      void netdiagnosefunc();
      void changesetting();
      void changeuser();
      void changesetret();
      void changesetsave();
      void changepwd();
      void changepwdret();
      void changepwdsure();
      void netsetreadonly(bool isreadonly);
      void checkurl();
      void clicklogin();
      void logintohost();
      void hostret();
      void turnleft();
      void changeToPrewidget();
      void saveFbl();
      void turnright();
      void replyFinished(QNetworkReply* reply);
      void dologintohost(K_yType val); 
      void dhcpclickfunc();
      void usersetclickfunc();
      void isdhcpclicktimerout();
      void timeabort();	
      void setResolution(int index);
      void adduserret();
      //void maintimerout(); 
      void netcheckfunc();
      void reconnectfunc();
      void fblBtn1Clicked();
      void fblBtn2Clicked();
      void fblBtn3Clicked();
      void fblBtn4Clicked();
      void fblBtn5Clicked();
      void moresetclickfunc(); //LXL add for DNS
#ifdef ZS
	  void showViewer();//by xzg
	  void showLoginWidget();//by xzg
	  void showCenterWidget();//by xzg
	void openWebView();//by xzg
	void setViewerGrabKeyboard();//by xzg
#endif
   public:
      int reconnecting;
      int isadduser;
      int ishasmail;
      QTimer* reconnecttimer;
      QTimer* netchecktimer;

      QTimer dohttptimer;
      QNetworkReply* m_reply;	

      CMyTextEdit* uuuurledit;
      CMyTextEdit* pppportedit;
      bool isadduserretshow;
      QPushButton *adduserretbtn;
      QPushButton *changesetretbtn;
      QPushButton *cs_btn;
      QPushButton *userchangeBtn;
      QPushButton *changsetBtn;
      QStackedLayout* netrightglayout;
      CNetAboutSpilter* spilter;
      QWidget* prewdiget;
      QWidget* adduserretwid;
      CMyTextEdit* urledit;
      ZcCommandType m_cmd;
      QString jsonmsg;
      QString m_url;
      QString m_uuid;
      QString m_hostisusb;
      map<QString, K_yType> m_hostinfo;
      CHostList * hostlistwd;
      QString uname;
      QString passwd;
      QString currenthostname;
      QString upic;
      QString m_strnickname;
      QString lastsucuname;
      QString lastsucupwd;
      QString newwsvrurl;
      CUserCard* photoLabel;
      vector<QString>m_user;

      dulinklist userlist;
      CUserCard*    cardarray[9];

      struct dulnode* cur;
      CLeftArrawPWDEdit* edit;

      SpiceMulViewer* viewer;
      CMenuWidget* menu;
      QString picpath;

      CUserCard* secondwuserpic;

      //loading
      CLoading* floading;
      CLoading* sloading;
      CLoading* hloading;

      int isviewshow;
      volatile int isdhcpclick;
      QTimer *isdhcpclicktimer;

      QPushButton* retbtn;
      QPushButton* savebtn;

      QLabel* cpwdusername;
      QLabel* cpwdusericon;
      CMyTextEdit* pwdeidt;
      CMyTextEdit* newpwdeidt;
      CMyTextEdit* surenewpwdeidt;

      bool okButtonIsRet;
      bool isstarthost;
      bool hasonceclick;
      bool ismacright;
      int fblIndex;
      QPushButton *m_okBtn;
      ZCMessageBox *msgbox;
      QPushButton *firstLoginBtn;

      QPushButton * secondLoginBtn;
      QPushButton * tmpBtn;
      QPushButton * fblBtn;
signals:
      void spicelogsignal(K_yType val);
};

extern CMenuWidget* g_menu_ptr;

class NetDiagnoseThread : public QThread
{
   Q_OBJECT

   public:
      NetDiagnoseThread(QWidget *parent = NULL)
      {
         mainw = (CMainWindow*)parent;
      }
      ~NetDiagnoseThread()
      {

      }
   public:
      virtual void run()
      {
         mainw->netdiagose();
      }
   private:
      CMainWindow* mainw;
};


#endif // MAINWINDOW_H
