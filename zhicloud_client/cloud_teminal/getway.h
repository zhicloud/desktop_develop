#ifndef GETWAY_H
#define GETWAY_H

 #include <arpa/inet.h>  //for in_addr
 #include <linux/rtnetlink.h>    //for rtnetlink
 //标签：Linux系统 C++ 获取网络接口，以及主机网关IP， 网络,网关,IP
 #include <net/if.h> //for IF_NAMESIZ, route_info
#include <stdio.h>
#include <stdlib.h> //for malloc(), free()
#include <string.h> //for strstr(), memset()
#include <string>
#include <QNetworkInterface>
#include <QList>
#include <sys/types.h>
#include <unistd.h>

#include  <fcntl.h>
#include  <errno.h>
#include  <sys/ioctl.h>
#include  <sys/socket.h>


typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char u8;
#include  <linux/sockios.h>
#include  <linux/ethtool.h>
using namespace std;

#define BUFSIZE 8192

struct route_info
{
   u_int dstAddr;
   u_int srcAddr;
   u_int gateWay;
   char ifName[IF_NAMESIZE];
//标签：Linux系统 C++ 获取网络接口，以及主机网关IP， 网络,网关,IP
};
int get_netlink_status(const char *if_name);
int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId);
void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo,char *gateway, char *ifName);//分析返回的路由信息
int get_gateway(char *gateway, char *ifName);
bool getmaskAddress(QString &ip, QString &netmask,QString &mac);
bool ConfigNetwork(QString type,QString ip="",QString netmask="",QString gateway="",QString broadcast="",QString dns="");


#endif
