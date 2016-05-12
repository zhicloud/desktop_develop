#ifndef _LOG_H_
#define _LOG_H_
#include <memory>
//#include <tchar.h>
#include <cstdarg>
//#include <QApplication>
//#define _WINSOCKAPI_ 
//#include <windows.h>
//#include<winsock2.h>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
using namespace std;

//#pragma comment(lib,"WS2_32.lib")

#include "singleton.h"
#include "common.h"
class ZCLog
{
public:
	ZCLog();
	~ZCLog();

	void WriteToLog(const LOG_LEVEL level, char *msg);
	void WriteToLog(const LOG_LEVEL level, QString msg);

	void InitLog();
	void UninitLog();

	const char* TypeToString(const LOG_LEVEL type);

	void Push(const QString& fileName);

private:
	//Log4Qt::Logger *m_zclog;
	FILE *m_logoFile;
	QMutex m_mutex;
	
	bool m_isWriteLog;//by xzg
	int m_level[5];//by xzg

	void write(const char* format, ...);

	void log(const LOG_LEVEL type, const char* message);
	void log(const LOG_LEVEL type, const char* format, va_list& varArgs);
};

typedef Singleton<ZCLog> MyZCLog;

#endif
