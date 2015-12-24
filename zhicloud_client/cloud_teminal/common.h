#ifndef __COMMON_H__
#define __COMMON_H__
#include <QString>
#include <QMutex>
#include <QWaitCondition>

#define LOG_MSG_LEN		256									//日志字符串的长度
const int MAX_HOST_NUM = 256;

//定义日志的等级
enum LOG_LEVEL
{
	ZCFATAL = 0,		//致命错误
	ZCERROR,			//错误
	ZCWARN,				//警告
	ZCINFO,				//提示信息
	ZCDEBUG				//调试		
};

struct Connections{
	char displayName[32];
	char displayPassword[32];
	char externalAddress[32];
	char internalAddress[32];
};

struct ControlServer{
	char externalAddress[32];
	char internalAddress[32];
};

struct Ports 
{
	int protocol;//协议:0所有协议;1TCP;2DUP
	char port[8];//需要开放的端口
	char server_port[8];//宿主机端口
	char outer_port[8];//映射到公网的端口
};

struct HostMessage
{
	char id[64];//唯一标识
	char host_name[256];//主机名
	int type;//类型：1运营商自用;2代理商自用;3终端用户自用
	char account[32];
	char password[32];
	int cpu_core;//cpu核数数量
	int cpu_usage;
	int memory;//内存
	int memory_usage;
	int sys_disk;//系统磁盘容量
	int sys_disk_usage;
	int data_disk;//数据磁盘容量
	int data_disk_usage;
	int bandwidth;//网络带宽
	int is_auto_startup;//是否自动启动:1是;2否
	int running_status;//运行状态:1关机;2启动
	int status;//状态:1正常;2停机;3欠费
	int region; // 1: 广州,2：成都,3：北京,4：香港
	char inner_ip[32];//内网监控地址IP
	char inner_port[8];//内网监控地址端口
	char outer_ip[32];//公网监控地址IP
	char outer_port[8];//公网健康地址端口
	char sysImageName[32]; /*系统名称 要模糊匹配 windows7 windows2003 windows2008 windows2012 centos */
	char inactivate_time[32]; //停机时间，格式为yyyyMMddHHmmssSSS

	int portsNum;
	Ports ports[1];//每个主机的端口个数待定
};

//验证后返回的数据，用结构体接收
struct LoginAuth{
	char status[16];
	char message[256];
	int connectionsNum;
	ControlServer server;
	Connections connections[1];
};

//云主机信息
struct TotalHostMessage
{
	char status[16];
	char message[256];
	int hostNum;
	HostMessage hostmsg[1];//云主机个数待定
	//HostMessage hostmsg[MAX_HOST_NUM];
};

//操作类型
enum ZcCommandType
{
	Auth = 1,					//鉴权操作
	Login = 2,
	GetHostInfo = 3,			//获取云主机信息
	UpdateHostInfo = 4,			//更新云主机信息
	StartHost = 5,				//开启云主机
	StopHost = 6,				//关闭云主机
	RestartHost = 7,			//重启云主机
	ModifyHostConfig = 8,		//修改云主机配置
	ForceStopHost = 9,			//强制关闭云主机
	QueryHostPrice = 10,			//查询主机修改配置时价格
	QueryLastVersion = 11,		//查询最新版本号
	FirstQueryHostPrice = 12,	//每次进入配置页面查询云主机价格
	UpdateHostInfo2 = 13,		//进程一直更新云主机信息
	ChangePWD = 14,					//	修改密码
	HeartBeat = 15,
	LogOut,
};

//操作云主机返回的数据
struct ControlHostMsg{
	char status[16];
	char message[256];
};

enum RETURNCMD{
	INVALIDPASSWORD = 1,
	UNKNOWURI,
	NORMALCLOSE,
	VIRTSTARTSUCC,
};
struct remote_viewer_data
{
	char ip_str[64];
	char control_password[32];
};
//获取云主机当前配置的价格
struct Price{
	double monthlyPrice;	//当前配置一个月的价格
	double balance;			//当前的余额
};
struct HostPrice{
	char status[16];
	char message[256];
	Price price;
};
/*
模糊匹配如下字符串
windows7
windows2003
windows2008
windows2012
centos
*/
enum SysImageType{
	SysUnknow = -1,
	SysWindows7 = 1,
	SysWindows2003 = 2,
	SysWindows2008 = 3,
	SysWindows2012 = 4,
	SysCentos =5,
	SysCentos6_4 = 6,
	SysCentos6_5 = 7,
	SysUbuntu =8,
	SysUbuntu12_04 = 9,
	SysUbuntu14_04 = 10

};
struct Versions
{
	char updateinfo[2024];//更新信息日志内容
	char name[64]; //最新版本号
	char path[256]; //最新软件包地址
	//char update_process_address[256];//更新程序地址

};
struct ProgressVersion
{
	char status[16];
	char message[256];
	Versions versions;
};

#endif
