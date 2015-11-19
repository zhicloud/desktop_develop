#include "mylog.h"
void ZCLOG(QString log)
{
		QDateTime time = QDateTime::currentDateTime();//获取系统现在的时间
		QString str = time.toString("yyyy-MM-dd hh:mm:ss ddd");
		QString tmp = log + QString(" ") + str;
		QByteArray b = tmp.toLatin1();
		FILE *fp = fopen("/home/json_result.txt", "a+");
		if (NULL != fp)
		{
			fprintf(fp, "%s\n", b.data());
			fclose(fp);
			fp = NULL;
		}
}
