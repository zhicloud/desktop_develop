#ifndef RED_ENCODING_DISPATCHER_H
#define RED_ENCODING_DISPATCHER_H

/*********************************************************************************************************
 * 
 * 说明：命令分发与回调消息注册
 *
 * 创建人：李陈祥
 *
 * 创建时间： 2015-12-01
 *
 * 修改及修改时间：
 *                2015-12-01 :初次创建文件，并实现基本代码
 *
 *
 ********************************************************************************************************/



#include "dispatcher.h"
#include "common/rect.h"
#include "common/region.h"
#include "common/ring.h"
#include "red_encoding_threading.h"
#include <spice.h>

typedef enum
{
    ENCODING_DISPATCHER_DATA_READY_CB = 0,
    ENCODING_DISPATCHER_GET_SURFACE_CB,
    ENCODING_DISPATCHER_CB_NUM
};


typedef struct EncodingDispatcherDataReadyMessage {
    int ready;
} EncodingDispatcherDataReadyMessage;

typedef struct EncodingDispatcherGetSurfaceMessage {
   int count;
} EncodingDispatcherGetSurfaceMessage;

void encoding_dispatcher_channel_event(int event, SpiceChannelEventInfo *info);

void encoding_dispatcher_init(SpiceCoreInterface *core,void * data_ready_cb,void* worker);




#endif
