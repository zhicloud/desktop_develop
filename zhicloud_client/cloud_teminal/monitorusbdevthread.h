#ifndef __MONITORUSBDEVTHREAD__
#define   MONITORUSBDEVTHREAD__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <QThread>
#include <QString>

#define UEVENT_BUFFER_SIZE 2048


class MonitorUsbDevThread : public QThread
{	
	Q_OBJECT
public:
	MonitorUsbDevThread();
	~MonitorUsbDevThread();
	int init_hotplug_sock();
protected:
	void run();
signals:
	void sendAddOrRemoveUsbDev(bool);
private:
	int m_hotplug_sock;
};
#endif

