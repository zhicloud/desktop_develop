#ifndef SPICEMULVIEWER_H
#define SPICEMULVIEWER_H

#include <vector>          
#include <list>
using namespace std;

#include <QPainter>        
#include <QImage>          
#include <QDebug>          
#include <QPushButton>     
#include <QEvent>          
#include <QTimer>          
#include <QMutexLocker>    
#include <QThread>         
#include <QDesktopWidget>  
#include <QApplication>    
#include <QWidget>
#include <QScrollArea>
#include <QWaitCondition>
#include <QPushButton>
#include <QPoint>
#include <QMutex>

#include "common_operater.h"
#include "cmenuwidget.h"   
#include "cmythread.h"     
#include "zc_message_box.h"
#include "mainwindow.h"
#include "common_operater.h"
#include "../packet/include/win32_spice_interface_types.h"
#include "../packet/include/win32_spice_interface.h"
#include "cmythread.h"
#include "qrectevent.h"
#include "usbconfig.h"

#define  MAX_OP_COUNT 10


class CMainWindow;

enum OpEnum
{
	OPEN,
	CLOSE,
	OTHER,
};
class CMenuWidget;
class CSpiceMultVEx;
class SpiceMulViewer : public QWidget
{
	Q_OBJECT

public:
	SpiceMulViewer(QWidget *parent,int width = 1024,int height = 768);
	~SpiceMulViewer();

	static bool Spice_Init();
	static bool Spice_Uninit();
	static bool Spice_IsInit();

	int OpenSpice(char* szIP, char* port, char* uname = NULL, char* password = NULL, int timeout = 1/*3s*/);//sync
	bool CloseSpice();
	int SetUsbEnable(int isenable);
	int ChangeResolution(int w, int h);
	void setMenu(CMenuWidget* menu);
	void setMultVExParent(QWidget *parent);
	void setViewerSuc();
	void setHostUsb(const QString &isusb);
	void setStartHost(bool istarthost);

private:
	QScrollArea* m_ScrollA;
	int isFreshLocalArea;
//	QPushButton *m_FullScrBtn;
	int x;
	int y;
	int m_width;
	int m_height;
	bool m_isGetInitpos;
	CMenuWidget* m_menu;
	
public:
	CSpiceMultVEx* m_SpiceViewer;
public slots:
	void BtnClick();
};


class CSpiceMultVEx : public QWidget
{
	Q_OBJECT

public:
        
	explicit CSpiceMultVEx(QWidget *parent = 0);
	~CSpiceMultVEx();
	void SetScrollA(QScrollArea* pScollA);
	void SetFullScreen(bool isFull);
	bool isFullScreen(){ return m_isFullScr; };
	void setMenu(CMenuWidget* menu);
	static bool Spice_Init();
	static bool Spice_Uninit();
	static bool Spice_IsInit(){ return m_isInit; }

	int OpenSpice(char* szIP, char* port, char* uname = NULL, char* password = NULL, int timeout = 1/*3s*/);//sync
	bool CloseSpice();

	int GetWidth(){ return m_width; };
	int GetHeight(){ return m_height; };
	void setMainWin(QWidget *parent);
	CMainWindow* GetMainWin() { return m_mainwin; };
	void setViewerSuc();
	void setHostUsb(const QString &isusb);
	void setStartHost(bool istarthost);
	int ChangeResolution(int w, int h);
	int SetUsbEnable(int isenable);
	static void* changeSetting(void* ctx, void *ud);
	static void* invalidate(void* ctx, void *ud);
	static void* mainChannel(void* ctx, void *ud);
	static void* cursorSet(void* ctx, void *ud);
	static void* cursorMove(void* ctx, void *ud);
	static void* cursorHide(void* ctx, void *ud);
	static void* cursorReset(void* ctx, void *ud);
	void Update();

	void imageCombined(char* bitmap, int x, int y, int width, int height);
	void logout();

protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void closeEvent(QCloseEvent * event);
	virtual void customEvent(QEvent *event);
	QPoint CalcPos(QPoint pt);
	private slots:
	void timerUpDate();

	public slots:
	bool eventFilter(QObject *obj, QEvent *ev);
	void menutimerOut();
	void heartbeat();
public:


	int isfresharea;

	list<SPICE_Invalidate*>m_bmplist;
	QMutex m_mutex;
	QMutex m_pbufmutex;
	QMutex m_parrbuffmuext;

	int img_w;
	int img_h;

	QRect m_freshrect;
	static SPICE_HANDLE m_hContext;
	int m_hspice;
private:
	CMyThread m_thread;
	static int m_hmain;
	static bool m_isInit;
	CMenuWidget* m_menu;


	bool m_isFullScr;
	bool m_reSetPos;
	

	int minx;
	int miny;
	int maxw;
	int maxh;

	int m_width;
	int m_height;

	int svfirstbmpdata;
	char* m_pImage;
	
	uchar* m_pImageArrBuf;
	//int m_hspice;
	bool m_isOpen;
	
	
	QTimer* m_ptimer;
	bool m_ResChange;
	QPoint m_LTPt;
	QScrollArea* m_pScrollA;
	QCursor m_Curor;
	QWaitCondition m_OpCond[MAX_OP_COUNT];
	QMutex m_OpMutex[MAX_OP_COUNT];
	LcxSpiceChannelEvent m_opCode[MAX_OP_COUNT];
	QMutex  m_updatemutex;

	

public:
#ifdef __linux
	UINT16			keymap_win322xtkbd[65536];
#else
	UINT16			keymap_win322xtkbd[256];
#endif
	
	QWaitCondition m_isgetWH;
	QMutex m_isgetWHMutex;
	CMainWindow *m_mainwin;

	volatile bool m_menushow;
	bool m_isstarthost;
	QTimer *m_menutimer;
	QString m_hostisusb;

	QTimer *m_heartbeattimer;
	usbonfig* m_usb;
};


#endif // SPICEMULVIEWER_H
