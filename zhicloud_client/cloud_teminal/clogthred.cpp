#include "clogthred.h"
#include <QDebug>

ClogThred::ClogThred(CMainWindow *mainw, QString ip, QString port, QString acc, QString pwd, QThread *parent)
: QThread(parent), mainw(mainw), ip(ip), port(port), acc(acc), pwd(pwd)
{

}

ClogThred::~ClogThred()
{

}

void ClogThred::stopJob()
{
	isRunning = false;
}

void ClogThred::startJob()
{
	isRunning = true;
	start();
}

void ClogThred::run()
{

	qDebug() << "thread,,,,,,,";
	if (mainw->spicelogin(ip, port, acc, pwd))
	{
	//	mainw->saveuserpictofile();
	}
}
