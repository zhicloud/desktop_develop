#ifndef CMYTHREAD_H
#define CMYTHREAD_H

#include <QThread>
#include <QWaitCondition>
class CSpiceMultVEx;
class CMyThread : public QThread
{
	Q_OBJECT

public:
	CMyThread(QWidget *parent = NULL);
	~CMyThread();
	void t_stop();
	void SetSpiceView(CSpiceMultVEx* ex);
	void Signalcon();
public:
	virtual void run();

	QWaitCondition con;
private:
	//QMutex m_conmutex;
	CSpiceMultVEx* m_ex;
	bool m_isRunning;
};

#endif // CMYTHREAD_H
