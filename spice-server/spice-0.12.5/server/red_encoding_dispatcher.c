
#include "red_encoding_dispatcher.h"
#include "red_spice_common_defines.h"


typedef struct EncodingDispatcher EncodingDispatcher;
struct EncodingDispatcher
{
    Dispatcher dispatcher;
    
    uint32_t pending;
    SpiceCoreInterface *core;
    void * main_worker;
    LCXCALLBACK data_ready_cb[ENCODING_DISPATCHER_CB_NUM];
};

enum {
    ENCODING_DISPATCHER_DATA_READY_EVENT = 0,
    ENCODING_DISPATCHER_GET_SURFACE_EVENT,

    ENCODING_DISPATCHER_NUM_MESSAGES
};

EncodingDispatcher encoding_dispatcher;



void encoding_dispatcher_data_ready_event(int ready)
{
    //spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : begin",__FILE__,__FUNCTION__,__LINE__);
    EncodingDispatcherDataReadyMessage msg = {0,};

    msg.ready = ready;
    dispatcher_send_message(&encoding_dispatcher.dispatcher,ENCODING_DISPATCHER_DATA_READY_EVENT,
                            &msg);
    //spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : end",__FILE__,__FUNCTION__,__LINE__);
}

void encoding_dispatcher_get_surface_event(int count)
{
    EncodingDispatcherGetSurfaceMessage msg = {0,};
    msg.count = count;
    dispatcher_send_message(&encoding_dispatcher.dispatcher,ENCODING_DISPATCHER_GET_SURFACE_EVENT,
                            &msg);
}

static void dispatcher_handle_read(int fd, int event, void *opaque)
{
    //Dispatcher *dispatcher = opaque;
    dispatcher_handle_recv_read(&encoding_dispatcher.dispatcher);
}

static void encoding_dispatcher_handle_data_ready_event(void *opaque,
                                                 void *payload)
{
    //spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : begin",__FILE__,__FUNCTION__,__LINE__);
    EncodingDispatcherDataReadyMessage *data_ready_event = payload;
    EncodingDispatcher * handle_encoding_dispatcher = (EncodingDispatcher *)opaque;
    LCXCALLBACK * data_ready_cb = &handle_encoding_dispatcher->data_ready_cb[ENCODING_DISPATCHER_DATA_READY_CB];

    if (data_ready_cb->_cb != NULL)
    {
        //spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 11111111111111111111111111111",__FILE__,__FUNCTION__,__LINE__);
        data_ready_cb->_ud = payload;
        data_ready_cb->_cb(data_ready_cb->_ctx,data_ready_cb->_ud);
    }
}

static void encoding_dispatcher_handle_get_surface_event(void *opaque,
                                                 void *payload)
{
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : begin",__FILE__,__FUNCTION__,__LINE__);
    EncodingDispatcherGetSurfaceMessage * msg = payload;
    EncodingDispatcher * handle_encoding_dispatcher = (EncodingDispatcher *)opaque;
    LCXCALLBACK * handle_cb = &handle_encoding_dispatcher->data_ready_cb[ENCODING_DISPATCHER_GET_SURFACE_CB];
    
    if (handle_cb->_cb != NULL)
    {
        handle_cb->_ud = payload;
        handle_cb->_cb(handle_cb->_ctx,handle_cb->_ud);
    }

}

static void encoding_handle_dispatcher_async_done(void *opaque,
                                                uint32_t message_type,
                                                void *payload)
{
    switch(message_type)
    {
        case ENCODING_DISPATCHER_GET_SURFACE_EVENT:
            {
                EncodingDispatcher * handle_encoding_dispatcher = (EncodingDispatcher *)opaque;
                LCXCALLBACK * handle_cb = &handle_encoding_dispatcher->data_ready_cb[ENCODING_DISPATCHER_GET_SURFACE_CB];
                RedEncodingThreading * threading = (RedEncodingThreading *)handle_cb->_ctx; 
                threading->surface_ready_encoding_threading(threading);
            }    
            break;
        default:
            break;
    }
}


void encoding_dispatcher_init(SpiceCoreInterface *core,void * data_ready_cb,void* worker)
{
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : begin",__FILE__,__FUNCTION__,__LINE__);
    memset(&encoding_dispatcher, 0, sizeof(encoding_dispatcher));
    encoding_dispatcher.core = core;
    encoding_dispatcher.main_worker = worker;
    
    memcpy(encoding_dispatcher.data_ready_cb,data_ready_cb,sizeof(LCXCALLBACK) * ENCODING_DISPATCHER_CB_NUM);


    dispatcher_init(&encoding_dispatcher.dispatcher, ENCODING_DISPATCHER_NUM_MESSAGES, &encoding_dispatcher);
    core->watch_add(encoding_dispatcher.dispatcher.recv_fd, SPICE_WATCH_EVENT_READ,
                    dispatcher_handle_read, worker);
    
    dispatcher_register_async_done_callback(
                                    &encoding_dispatcher.dispatcher,
                                    encoding_handle_dispatcher_async_done);

    dispatcher_register_handler(&encoding_dispatcher.dispatcher, ENCODING_DISPATCHER_DATA_READY_EVENT,
                                encoding_dispatcher_handle_data_ready_event,
                                sizeof(EncodingDispatcherDataReadyMessage), DISPATCHER_NONE /* no ack */);
    
    //sync get surface data
    dispatcher_register_handler(&encoding_dispatcher.dispatcher, ENCODING_DISPATCHER_GET_SURFACE_EVENT,
                                encoding_dispatcher_handle_get_surface_event,
                                sizeof(EncodingDispatcherGetSurfaceMessage), DISPATCHER_ASYNC /* sync */);
}

