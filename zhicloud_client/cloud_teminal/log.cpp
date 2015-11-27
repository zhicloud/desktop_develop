#include "log.h"
#include "common.h"
#include <ctime>

ZCLog::ZCLog()
{
	m_logoFile = NULL;
	InitLog();
};

ZCLog::~ZCLog()
{
	UninitLog();
}

//初始化日志
void ZCLog::InitLog()
{
#if 0
	m_logoFile = fopen("cloud_client.log", "w");
	int count = 5;
	while ((m_logoFile ==NULL) && (0 != count))
	{
		count--;
		m_logoFile = fopen("cloud_client.log", "w");
	}
#endif
	m_logoFile = fopen("cloud_client.log", "w");
	int count = 5;
	while ((m_logoFile ==NULL) && (0 != count))
	{
		count--;
		m_logoFile = fopen("cloud_client.log", "w");
	
	}
	if(!m_logoFile)
	{	
		return;
	}
	int c;
	int lines = 0;
	while ((c = getc(m_logoFile)) != EOF)
	{
		if (c == 10)
			lines++;
	}
	printf("log file has %d lines\n", lines);
	if (lines > 100)
	{
		fclose(m_logoFile);
		m_logoFile = fopen("cloud_client.log", "w");
		int count = 5;
		while ((m_logoFile == NULL) && (0 != count))
		{
			count--;
			m_logoFile = fopen("cloud_client.log", "w");
		}
	}
}

void ZCLog::UninitLog()
{	
	if(m_logoFile)
	{
	fclose(m_logoFile);
	m_logoFile = NULL;
	}
}

const char* ZCLog::TypeToString(const LOG_LEVEL type) {
	switch (type) {
	case ZCFATAL:
		return "FATAL";
	case ZCERROR:
		return "ERROR";
	case ZCWARN:
		return "WARN ";
	case ZCINFO:
		return "INFO ";
	case ZCDEBUG:
		return "DEBUG";
	default:
		break;
	}
	return "UNKNOW";
}

void ZCLog::write(const char* format, ...)
{
	if(!m_logoFile)
	{	
		return;
	}
	char buffer[10240] = { 0 };

	va_list varArgs;
	va_start(varArgs, format);
	vsnprintf(buffer, sizeof(buffer), format, varArgs);
	va_end(varArgs);
// 	QTextStream out(&m_file);
// 	out << buffer;
// 	m_file.flush();
	m_mutex.lock();
	fflush(m_logoFile);
	fwrite(buffer, 1, strlen(buffer), m_logoFile);
	fflush(m_logoFile);
	m_mutex.unlock();
}

void ZCLog::log(const LOG_LEVEL type, const char* message)
{
	static const int TIMESTAMP_BUFFER_SIZE = 21;
	char buffer[TIMESTAMP_BUFFER_SIZE];
	time_t timestamp;
	time(&timestamp);
	strftime(buffer, sizeof(buffer), "%X %x", localtime(&timestamp));

	write("%s [%s] - %s", buffer, TypeToString(type), message);
}
void ZCLog::log(const LOG_LEVEL type, const char* format, const va_list& varArgs)
{
	char buffer[10240] = { 0 };
	vsnprintf(buffer, sizeof(buffer), format, varArgs);
	log(type, buffer);
}

//写入日志信息
void ZCLog::WriteToLog(const LOG_LEVEL level, char *msg)
{
	if (msg == "" || msg == NULL) return;
	char tmp[1024] = { 0 };
	strcpy(tmp, msg);
	strcat(tmp, "\n");
	log(level, tmp);
}

void ZCLog::WriteToLog(const LOG_LEVEL level, QString msg)
{
	if (msg == "" || msg == NULL) return;
	msg += "\n";
	log(level, msg.toUtf8().data());
}
