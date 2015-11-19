#ifndef win32_spice_interface_h
#define win32_spice_interface_h
/*********************************************************************************************************
 *
 * 说明：此文件为QT提供SPICE通讯接口
 *
 *
 * 创建人：李陈祥
 *
 *
 * 创建时间： 2014-12-04
 *
 *
 * 修改及修改时间：
 *
 *
 * 注意：现在建议只开启一个main_loop，多定义虽然能够运行但是并没有测试其稳定性
 *       生成文件中只有dll文件和def文件，如果需要生成.lib文件 使用命令 
 *       lib.exe /machine:i386 /def:name.def (name 是你的def文件名字，lib.exe在你所安装VC的bin下面)
 *       另外有些文档上说要先执行“sed 's/ @ [0-9]* //g' hello.def” 才能生成可用的lib，但是实际测试
 *       不去掉也是可以的
 *
 *********************************************************************************************************/
#include <stdio.h>

#include <stdbool.h>

#include "win32_spice_interface_types.h"




# ifdef __cplusplus
extern "C"
{
#endif

/*
 * 函数名：SpiceInit
 *
 * 参数：无
 *
 * 返回：SPICE_HANDLE
 *
 * 功能：初始化spice运行环境
 *
 */
SPICE_HANDLE SpiceInit(); 

/*
 * 函数名：SpiceUninit
 *
 * 参数：
 *      sh : SPICE_HANDLE句柄
 *
 * 返回：无
 *
 * 功能：反初始化spice运行环境
 *
 */

void SpiceUninit(IN_OUT SPICE_HANDLE sh);

/*
 * 函数名：SpiceOpenMain
 *
 * 参数：
 *       sh：SPICE_HANDLE句柄
 *       default_ctx：是否启用默认context
 *
 * 返回：
 *      正常返回>=0的整型值
 *      错误返回负值
 *
 * 功能：打开一个main消息句柄
 *
 */

int SpiceOpenMain(IN_OUT SPICE_HANDLE sh);

/*
 * 函数名：SpiceCloseMain
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hmain：对应的mian句柄
 *
 * 返回：
 *      正常返回0
 *      错误返回负值
 *
 * 功能：关闭一个main消息句柄
 *
 */

int SpiceCloseMain(IN_OUT SPICE_HANDLE sh,IN int hmain);

/*
 * 函数名：SpiceRunMainLoop
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hmain：对应的mian句柄
 *      default_ctx：是否使用默认main context
 *
 * 返回：
 *      返回main_loop退出状态
 *
 * 功能：开始一个main loop
 *
 */

int SpiceRunMainLoop(IN_OUT SPICE_HANDLE sh,IN int hmain,bool default_ctx);

/*
 * 函数名：SpiceStopMainLoop
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hmain：对应的mian句柄
 *
 * 返回：
 *      0：正常
 *      <0 ： 见错误码
 *
 * 功能：退出一个main loop
 *
 */

int SpiceStopMainLoop(IN_OUT SPICE_HANDLE sh,IN int hmain);


/*
 * 函数名：SpiceOpen
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *
 * 返回：
 *      正常返回>=0的整型句柄
 *      错误返回负值
 *
 * 功能：开打一个spice connection 句柄
 *
 */

int SpiceOpen(IN_OUT SPICE_HANDLE sh);

/*
 * 函数名：SpiceSetCalls
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      scbs：回调数组
 *      cc：数组个数
 *
 * 返回：
 *
 * 功能：设置回调
 *
 */

void SpiceSetCalls(IN_OUT SPICE_HANDLE sh,IN int hspice,SpiceCallBacks *scbs,int cc);

/*
 * 函数名：SpiceClose
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *
 * 返回：
 *       0：成功
 *       -1：不成功
 *
 * 功能：关闭一个spice connection 句柄
 *
 */

int SpiceClose(IN_OUT SPICE_HANDLE sh,IN int hspice);

/*
 * 函数名：SpiceConnect
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      host：主机名，暂时支持IP
 *      port：端口，
 *      pt：tls协议 端口 (port-tsl)
 *      pw：spice登录密码
 *      cf：cacert.pem 证书文件，对应vv文件中的ca字段(ca_file) (File holding the CA certificates for the host the client is)
 *      cc：cacert.pem 证书中的内容一般情况下与cf只需要设置其中之一(ca_cert) (CA certificates in PEM format. 
 *          The text data can contain several CA certificates identified by:
 *          -----BEGIN CERTIFICATE-----
 *          ... (CA certificate in base64 encoding) ...
 *          -----END CERTIFICATE-----
 *          )
 *          注意：在此接口中去掉此项
 *      cs：证书的主题，对应VV文件中的host-subject字段 (cert-subject)
 *      sound：是否允许播放音频
 *
 *
 * 返回：
 *      0：成功
 *      -1：不成功
 *
 * 功能：根据用户信息连接到KVM虚拟机
 *
 */

int SpiceConnect(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char * host,IN const char * port,IN const char *tp,
      IN const char *pw,IN const char* cf,/*IN_OUT SByteArray *cc,*/IN const char*cs,IN  bool sound);

/*
 * 函数名：SpiceStartSessionFromVvFile 
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      vvFileName：vv文件路径
 *      sound：是否允许播放音频
 *
 * 返回：
 *      0：成功
 *      -1：不成功
 *
 * 功能：根据vv 文件连接到KVM虚拟机
 *
 */

int SpiceStartSessionFromVvFile(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char* vvFileName,IN  bool sound);


/*
 * 函数名：SpiceDisconnect 
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *
 * 返回：
 *
 * 功能：根据SPICE_HANDLE 和 hspice断开KVM虚拟机连接 
 *
 */

void SpiceDisconnect(IN_OUT SPICE_HANDLE sh,IN int hspice);


/*
 * 函数名：SpiceCreateOvirtSession
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      URI：govirt解析地址
 *      user：用户名
 *      pass：密码
 *      sslCaFile：证书文件
 *      sound：是否允许播放音频
 *      sslStrict：是否禁用强行的证书检查
 *
 * 返回：
 *      0：成功
 *      -1：不成功
 *
 * 功能：通过govirt进行连接服务器 
 *
 */

int SpiceCreateOvirtSession(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char* URI, IN const char * user, IN const char *  pass,
      IN const char * sslCaFile,IN bool sound, IN bool sslStrict);

/*
 * 函数名：SpiceFetchVmNames
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      URI：govirt解析地址
 *      user：用户名
 *      pass：密码
 *      sslCaFile：证书文件
 *      sound：是否允许播放音频
 *      sslStrict：
 * 返回：
 *      0：成功
 *      -1：失败
 *
 * 功能：获取Vm虚拟机名称 
 *
 */

int SpiceFetchVmNames(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char *URI, IN const char* user, IN const char* password,
      IN const char * sslCaFile,IN bool sslStrict);


/*
 * 函数名：SpiceRequestResolution
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      x：鼠标可移动最大x坐标
 *      y：鼠标可移动最大y坐标
 * 返回：
 *
 * 功能：处理鼠标可移动坐标处理 
 *
 */

void SpiceRequestResolution(IN_OUT SPICE_HANDLE sh,IN int hspice,IN int x, IN int y); 

/*
 * 函数名：SpiceKeyEvent
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      down：是否按键被按下
 *      hardware_keycode：键盘按下的硬件码(VK_BACK VK_TAB等)
 * 返回：
 *
 * 功能：主要处理键盘按键 
 *
 */

void SpiceKeyEvent(IN_OUT SPICE_HANDLE sh,IN int hspice,IN bool down, IN int hardware_keycode);

/*
 * 函数名：SpiceButtonEvent
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      x：鼠标横坐标
 *      y：鼠标纵坐标
 *      metaState：暂时无用
 *      type：鼠标按键(最高位表示按键是按下还是释放,其他位表示LCXSpiceMouseButton)
 * 返回：
 *
 * 功能：主要处理鼠标移动和鼠标按键消息 
 *
 */

void SpiceButtonEvent(IN_OUT SPICE_HANDLE sh,IN int hspice,IN int  x, IN int y, IN int metaState, IN int type); 
/*
 * 函数名：SpiceEnableAutoUSBRedirectBefore
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      enable：是否允许自动重定向
 * 返回：
 *
 * 功能：开启自动重定向功能 
 *
 */

void SpiceEnableAutoUSBRedirectBefore(IN_OUT SPICE_HANDLE sh,IN int hspice,IN int enable); 

/*
 * 函数名：SpiceEnableAutoUSBRedirect
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      enable：是否允许自动重定向
 * 返回：
 *
 * 功能：开启自动重定向功能 
 *
 */

void SpiceEnableAutoUSBRedirect(IN_OUT SPICE_HANDLE sh,IN int hspice,IN int enable); 

/*
 * 函数名：SpiceSetUSBFilter
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      filter：过滤字符串，格式 rule = @class,@vendor,@product,@version,@allow  ,  filter = rule1 | rule2 | rule3
 * 返回：
 *
 * 功能：设置USB插入时USB重定向过滤器
 *
 */

void SpiceSetUSBFilter(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char * filter); 


/*
 * 函数名：SpiceSetUSBFilterBefore
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      filter：过滤字符串，格式 rule = @class,@vendor,@product,@version,@allow  ,  filter = rule1 | rule2 | rule3
 * 返回：
 *
 * 功能：设置USB插入时USB重定向过滤器
 *
 */


void SpiceSetUSBFilterBefore(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char * filter);
/*
 * 函数名：SpiceSetUSBRedirOnConnectFilterBefore
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      filter：过滤字符串，格式 rule = @class,@vendor,@product,@version,@allow  ,  filter = rule1 | rule2 | rule3
 * 返回：
 *
 * 功能：设置已经插入USB口的USB设备过滤器
 *
 */

void SpiceSetUSBRedirOnConnectFilterBefore(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char * filter); 

/*
 * 函数名：SpiceSetUSBRedirOnConnectFilter
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      filter：过滤字符串，格式 rule = @class,@vendor,@product,@version,@allow  ,  filter = rule1 | rule2 | rule3
 * 返回：
 *
 * 功能：设置已经插入USB口的USB设备过滤器
 *
 */

void SpiceSetUSBRedirOnConnectFilter(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char * filter); 

/*
 * 函数名：SpiceUSBRedirect
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      dev：USB设备描述，上层不需要知道具体格式
 * 返回：
 *
 * 功能：重定向dev指向的usb设备 
 *
 */

void SpiceUSBRedirect(IN_OUT SPICE_HANDLE sh,IN int hspice,IN void * dev); 

/*
 * 函数名：SpiceSetUSBDisconnect
 *
 * 参数：
 *      sh：SPICE_HANDLE句柄
 *      hspice：对应的spice连接句柄
 *      dev：USB设备描述，上层不需要知道具体格式
 * 返回：
 *
 * 功能：断开dev指向的usb设备 
 *
 */

void SpiceSetUSBDisconnect(IN_OUT SPICE_HANDLE sh,IN int hspice,IN void * dev); 

# ifdef __cplusplus
}
#endif

#endif
