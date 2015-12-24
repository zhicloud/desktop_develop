#include "cmythread.h"
#include "SpiceMulViewer.h"
CMyThread::CMyThread(QWidget *parent)
	: QThread(parent)
{
	m_isRunning = true;
}

CMyThread::~CMyThread()
{

}

void CMyThread::run()
{
	while (m_isRunning)
	{
		m_ex->Update();
	}
}

void CMyThread::SetSpiceView(CSpiceMultVEx* ex)
{
	m_ex = ex;
}

void CMyThread::t_stop()
{
	m_isRunning = false;
	m_ex->m_mutex.lock();
	con.wakeOne();
	m_ex->m_mutex.unlock();
}
