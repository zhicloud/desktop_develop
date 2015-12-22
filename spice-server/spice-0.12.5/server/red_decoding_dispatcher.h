#ifndef RED_DECODING_DISPATCHER_H
#define RED_DECODING_DISPATCHER_H

#include "dispatcher.h"
#include "common/rect.h"
#include "common/region.h"
#include "common/ring.h"
typedef struct DecodingDispatcher DecodingDispatcher;
struct DecodingDispatcher
{
    Dispatcher dispatcher;
    
    uint32_t pending;
    DecodingDispatcher * next;
    Ring async_commands;
    pthread_mutex_t  async_lock;
};




#endif
