

#include <QPainter>
#include <QImage>
#include <QDebug>
#include <QTimer>
#include "common_operater.h"
#include "SpiceMulViewer.h"
#include <qpushbutton.h>
#include <qevent.h>
#include <QDesktopWidget>
#include <QApplication>
#include "cmenuwidget.h"
#include "cmythread.h"
#include "zc_message_box.h"
#include <QThread>
#include <vector>
//#include "create_bmp.h"
SPICE_HANDLE CSpiceMultVEx::m_hContext = NULL;
int CSpiceMultVEx::m_hmain = 0;
bool CSpiceMultVEx::m_isInit = false;


#define SURSOR_SET_EVENT QEvent::User + 100
#define SURSOR_HIDE_EVENT QEvent::User + 101
#define ORI_DIS_EVENT QEvent::User+102
#define SPICE_CHANNEL_ERROR_LINK QEvent::User+103
#define SPICE_CHANNEL_CLOSED QEvent::User+104  
#define SPICE_CHANNEL_ERROR_AUTH QEvent::User+105
#define SPICE_REFRESH QEvent::User+106
#define ERROR_IO_SHOW_WIDGET QEvent::User+120

class MyThread : public QThread
{
public:
    void run();
    void setArgs(void* pThis,char* ip,char* port);
private:
    void *pThis;
    char ip[20];
    char port[20];
};

void MyThread::run()
{
        
        int ret = 0;
        CSpiceMultVEx *tmp = (CSpiceMultVEx *)pThis; 
	ret = SpiceConnect(CSpiceMultVEx::m_hContext, tmp->m_hspice,ip, port, NULL,
		"123456", NULL, NULL, 1);
	if (ret != 0)
	{
		printf("SpiceConnect failed! error:%d\n", ret);
		ZCMessageBox *msgbox = new ZCMessageBox(QString("SpiceConnect failed!"), tmp, NULL);
		msgbox->show();
		return ;
	}
        while(1)
        {
            sleep(100000000);
        }
}

void MyThread::setArgs(void* pThis,char* ip,char* port)
{
    this->pThis = pThis;
    memset(this->ip,0,20);
    memset(this->port,0,20);
    strcpy(this->ip,ip);
    strcpy(this->port,port);

}


CSpiceMultVEx::CSpiceMultVEx(QWidget *parent)
	: QWidget(parent)
{
	isfresharea = 0;
	img_w = 0;
	img_h = 0;
	m_width = 0;
	m_height = 0;
	m_pImage = NULL;
	m_pImageArrBuf = NULL;
	m_hspice = -1;
	m_isOpen = false;
	m_isstarthost = false;
	memset(keymap_win322xtkbd, 0, sizeof(keymap_win322xtkbd));
	InitKeyCode(keymap_win322xtkbd);
//	this->setFocusPolicy(Qt::StrongFocus);

//****&&&&
	this->grabKeyboard();//forbidden tab key change focus


	m_ptimer = new QTimer(this);

	//新建定时器

	connect(m_ptimer, SIGNAL(timeout()), this, SLOT(timerUpDate()));

	//关联定时器计满信号和相应的槽函数

	//m_ptimer->start(1);

	installEventFilter(this);

	m_ResChange = false;

	m_isFullScr = false;

	m_reSetPos = false;

	setMouseTracking(true);
	m_mainwin = NULL;

	m_thread.SetSpiceView(this);
	m_thread.start();

	m_menutimer = new QTimer(this);
	connect(m_menutimer, SIGNAL(timeout()), this, SLOT(menutimerOut()));
	m_menushow = false;

	minx = -1;
	miny = -1;
	maxw = -1;
	maxh = -1;

	svfirstbmpdata = 0;


	m_heartbeattimer = new QTimer(this);
	connect(m_heartbeattimer, SIGNAL(timeout()), this, SLOT(heartbeat()));
	m_heartbeattimer->start(1000/* * 60*/ * 15);

}

CSpiceMultVEx::~CSpiceMultVEx()
{
	m_heartbeattimer->stop();
      SpiceCallBacks callBack[CB_NULL];
	callBack[0].type = CB_SETTING_CHANGE;
	callBack[0].lcb._cb = NULL;
	callBack[0].lcb._ctx = this;

	callBack[1].type = CB_INVALIDATE;
	callBack[1].lcb._cb = NULL;
	callBack[1].lcb._ctx = this;

	callBack[2].type = CB_MAIN_CHANNEL_EVENT;
	callBack[2].lcb._cb = NULL;
	callBack[2].lcb._ctx = this;

	callBack[3].type = CB_CURSOr_SET;
	callBack[3].lcb._cb = NULL;
	callBack[3].lcb._ctx = this;

	callBack[4].type = CB_CURSOR_MOVE;
	callBack[4].lcb._cb = NULL;
	callBack[4].lcb._ctx = this;

	callBack[5].type = CB_CURSOR_HIDE;
	callBack[5].lcb._cb = NULL;
	callBack[5].lcb._ctx = this;

	callBack[6].type = CB_CURSOR_RESET;
	callBack[6].lcb._cb = NULL;
	callBack[6].lcb._ctx = this;

	int cc = 7;

	int ret = -1;
	SpiceSetCalls(CSpiceMultVEx::m_hContext, m_hspice, callBack, cc);

	CloseSpice();
	m_ptimer->stop();
	m_thread.t_stop();
	m_thread.wait();
	
	logout();

	if (m_pImage != NULL)
	{
		free(m_pImage);
		m_pImage = NULL;
	}
	m_height = m_width = 0;
}


bool CSpiceMultVEx::Spice_Init()
{
	if (m_isInit == true)
	{
		return true;
	}

	//初始化运行环境
	m_hContext = SpiceInit();
	if (!m_hContext)
	{
		return false;
	}
	//打开消息循环
	m_hmain = SpiceOpenMain(m_hContext);
	if (m_hmain)
	{
		SpiceUninit(m_hContext);
		return false;
	}
	//运行消息循环
	int ret = SpiceRunMainLoop(m_hContext, m_hmain, true);
	if (ret != 0)
	{
		printf("SpiceRunMainLoop failed! error:%d\n", ret);
		SpiceCloseMain(m_hContext, m_hmain);
		SpiceUninit(m_hContext);
		return false;
	}
	m_isInit = true;
	return true;
}

bool CSpiceMultVEx::Spice_Uninit()
{
	if (m_isInit)
	{
		SpiceStopMainLoop(m_hContext, m_hmain);
		SpiceCloseMain(m_hContext, m_hmain);
		SpiceUninit(m_hContext);
		m_isInit = false;
	}
	return true;
}

int CSpiceMultVEx::OpenSpice(char* szIP, char* port, char* uname, char* password, int timeout)
{
	printf("ip:%s,port:%s,password:%s",szIP,port,password);
	m_isOpen = false;
	m_opCode[OPEN] = LCX_SPICE_CHANNEL_ERROR_CONNECT;

	if (CSpiceMultVEx::m_isInit == false)
	{
		return false;
	}

	//打开一个spice连接
	m_hspice = SpiceOpen(CSpiceMultVEx::m_hContext);
	if (m_hspice < 0)
	{
		printf("SpiceOpen filed! error:%d\n", m_hspice);
		return false;
	}

	SpiceCallBacks callBack[CB_NULL];
	callBack[0].type = CB_SETTING_CHANGE;
	callBack[0].lcb._cb = changeSetting;
	callBack[0].lcb._ctx = this;

	callBack[1].type = CB_INVALIDATE;
	callBack[1].lcb._cb = invalidate;
	callBack[1].lcb._ctx = this;

	callBack[2].type = CB_MAIN_CHANNEL_EVENT;
	callBack[2].lcb._cb = mainChannel;
	callBack[2].lcb._ctx = this;

	callBack[3].type = CB_CURSOr_SET;
	callBack[3].lcb._cb = cursorSet;
	callBack[3].lcb._ctx = this;

	callBack[4].type = CB_CURSOR_MOVE;
	callBack[4].lcb._cb = cursorMove;
	callBack[4].lcb._ctx = this;

	callBack[5].type = CB_CURSOR_HIDE;
	callBack[5].lcb._cb = cursorHide;
	callBack[5].lcb._ctx = this;

	callBack[6].type = CB_CURSOR_RESET;
	callBack[6].lcb._cb = cursorReset;
	callBack[6].lcb._ctx = this;

	int cc = 7;

	int ret = -1;
	SpiceSetCalls(CSpiceMultVEx::m_hContext, m_hspice, callBack, cc);
   SetUsbEnable(true);

	//qDebug() << "ip: " << szIP << "\n port: " << port;



/*	MyThread *tmp = new MyThread();
        tmp->setArgs(this,szIP,port);
       tmp->start();*/




	//ret = SpiceConnect(CSpiceMultVEx::m_hContext, m_hspice, "172.16.6.1", "5926", NULL,
	ret = SpiceConnect(CSpiceMultVEx::m_hContext, m_hspice,szIP,port,NULL,
		password, NULL, NULL, 1);

	if (ret != 0)
	{
		printf("SpiceConnect failed! error:%d\n", ret);
		ZCMessageBox *msgbox = new ZCMessageBox(QString("SpiceConnect failed!"), this, NULL);
		msgbox->show();
		return false;
	}
	

	m_OpMutex[OPEN].lock();
	MyZCLog::Instance().WriteToLog(ZCINFO, QString("m_OpMutex[OPEN].lock(); "));
	m_OpCond[OPEN].wait(&(m_OpMutex[OPEN]), timeout * 1000);
	m_OpMutex[OPEN].unlock();
	MyZCLog::Instance().WriteToLog(ZCINFO, QString("m_OpMutex[OPEN].unlock(); "));
        return true;
	//return m_opCode[OPEN];
}

bool CSpiceMultVEx::CloseSpice()
{
	if (m_hspice == -1)
		return true;

	if (m_hspice >= 0)
	{
		SpiceDisconnect(CSpiceMultVEx::m_hContext, m_hspice);
		SpiceClose(CSpiceMultVEx::m_hContext, m_hspice);
		m_hspice = -1;
	}



/*
	m_pbufmutex.lock();
	if (m_pImage != NULL)
	{
		free(m_pImage);
		m_pImage = NULL;
	}
	m_height = m_width = 0;
	m_pbufmutex.unlock();
*/	
	m_isOpen = false;
	return true;
}

void* CSpiceMultVEx::changeSetting(void* ctx, void *ud)
{
	SPICE_SettringChange *sc = (SPICE_SettringChange *)ud;
	CSpiceMultVEx *pThis = (CSpiceMultVEx *)ctx;
	//	SPICE_LOG("%d , %d , %d\n", sc->width, sc->height, sc->bpp);

	if ((pThis->m_width != sc->width) ||
		(pThis->m_height != sc->height))
	{
		pThis->m_pbufmutex.lock();
		if (pThis->m_pImage != NULL)
		{
			free(pThis->m_pImage);
			pThis->m_pImage = NULL;
		}
		pThis->m_pImage = (char*)malloc(sc->width * sc->height * 4);

		pThis->m_width = sc->width;
		pThis->m_height = sc->height;
		pThis->m_ResChange = true;
		pThis->m_pbufmutex.unlock();
	}

	QDesktopWidget* desktopWidget = QApplication::desktop();
        QRect screenRect = desktopWidget->screenGeometry();
        if (sc->width != screenRect.width() || sc->height != screenRect.height())
        {
               pThis->ChangeResolution(screenRect.width(), screenRect.height());
	       pThis->isfresharea = 0;
        }

	if(sc->width == screenRect.width() && sc->height == screenRect.height())
	{
	       pThis->isfresharea = 1;
	}

	return NULL;
}

void* CSpiceMultVEx::invalidate(void* ctx, void *ud)
{	
	int iq;
	int iqq;
	int iqqq;
	int iqqqq;
	CSpiceMultVEx *pThis = (CSpiceMultVEx *)ctx;
	SPICE_Invalidate* pud = (SPICE_Invalidate*)ud;

	int size = sizeof(SPICE_Invalidate);
	if (pThis->m_pImage)
	{
		QTime q = QTime::currentTime();
		iq = q.msec();

		char* data = (char*)malloc(size);

		QTime qq = QTime::currentTime();
		iqq = qq.msec();

		memcpy(data, ud, size);

		QTime qqq = QTime::currentTime();
		iqqq = qqq.msec();

		//memcpy(data + size, ((SPICE_Invalidate*)ud)->bitmap, pud->w * pud->h * 4);
		//QTime qqqq = QTime::currentTime();
		//iqqqq = qqqq.msec();
		

		pThis->m_mutex.lock();
		pThis->m_bmplist.push_back(data);
		pThis->m_thread.con.wakeOne();
		pThis->m_mutex.unlock();
		
	}
	if (pThis->m_isOpen == false)
	{
		pThis->m_isOpen = true;
		QEvent *event = new QEvent(QEvent::Type(ORI_DIS_EVENT));
		QCoreApplication::postEvent(pThis->GetMainWin(), event);
		MyZCLog::Instance().WriteToLog(ZCINFO, QString("CSpiceMultVEx post show event; "));
	}

	

	//MyZCLog::Instance().WriteToLog(ZCERROR, QString("invalidate cost %1 %2 %3").arg(iqq - iq).arg(iqqq - iqq).arg(pud->w * pud->h * 4));

	return NULL;
}

void* CSpiceMultVEx::mainChannel(void* ctx, void *ud)
{
	SPICE_MainChannelEvent * event = (SPICE_MainChannelEvent*)ud;
	CSpiceMultVEx* pThis = (CSpiceMultVEx*)ctx;
	switch (event->et) {
	case LCX_SPICE_CHANNEL_OPENED:
		break;
	case LCX_SPICE_CHANNEL_ERROR_CONNECT:
	{

											QEvent *event = new QEvent(QEvent::Type(SPICE_CHANNEL_ERROR_LINK));
											QCoreApplication::postEvent(pThis->GetMainWin(), event);
											break;
	}
	case LCX_SPICE_CHANNEL_ERROR_AUTH:
	{

										 QEvent *event = new QEvent(QEvent::Type(SPICE_CHANNEL_ERROR_AUTH));
										 QCoreApplication::postEvent(pThis->GetMainWin(), event);
										 break;
	}
		break;
	case LCX_SPICE_CHANNEL_SWITCHING:
		//SPICE_LOG("main channel: switching host\n");
		break;
	case LCX_SPICE_CHANNEL_CLOSED:
		/*printf("main channel: closed\n");
		pThis->m_mutex.lock();
		pThis->CloseSpice();
		pThis->m_isOpen = false;
		pThis->m_mutex.unlock();*/

	{

			
				MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("=====T==========================LCX_SPICE_CHANNEL_CLOSED:=============================="));

									 QEvent *event = new QEvent(QEvent::Type(SPICE_CHANNEL_CLOSED));
									 QCoreApplication::postEvent(pThis->GetMainWin(), event);
									 break;
	}
		break;
      
      //qDebug() << "LCX_SPICE_CHANNEL_CLOSED  main channel: closed";
	case LCX_SPICE_CHANNEL_ERROR_IO:
	{
		//SPICE_LOG("main channel: closed\n");
		 //qDebug() << "SPICE_CHANNEL_ERROR_IO main channel: closed";
		 QEvent *event1 = new QEvent(QEvent::Type(ERROR_IO_SHOW_WIDGET));
		 QCoreApplication::postEvent(pThis->GetMainWin(), event1);

		break;
	}
	case LCX_SPICE_CHANNEL_ERROR_TLS:
	case LCX_SPICE_CHANNEL_ERROR_LINK:
		//SPICE_LOG("main channel: auth failure (wrong password?\n");
	{

										 QEvent *event = new QEvent(QEvent::Type(SPICE_CHANNEL_ERROR_LINK));
										 QCoreApplication::postEvent(pThis->GetMainWin(), event);
										 break;
	}
		break;
	default:
		//SPICE_LOG("unknown main channel event: %d\n", event);
		break;
	}
	return NULL;
}

void CSpiceMultVEx::paintEvent(QPaintEvent *event)
{
	m_pbufmutex.lock();
	if (m_pImage == NULL)
	{
		m_pbufmutex.unlock();
		return;
	}
	QImage image = QImage((uchar*)m_pImage, m_width, m_height, QImage::Format_RGB32);
	m_pbufmutex.unlock();
	
 	QDesktopWidget* desktopWidget = QApplication::desktop();
 	QRect screenRect = desktopWidget->screenGeometry();
	if (image.width() != screenRect.width() || image.height() != screenRect.height())
		image = image.scaled(screenRect.width(), screenRect.height());

	QPainter paint(this);
	paint.drawImage(QPoint(0, 0), image);
	paint.end();
	this->resize(screenRect.width(), screenRect.height());
}

void CSpiceMultVEx::closeEvent(QCloseEvent * event)
{
	if (m_isOpen == false)
	{
		return;
	}
	if (m_hspice == -1)
	{
		return;
	}
	SpiceDisconnect(CSpiceMultVEx::m_hContext, m_hspice);
	SpiceClose(CSpiceMultVEx::m_hContext, m_hspice);
	m_hspice = -1;
}

void CSpiceMultVEx::timerUpDate()
{
	if (m_isOpen == false)
	{
		return;
	}
	

	if (m_ResChange)
	{
		setGeometry(0, 0, m_width, m_height);
		m_ResChange = false;
	}
	//repaint();
	m_mutex.lock();
	if (m_bmplist.size() == 0)
	{
		m_mutex.unlock();
		return;
	}
	
}

bool CSpiceMultVEx::eventFilter(QObject *obj, QEvent *ev)
{

	do 
	{

		if (obj != this)
		{
			break;
		}

		if (m_isOpen == false)
		{
			break;
		}

		if (m_hspice == -1)
		{
			break;
		}

   /*
		if (QString("%1").arg(1) != m_hostisusb)
		{
			return QWidget::eventFilter(obj, ev);
		}
*/
      
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(ev);
		

		QPoint pt = CalcPos(mouseEvent->pos());

//		MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("=====EVENT POS:x=%1,y=%2========").arg(pt.x()).arg(pt.y()));
		if (m_menu)
		{
			QDesktopWidget* desktopWidget = QApplication::desktop();
 			QRect screenRect = desktopWidget->screenGeometry();


			QMouseEvent* even = (QMouseEvent*)ev;
			if (even->pos().y() >= 0 && even->pos().y() <= 52 && 
		even->pos().x() > (screenRect.width() / 2 - 500 / 2) && even->pos().x() < (screenRect.width() / 2 + 500 / 2))
			{
				m_menushow = true;
				if (!m_menutimer->isActive())
				{
					MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("=====TIME START EVENT POS:x=%1,y=%2========").arg(even->pos().x()).arg(even->pos().y()));
					if (ev->type() == QEvent::MouseMove)
					{
						m_menutimer->start(2000);
					}
					//qDebug() << "timer start ....................";
				}
			}

		}
		

		if (ev->type() == QEvent::MouseMove)
		{
			if (m_menu)
			{
			
				int haveunread = 0;
				QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
				if (msgfile)
				{
					msgfile->beginReadArray("messages");
					QStringList keys = msgfile->allKeys();
					keys.removeOne("lasttime");
					int size = keys.size();
					for (int i = 0; i < size; i++)
					{
						QString val = msgfile->value(keys.at(i)).toString();
						QStringList list = val.split(",");
						if (list[3] == "unread")
						{
							haveunread = 1;
							MyZCLog::Instance().WriteToLog(ZCERROR, QString("haveunread mail = %1").arg(haveunread));
							break;
						}
						else
						{
							continue;
						}

					}
				}
				
				if(haveunread == 0)
				{
					if(m_menu->mail_status == 1)
					{
						m_menu->mail_status == 0;
						m_menu->mail->setStyleSheet("QPushButton#mail{border-image: url(:/menu/msgno);}"
										"QPushButton#mail:hover{border-image: url(:/menu/msgno);}"
										"QPushButton#mail:pressed{border-image: url(:/menu/msgnoclick);}");
					}
		

				QDesktopWidget* desktopWidget = QApplication::desktop();
                        	QRect screenRect = desktopWidget->screenGeometry();
				QMouseEvent* even = (QMouseEvent*)ev;
				//if (even->pos().y() > 0 && even->pos().y() < 30)
				if (even->pos().y() >= 52 || (even->pos().x() < (screenRect.width() / 2 - 500 / 2)) ||
				 (even->pos().x() > (screenRect.width() / 2 + 500 / 2)))
				{
					if (m_menutimer->isActive())
					{
						m_menutimer->stop();
						m_menushow = false;
						//qDebug() << "timer out stop .....................";
					}
					m_menu->hide();
				}
				
				
				}
				else
				{
					if(m_menu->mail_status == 0)
					{
						m_menu->mail_status = 1;
						m_menu->mail->setStyleSheet("QPushButton#mail{border-image: url(:/menu/msghave);}"
						"QPushButton#mail:hover{border-image: url(:/menu/msghave);}"
						"QPushButton#mail:pressed{border-image: url(:/menu/msghaveclick);}");
					}
					m_menu->show();
				}
			}
			SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, 0);
		}
		else if (ev->type() == QEvent::MouseButtonPress)
		{
			Qt::MouseButton b = mouseEvent->button();
			switch (b)
			{
			case Qt::RightButton:
				SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, (PTRFLAGS_DOWN | LCX_SPICE_MOUSE_BUTTON_RIGHT));
				break;
			case Qt::LeftButton:
				SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, PTRFLAGS_DOWN | LCX_SPICE_MOUSE_BUTTON_LEFT);
				break;
			case Qt::MidButton:
				SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, PTRFLAGS_DOWN | LCX_SPICE_MOUSE_BUTTON_MIDDLE);
				break;
			default:
				break;
			}
		}
		else if (ev->type() == QEvent::MouseButtonRelease)
		{
			Qt::MouseButton b = mouseEvent->button();
			switch (b)
			{
			case Qt::RightButton:
				SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, 0 | LCX_SPICE_MOUSE_BUTTON_RIGHT);
				break;
			case Qt::LeftButton:
				SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, LCX_SPICE_MOUSE_BUTTON_LEFT | 0);
				break;
			case Qt::MidButton:
				SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, LCX_SPICE_MOUSE_BUTTON_MIDDLE | 0);
				break;

			default:
				break;
			}
		}
		else if (ev->type() == QEvent::MouseButtonDblClick)
		{
			
			/*
			if (m_isGet == false)
			{
				m_oldrc = geometry();
				m_isGet = true;

			}

			static int i = 0;
			if (i == 0)
			{
				setGeometry(0, 0, m_width, m_height);
				i = 1;
			}
			else
			{
				i = 0;
				setGeometry(0, 0, m_oldrc.width(), m_oldrc.height());
			}
			//*/
			
			SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, PTRFLAGS_DOWN | LCX_SPICE_MOUSE_BUTTON_LEFT);
			//SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, LCX_SPICE_MOUSE_BUTTON_LEFT | 0);
			//SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, PTRFLAGS_DOWN | LCX_SPICE_MOUSE_BUTTON_LEFT);
			//SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, LCX_SPICE_MOUSE_BUTTON_LEFT | 0);
		}
		else if (QEvent::Wheel == ev->type())
		{
			QWheelEvent *mouseWheelEvent = static_cast<QWheelEvent*>(ev);

			if (mouseWheelEvent->delta() < 0)
			{
				SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, PTRFLAGS_DOWN | LCX_SPICE_MOUSE_BUTTON_DOWN);
			}
			else
			{
				SpiceButtonEvent(CSpiceMultVEx::m_hContext, m_hspice, pt.x(), pt.y(), 0, PTRFLAGS_DOWN | LCX_SPICE_MOUSE_BUTTON_UP);
			}

		}
		else if (ev->type() == QEvent::KeyPress)
		{

			//printf("KeyPress\n");

			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
			quint32 virtualKey = keyEvent->nativeVirtualKey();
			MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("=====KEY PRESS EVENT -> VIRTUALKEY : %1========").arg(virtualKey));
			if (virtualKey <= 65536)
			{
            MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("=====KEY PRESS EVENT -> WIN32KEY : %1==333333333333333333======").arg(keymap_win322xtkbd[virtualKey]));
				SpiceKeyEvent(m_hContext, m_hspice, true, keymap_win322xtkbd[virtualKey]);
			}
		//	MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("=====KEY PRESS EVENT -> WIN32KEY : %1========").arg(keymap_win322xtkbd[virtualKey]));
			//qDebug() << "===========" << virtualKey;
			if(virtualKey == 65289)
			ev->ignore();
		}
		else if (ev->type() == QEvent::KeyRelease)
		{
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
			quint32 virtualKey = keyEvent->nativeVirtualKey();
			qDebug() << ".............................." << virtualKey;
			if (virtualKey <= 65536)
            {
               MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("=====KEY RELEASE EVENT -> VIRTUALKEY : %1========").arg(virtualKey));
			   	SpiceKeyEvent(m_hContext, m_hspice, false, keymap_win322xtkbd[virtualKey]);
            }
      }
//		else
		{
//			printf("type = %d\n", ev->type());
		}

	} while (0);
//	MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("=====KEY EVENT -> VIRTUALKEY : %1========").arg(virtualKey));
	return QWidget::eventFilter(obj, ev);
}

QPoint CSpiceMultVEx::CalcPos(QPoint pt)
{
	if (m_isFullScr)
	{
		return QPoint(pt.x() - m_LTPt.x(),pt.y() - m_LTPt.y());
	}
	return pt;
}

void CSpiceMultVEx::setMenu(CMenuWidget* menu) 
{
	m_menu = menu; 
	m_menu->setViewer(this);
}

void CSpiceMultVEx::SetScrollA(QScrollArea* pScollA)
{
	m_pScrollA = pScollA;
}

void CSpiceMultVEx::SetFullScreen(bool isFull)
{
	m_isFullScr = isFull;
	m_reSetPos = true;

}

void* CSpiceMultVEx::cursorSet(void* ctx, void *ud)
{
	//printf("========cursorSet===========\n");
	CSpiceMultVEx *pThis = (CSpiceMultVEx *)ctx;

	SPICE_CursorSet* tmp = (SPICE_CursorSet*)ud;
	QPixmap pImage;
	QByteArray imageByteArray = QByteArray((const char*)tmp->data, tmp->width * tmp->height * 4);
	uchar*  transData = (unsigned char*)imageByteArray.data();
	QImage image = QImage(transData, tmp->width, tmp->height, QImage::Format_RGBA8888);
	pImage = QPixmap::fromImage(image);
	
	pThis->m_Curor = QCursor(pImage, tmp->hot_spot_x, tmp->hot_spot_y);
	QEvent *event = new QEvent(QEvent::Type(SURSOR_SET_EVENT));
	QCoreApplication::postEvent(pThis, event);
	
	return (void*)NULL;
}

void* CSpiceMultVEx::cursorMove(void* ctx, void *ud)
{
	return NULL;
}

void* CSpiceMultVEx::cursorHide(void* ctx, void *ud)
{
	//QApplication::setOverrideCursor(Qt::BlankCursor);
	CSpiceMultVEx *pThis = (CSpiceMultVEx *)ctx;
	QEvent *event = new QEvent(QEvent::Type(SURSOR_HIDE_EVENT));
	QCoreApplication::postEvent(pThis, event);
	return (void*)NULL;
}

void* CSpiceMultVEx::cursorReset(void* ctx, void *ud)
{
	return NULL;
}

void CSpiceMultVEx::customEvent(QEvent *event)
{
	int type = event->type();
	
	switch (type)
	{
	case SPICE_REFRESH:
	{
		QRectEvent* ev = (QRectEvent*)event;
		if(isfresharea)
		{
		   update(ev->GetRect());
		}
		else
		{
		  repaint();
		}
		event->accept();
	 break;
	}
	case SURSOR_SET_EVENT:
	{
		 //QApplication::setOverrideCursor(m_Curor);	
		 setCursor(m_Curor);
		 event->accept();
		 break;
	}
	case SURSOR_HIDE_EVENT:
	{
		setCursor(Qt::BlankCursor);
		event->accept();
		break;
	}
	default:
		event->accept();
		break;
	}
}

void CSpiceMultVEx::setMainWin(QWidget *parent)
{
	m_mainwin = dynamic_cast<CMainWindow*>(parent);
}

void CSpiceMultVEx::setViewerSuc()
{
	m_opCode[OPEN] = LCX_SPICE_CHANNEL_OPENED;
}

void CSpiceMultVEx::menutimerOut()
{
	m_menutimer->stop();
	//qDebug() << "in the timerout stop .........................." << QString("%1").arg(m_menushow);
	if (m_menushow)
		m_menu->show();
		MyZCLog::Instance().WriteToLog(ZCDEBUG, QString("=====m_menu->show();========"));
}

void CSpiceMultVEx::setHostUsb(const QString &isusb)
{
	m_hostisusb = isusb;
}

void CSpiceMultVEx::setStartHost(bool istarthost)
{
	m_isstarthost = istarthost;
}

void CSpiceMultVEx::Update()
{
	m_mutex.lock();
	m_thread.con.wait(&m_mutex);
	int size_t = m_bmplist.size();
	if (size_t == 0)
	{
		m_mutex.unlock();
		return;
	}
	if(m_width == 0)
	{
		return;
	}
	list<char*>tmplist = m_bmplist;
	m_bmplist.clear();
	m_mutex.unlock();

	list<char*>::iterator ita = tmplist.begin();
	int size = tmplist.size();
	int x, y, w, h;
	find_max_range(tmplist,x,y,w,h);


	m_pbufmutex.lock();
	for (; ita != tmplist.end(); ita++)
	{
		char* ptr = *ita;
		SPICE_Invalidate* ud = (SPICE_Invalidate*)ptr;
	
		if (ud->x >= 0 && ud->y >= 0 && (ud->x + ud->w) <= m_width && (ud->y + ud->h) <= m_height)
			imageCombined((char*)ud->bitmap,ud->x,ud->y,ud->w,ud->h);
		
		ud->free_bitmap(ud->bitmap);
		free(*ita);
	}
	m_pbufmutex.unlock();
	tmplist.clear();

	QRectEvent *event = new QRectEvent(QEvent::Type(SPICE_REFRESH));
	event->SetRect(QRect(x,y,w,h));
	QCoreApplication::postEvent(this, event);
}

void CSpiceMultVEx::imageCombined(char* bitmap, int x, int y, int width, int height)
{
	if (bitmap == NULL || m_pImage == NULL)
		return;

	uchar* pixels = (uchar*)m_pImage;

	int slen = m_width * 4;
	int slen1 = width * 4;
	int offset = (slen * y) + (x * 4);
	uchar *source = (uchar*)bitmap;
	//uchar *sourcepix = (uchar*)&source[offset];
	uchar *sourcepix = (uchar*)source;
	uchar *destpix = (uchar*)&pixels[offset];

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width * 4; j += 4) {
			destpix[j + 0] = sourcepix[j + 0];
			destpix[j + 1] = sourcepix[j + 1];
			destpix[j + 2] = sourcepix[j + 2];
		}
		sourcepix = sourcepix + slen1;
		destpix = destpix + slen;
	}
}

void CSpiceMultVEx::find_max_range(list<char*> img_list, int &x, int &y, int &w, int &h)
{
 	w = h = 0;
 	x = y = 0xffff;
 	list<char*>::iterator iter = img_list.begin();
	if (iter == img_list.end())
 	{
		return;
 	}
 	
 	int x1,  y1,w1,h1;
 	char* ptr = *iter;
 	SPICE_Invalidate* ud = (SPICE_Invalidate*)ptr;
 	x1 = ud->x;
 	y1 = ud->y;
	w1 = ud->w;
	h1 = ud->h;
 
	for (; iter != img_list.end(); iter++)
	{
		char* ptr = *iter;
		SPICE_Invalidate* ud = (SPICE_Invalidate*)ptr;
		//x2 = ud->x;
		//y2 = ud->y;
		if (ud->x < x) x = ud->x;
		if (ud->y < y) y = ud->y;
		w = (max((x1 + w1), (ud->x + ud->w)) - x);
		h = (max((y1 + h1), (ud->y + ud->h)) - y);
		w1 = w;
		h1 = h;
		x1 = x;
		y1 = y;
	}
 
}

int CSpiceMultVEx::ChangeResolution(int w, int h)
{
	SpiceRequestResolution(m_hContext, m_hspice, w, h);
	return 1;
}
int CSpiceMultVEx::SetUsbEnable(int isenable)
{
	SpiceEnableAutoUSBRedirectBefore(m_hContext,m_hspice,isenable);
	//SpiceSetUSBFilter(m_hContext,m_hspice,"0x08,-1,-1,-1,1|07,-1,-1,-1,1|0x0B,-1,-1,-1,1|-1,-1,-1,-1,0");
   //SpiceSetUSBRedirOnConnectFilter(m_hContext,m_hspice,"0x08,-1,-1,-1,1|0x07,-1,-1,-1,1|0x0B,-1,-1,-1,1|-1,-1,-1,-1,0");
	SpiceSetUSBFilterBefore(m_hContext,m_hspice,"-1,7104,32787,-1,1|0x07,-1,-1,-1,1|0x08,-1,-1,-1,1|0x02,-1,-1,-1,1|-1,-1,-1,-1,0");
   SpiceSetUSBRedirOnConnectFilterBefore(m_hContext,m_hspice,"-1,7104,32787,-1,1|0x07,-1,-1,-1,1|0x08,-1,-1,-1,1|0x02,-1,-1,-1,1|-1,-1,-1,-1,0");



		
	//SpiceSetUSBFilter(m_hContext,m_hspice,"-1,-1,-1,-1,1");
	return 1;
}


void CSpiceMultVEx::logout()
{
	QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
        QString svrurl, uname;
        if (settings)
        {
                svrurl = settings->value("server/url").toString();

                uname = settings->value("lastsucuser/path").toString();
        }

        QString strfmt("logout?user_name=%1");
        QString url = strfmt.arg(uname);
        svrurl.replace("connect", url);
//        qDebug() << "LogOut : " << svrurl;
	m_mainwin->m_cmd=LogOut;
        m_mainwin->doHttpGet(LogOut, svrurl);

}



void CSpiceMultVEx::heartbeat()
{
	QSettings* settings = new QSettings(INIFILE, QSettings::IniFormat);
	QString svrurl, uname;
	if (settings)
	{
		svrurl = settings->value("server/url").toString();

		uname = settings->value("lastsucuser/path").toString();
	}

	QString lasttime = "0";
	QSettings* msgfile = new QSettings(MSGFILE, QSettings::IniFormat);
	if (msgfile)
	{
		
		lasttime = msgfile->value("messages/lasttime").toString();
	}

	
	QString strfmt("keepalive?user_name=%1&time=%2");
	QString url = strfmt.arg(uname).arg(lasttime);
	svrurl.replace("connect", url);
	//qDebug() << "HeartBeat : " << svrurl;
	m_mainwin->m_cmd = HeartBeat;
	m_mainwin->doHttpGet(HeartBeat, svrurl);

}


SpiceMulViewer::SpiceMulViewer(QWidget *parent, int width, int height)
: QWidget(parent)
{
	m_width = width;
	m_height = height;
	//m_ScrollA = new QScrollArea(this);
	//m_ScrollA->setGeometry(0, 0, width, height);
	m_SpiceViewer = new CSpiceMultVEx(this);
	//m_SpiceViewer->SetScrollA(m_ScrollA);
	//m_ScrollA->setWidget(m_SpiceViewer);
	//m_ScrollA->setWidgetResizable(false);
	//m_FullScrBtn = new QPushButton(this);
	//m_FullScrBtn->setText(" ");

	//m_FullScrBtn->move(width / 2 - m_FullScrBtn->geometry().width() / 2, 0);
	//m_FullScrBtn->setFlat(true);
//	connect(m_FullScrBtn, SIGNAL(clicked()), this, SLOT(BtnClick()));

	m_isGetInitpos = false;
	QPalette palette;
	palette.setColor(QPalette::Background, QColor(0, 0, 0));
	//palette.setBrush(QPalette::Background, QBrush(QPixmap(":/background.png")));
	setPalette(palette);
}

SpiceMulViewer::~SpiceMulViewer()
{
	if (m_SpiceViewer)
	{
		delete m_SpiceViewer;
		m_SpiceViewer = NULL;
	}
	
	//if (m_FullScrBtn)
	//{
	//	delete m_FullScrBtn;
	//	m_FullScrBtn = NULL;
	//}
}


bool SpiceMulViewer::Spice_Init()
{
	return CSpiceMultVEx::Spice_Init();
}

bool SpiceMulViewer::Spice_Uninit()
{
	return CSpiceMultVEx::Spice_Uninit();
}

int SpiceMulViewer::OpenSpice(char* szIP, char* port, char* uname /*= NULL*/, char* password /*= NULL*/, int timeout /*= 3/*3s*/)
{
	return m_SpiceViewer->OpenSpice(szIP, port, uname, password, timeout);
}

bool SpiceMulViewer::CloseSpice()
{
	return m_SpiceViewer->CloseSpice();
}

bool SpiceMulViewer::Spice_IsInit()
{
	return CSpiceMultVEx::Spice_IsInit();
}

void SpiceMulViewer::BtnClick()
{
	//*//
// 	QRect rc;
// 	if (m_SpiceViewer->isFullScreen() == false)
// 	{
		

		QDesktopWidget* desktopWidget = QApplication::desktop();
		//获取设备屏幕大小
		QRect screenRect = desktopWidget->screenGeometry();
		
		if (m_isGetInitpos == false)
		{
			m_isGetInitpos = true;
			x = geometry().x();
			y = geometry().y();
		}

// 		m_SpiceViewer->m_isgetWHMutex.lock();
// 		m_SpiceViewer->m_isgetWH.wait(&(m_SpiceViewer->m_isgetWHMutex), 3 * 1000);
// 		m_SpiceViewer->m_isgetWHMutex.unlock();

		int w = m_SpiceViewer->GetWidth();
		int h = m_SpiceViewer->GetHeight();
		m_SpiceViewer->move((screenRect.width() - w) / 2, (screenRect.height() - h) / 2);
		//m_ScrollA->setGeometry((screenRect.width() - w) / 2, (screenRect.height() - h) / 2,w,h);
	//	rc = m_SpiceViewer->geometry();
		//m_FullScrBtn->setText(" ");
		//m_FullScrBtn->move(screenRect.width() / 2 - m_FullScrBtn->geometry().width() / 2, 0);
		m_SpiceViewer->SetFullScreen(true);
		//m_ScrollA->setDisabled(true);
		//m_ScrollA->setWidgetResizable(true);
		setWindowFlags(Qt::Dialog);
		showFullScreen();
		
	

// 	}
// 	else
// 	{
// 		setWindowFlags(Qt::SubWindow);
// 		//move(x, y);
// 		m_ScrollA->setGeometry(x, y, m_width, m_height);
// 		m_ScrollA->setDisabled(false);
// 		m_ScrollA->setWidgetResizable(false);
// 		m_FullScrBtn->move(x + m_width / 2 - m_FullScrBtn->geometry().width() / 2, 0);
// 		m_SpiceViewer->SetFullScreen(false);		
// 		showNormal();
// 	}
	//*/



}

void SpiceMulViewer::setMenu(CMenuWidget* menu)
{
	//m_menu = menu;
	m_SpiceViewer->setMenu(menu);
}

void SpiceMulViewer::setMultVExParent(QWidget *parent)
{
	m_SpiceViewer->setMainWin(parent);
}

void SpiceMulViewer::setViewerSuc()
{
	m_SpiceViewer->setViewerSuc();
}

void SpiceMulViewer::setHostUsb(const QString &isusb)
{
	m_SpiceViewer->setHostUsb(isusb);
}

int SpiceMulViewer::ChangeResolution(int w, int h)
{
	return m_SpiceViewer->ChangeResolution(w, h);
}
int SpiceMulViewer::SetUsbEnable(int isenable)
{
	return m_SpiceViewer->SetUsbEnable(isenable);
}
void SpiceMulViewer::setStartHost(bool istarthost)
{
	m_SpiceViewer->setStartHost(istarthost);
}
