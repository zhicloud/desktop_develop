#ifndef win32_spice_interface_types_h
#define win32_spice_interface_types_h

/*********************************************************************************************************
 *
 * 说明：通用结构体定义
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
 *
 *********************************************************************************************************/

#ifndef IN
#define IN
#endif 

#ifndef OUT
#define OUT
#endif 

#ifndef IN_OUT
#define IN_OUT
#endif 


typedef void* SPICE_HANDLE  ;
#define PTRFLAGS_DOWN 0x8000


/*****
 * 错误号定义
 */
#define ERROR_OK   0       //无措
#define ERROR_MAX -1       // 已经达到可分配的最大个数
#define ERROR_INTERNAL -2  // 内部错误
#define ERROR_NOT_EXIST -3 // 传入句柄不存在
#define ERROR_NO_MEMORY -4 //内存不足
#define ERROR_HANDLE -5     //句柄错误
#define ERROR_RESOURCE -6  //资源不足

/*
 * 名称：SByteArray
 *
 * 成员：
 *       data：byte数组指针
 *       len：数组长度
 * 描述：
 *       定义二进制数组结构体
 */

typedef struct SByteArray
{
   unsigned char *data;
   unsigned int    len;

}SByteArray;

/*
 * 名称：LCXCALLBACK
 *
 * 成员：
 *       _cb：回调函数原型
 *       _ctx：用户数据指针
 *       _ud：用户回传数据
 * 描述：
 *       对回调函数的通用定义
 */

typedef struct LCXCALLBACK
{
   void* (*_cb)(void* ctx,void * ud); 
   void* _ctx;
   void* _ud;
}LCXCALLBACK;

/*
 * 名称：LcxSpiceChannelEvent
 *
 * 成员：
 *       LCX_SPICE_CHANNEL_NONE：空事件，作为通知使用
 *       LCX_SPICE_CHANNEL_OPENED：main channel opened
 *       LCX_SPICE_CHANNEL_SWITCHING：main channel: switching host
 *       LCX_SPICE_CHANNEL_CLOSED：main channel: closed
 *       LCX_SPICE_CHANNEL_ERROR_CONNECT：连接出错
 *       LCX_SPICE_CHANNEL_ERROR_TLS：TLS出错
 *       LCX_SPICE_CHANNEL_ERROR_LINK：连接出错
 *       LCX_SPICE_CHANNEL_ERROR_AUTH：认证出错
 *       LCX_SPICE_CHANNEL_ERROR_IO：IO出错
 * 描述：
 *       对main channel事件类型进行定义
 */

typedef enum
{
    LCX_SPICE_CHANNEL_NONE = 0,
    LCX_SPICE_CHANNEL_OPENED = 10,
    LCX_SPICE_CHANNEL_SWITCHING,
    LCX_SPICE_CHANNEL_CLOSED,
    LCX_SPICE_CHANNEL_ERROR_CONNECT = 20,
    LCX_SPICE_CHANNEL_ERROR_TLS,
    LCX_SPICE_CHANNEL_ERROR_LINK,
    LCX_SPICE_CHANNEL_ERROR_AUTH,
    LCX_SPICE_CHANNEL_ERROR_IO,
} LcxSpiceChannelEvent; 


/*
 * 名称：CBTypes
 *
 * 成员：
 *       CB_SETTING_CHANGE：分辨率出现变化
 *       CB_INVALIDATE：界面重绘
 *       CB_CURSOr_SET：设置光标
 *       CB_CURSOR_MOVE：鼠标移动事件
 *       CB_CURSOR_HIDE：鼠标隐藏事件
 *       CB_CURSOR_RESET：鼠标重置事件
 *       CB_USB_CONNECT_FAILD：USB连接失败
 *       CB_USB_REMOVE：USB设备被拔出
 *       CB_USB_AUTO_CONNECT_FAILD：USB自动连接失败
 *       CB_USB_DEVICE_ERROR：USB设备错误
 *       CB_USB_ADD_DEV：USB设备插入事件
 *       CB_MAIN_CHANNEL_EVENT：main channel事件
 *       CB_NULL：空事件，没有定义
 * 描述：
 *       对回调函数类型进行定义
 */

typedef enum CBTypes{
   CB_SETTING_CHANGE = 0,
   CB_INVALIDATE,
   CB_CURSOr_SET,
   CB_CURSOR_MOVE,
   CB_CURSOR_HIDE,
   CB_CURSOR_RESET,
   CB_USB_CONNECT_FAILD,
   CB_USB_REMOVE,
   //CB_USB_AUTO_CONNECT_FAILD,
   CB_USB_DEVICE_ERROR,
   CB_USB_ADD_DEV,
   CB_MAIN_CHANNEL_EVENT,
   CB_NULL
}CBTypes; 

/*
 * 名称：LCXSpiceMouseButton
 *
 * 成员：
 *       LCX_SPICE_MOUSE_BUTTON_INVALID 鼠标按键不可用
 *       LCX_SPICE_MOUSE_BUTTON_LEFT 鼠标左键
 *       LCX_SPICE_MOUSE_BUTTON_MIDDLE 鼠标中间件
 *       LCX_SPICE_MOUSE_BUTTON_RIGHT 鼠标右键
 *       LCX_SPICE_MOUSE_BUTTON_UP 向上滚动
 *       LCX_SPICE_MOUSE_BUTTON_DOWN 向下滚动
 *       LCX_SPICE_MOUSE_BUTTON_ENUM_END 鼠标按键结束
 * 描述：
 *       对回调函数类型进行定义
 */

typedef enum LCXSpiceMouseButton {
    LCX_SPICE_MOUSE_BUTTON_INVALID,
    LCX_SPICE_MOUSE_BUTTON_LEFT,
    LCX_SPICE_MOUSE_BUTTON_MIDDLE,
    LCX_SPICE_MOUSE_BUTTON_RIGHT,
    LCX_SPICE_MOUSE_BUTTON_UP,
    LCX_SPICE_MOUSE_BUTTON_DOWN,

    LCX_SPICE_MOUSE_BUTTON_ENUM_END
} LCXSpiceMouseButton;


/*
 * 名称：SPICE_SettringChange
 *
 * 成员：
 *       width：长
 *       height：宽
 *       bpp：bpp值
 * 描述：
 *       分辨率重置结构体
 */
typedef struct SPICE_SettringChange
{
   int width;
   int height;
   int bpp;
}SPICE_SettringChange;

/*
 * 名称：SPICE_Invalidate
 *
 * 成员：
 *       x：x坐标
 *       y：y坐标
 *       w：长
 *       h：宽
 *       bitmap：图片内容
 * 描述：
 *       界面重绘
 */
typedef struct SPICE_Invalidate
{
   int x;
   int y;
   int w;
   int h;
   unsigned char * bitmap;
   void (*free_bitmap)(void* p);
}SPICE_Invalidate;

/*
   暂时只定义这些结构和结构体内容，需要时补充其他的
   ...................
   ...................
   ...................
 */

/*
 * 名称：SPICE_CursorSet
 *
 * 成员：
 *       width：长
 *       height：宽
 *       hot_spot_x：横坐标
 *       hot_spot_y：纵坐标
 *       data：鼠标数据
 * 描述：
 *       重新设置鼠标形状
 */

typedef struct SPICE_CursorSet
{
   int width;
   int height;
   int hot_spot_x;
   int hot_spot_y;
   unsigned char *data;

}SPICE_CursorSet;

/*
 * 名称：SPICE_CursorMove
 *
 * 成员：
 *       x：横坐标
 *       y：纵坐标
 * 描述：
 *       控制鼠标移动
 */

typedef struct SPICE_CursorMove
{
   int x;
   int y;
}SPICE_CursorMove;

/*
 * 名称：SPICE_CursorHide
 *
 * 成员：
 * 描述：
 *       控制鼠标隐藏
 */

#if 0
typedef struct SPICE_CursorHide
{

}SPICE_CursorHide;
#endif 

/*
 * 名称：SPICE_CursorReset
 *
 * 成员：
 * 描述：
 *       控制鼠标隐藏
 */
#if 0
typedef struct SPICE_CursorReset
{

}SPICE_CursorReset;
#endif


/*
 * 名称：SPICE_USBConnectFaild
 *
 * 成员：
 *       err_message：错误消息描述 
 * 描述：
 *       USB连接失败
 */

typedef struct SPICE_USBConnectFaild
{
    char err_message[1024]; 

}SPICE_USBConnectFaild;

/*
 * 名称：SPICE_USBRemove
 *
 * 成员：
 *       device：设备相关描述
 * 描述：
 *       移除USB设备
 */

typedef struct SPICE_USBRemove
{
    void * device;

}SPICE_USBRemove;

/*
 * 名称：SPICE_USBAutoConnFaild
 *
 * 成员：
 * 描述：
 *       USB设备连接错误
 */

#if 0
typedef struct SPICE_USBAutoConnFaild
{

}SPICE_USBAutoConnFaild;
#endif


/*
 * 名称：SPICE_USBDeviceErr
 *
 * 成员：
 *       err_message：错误消息描述 
 * 描述：
 *       USB设备错误
 */

typedef struct SPICE_USBDeviceErr
{
    char err_message[1024];

}SPICE_USBDeviceErr;

/*
 * 名称：SPICE_USBAddDev
 *
 * 成员：
 *       device：设备相关描述
 *       desc：设备信息，字符串形式
 *       is_connect：设备是否已经连接
 * 描述：
 *       增加USB设备
 */

typedef struct SPICE_USBAddDev
{
    void * device;
    char desc[1024];
    int is_connect;
}SPICE_USBAddDev;
 


/*
 * 名称：SPICE_MainChannelEvent
 *
 * 成员：
 *       et：信息号
 * 描述：
 *       main channel msg
 */


typedef struct SPICE_MainChannelEvent
{
   LcxSpiceChannelEvent et;
}SPICE_MainChannelEvent;


/*
 * 名称：SpiceCallBacks
 *
 * 成员：
 *       type：定义函数指针类型
 *       lcb：函数回调结构体
 * 描述：
 *       定义spice 回调类型结构体
 */

typedef struct SpiceCallBacks
{
   CBTypes type;
   LCXCALLBACK lcb;
}SpiceCallBacks;


#endif
