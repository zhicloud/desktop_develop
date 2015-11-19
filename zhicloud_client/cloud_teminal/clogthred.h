#ifndef CLOGTHRED_H
#define CLOGTHRED_H

#include <QThread>

class CMainWindow;
#include "mainwindow.h"

class ClogThred : public QThread
{
	Q_OBJECT

public:
	ClogThred(CMainWindow *mainw, QString ip, QString port, QString acc, QString pwd, QThread *parent = 0);
	~ClogThred();

	void startJob();
	void stopJob();

	bool isRunning;

private:
	void run();
	CMainWindow *mainw;
	QString ip;
	QString port;
	QString acc;
	QString pwd;
};

#endif // CLOGTHRED_H
