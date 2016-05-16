#include <QStringList>
#include "monitorusbdevthread.h"

MonitorUsbDevThread::MonitorUsbDevThread()
{
	m_hotplug_sock = init_hotplug_sock();
}
MonitorUsbDevThread::~MonitorUsbDevThread()
{
}

int MonitorUsbDevThread::init_hotplug_sock()
{
		const int buffersize = 1024;
		int ret;
		struct sockaddr_nl snl;
		bzero(&snl, sizeof(struct sockaddr_nl));
		snl.nl_family = AF_NETLINK;
		snl.nl_pid = 0;//getpid();
		snl.nl_groups = 1;

		int s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
		if(s == -1){
				perror("socket");
				return -1;
		}
		int on = 1;
		ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR/*SO_RCVBUF*/, &on/*&buffersize*/, sizeof(on/*buffersize*/));
		if(ret < 0){
				perror("setsockopt failed");
				return -1;
		}
		ret = bind(s, (struct sockaddr *)&snl, sizeof(struct sockaddr_nl));
		if(ret < 0){
				perror("socket_bind");
				close(s);
				return -1;
		}
		return s;
}
void MonitorUsbDevThread::run()
{
	if(m_hotplug_sock == -1){
			return;
	}
	while(1){
		char buf[UEVENT_BUFFER_SIZE * 2] = {0};
		recv(m_hotplug_sock, &buf, sizeof(buf), 0);
		//printf("%s\n", buf);
		QString usbmsg(buf);
		QStringList msglist = usbmsg.split("@");
		if(!msglist.isEmpty()){
			if(msglist.at(0) == QString("add")){
				emit sendAddOrRemoveUsbDev(true);
				//printf("add usb dev\n");
			}else{
				emit sendAddOrRemoveUsbDev(false);
				//printf("remove usb dev\n");
			}
		}
	}
}

