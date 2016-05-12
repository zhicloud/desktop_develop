#include "log.h"
#include "common.h"
#include <ctime>
#include <stdarg.h>

#include <QSettings> //by xzg
#define LXLLOG//by xzg

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
		m_logoFile = fopen("/home/cloud_teminal/cloud_client.log", "w");
	}
#endif
/***********************by xzg******************************/
	m_isWriteLog = false;
	for(int i = 0; i < 5; i++){
		m_level[i] = 0;
	}
	QSettings *log_level_file = new QSettings("/home/cloud_teminal/log.ini", QSettings::IniFormat);
	if(log_level_file){
		foreach(QString group, log_level_file->childGroups()){
			if(group == QString("LOG_LEVEL")){
				log_level_file->beginGroup(group);
				m_level[0] = log_level_file->value("ZCFATAL").toInt();	
				m_level[1] = log_level_file->value("ZCERROR").toInt();	
				m_level[2] = log_level_file->value("ZCWARN").toInt();	
				m_level[3] = log_level_file->value("ZCINFO").toInt();	
				m_level[4] = log_level_file->value("ZCDEBUG").toInt();	
				log_level_file->endGroup();
				break;
			}
		}
	}
	delete log_level_file;
/***********************by xzg******************************/
	m_logoFile = fopen("/home/cloud_teminal/cloud_client.log", "w");
	int count = 5;
	while ((m_logoFile ==NULL) && (0 != count))
	{
		count--;
		m_logoFile = fopen("/home/cloud_teminal/cloud_client.log", "w");
	
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
	//printf("log file has %d lines\n", lines);
	if (lines > 100)
	{
		fclose(m_logoFile);
		m_logoFile = fopen("/home/cloud_teminal/cloud_client.log", "w");
		int count = 5;
		while ((m_logoFile == NULL) && (0 != count))
		{
			count--;
			m_logoFile = fopen("/home/cloud_teminal/cloud_client.log", "w");
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
		//***by xzg***
		if(m_level[0] == 1)
			m_isWriteLog = true;
		else
			m_isWriteLog = false;
		//***by xzg***
		return "FATAL";
	case ZCERROR:
		if(m_level[1] == 1)
			m_isWriteLog = true;
		else
			m_isWriteLog = false;
		return "ERROR";
	case ZCWARN:
		if(m_level[2] == 1)
			m_isWriteLog = true;
		else
			m_isWriteLog = false;
		return "WARN ";
	case ZCINFO:
		if(m_level[3] == 1)
			m_isWriteLog = true;
		else
			m_isWriteLog = false;
		return "INFO ";
	case ZCDEBUG:
		if(m_level[4] == 1)
			m_isWriteLog = true;
		else
			m_isWriteLog = false;
		return "DEBUG";
	default:
		m_isWriteLog = false;//***by xzg***
		break;
	}
	return "UNKNOW";
}

void ZCLog::write(const char* format, ...)
{
	if(!m_isWriteLog)
		return;
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
void ZCLog::log(const LOG_LEVEL type, const char* format, va_list& varArgs)
{
	char buffer[10240] = { 0 };
	vsnprintf(buffer, sizeof(buffer), format, varArgs);
	log(type, buffer);
}

//写入日志信息
void ZCLog::WriteToLog(const LOG_LEVEL level, char *msg)
{
#ifdef LXLLOG
	if (msg == "" || msg == NULL) return;
	char tmp[1024] = { 0 };
	strcpy(tmp, msg);
	strcat(tmp, "\n");
	log(level, tmp);
#else
	return;
#endif;
}

void ZCLog::WriteToLog(const LOG_LEVEL level, QString msg)
{
#ifdef LXLLOG
	if (msg == "" || msg == NULL) return;
	msg += "\n";
	log(level, msg.toUtf8().data());
#else
	return;
#endif
}
