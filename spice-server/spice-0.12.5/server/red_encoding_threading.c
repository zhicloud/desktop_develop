#include "red_encoding_threading.h"
#include "h264_type.h"
#include "red_encoding_dispatcher.h"

#include <spice/protocol.h>
#include <errno.h>
#include<unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#define RED_SPICE_ENCODING_THREADING_INIT(threading) do{\
   RED_SPICE_BASE_THREADING_INIT(threading->tbase);\
   threading->init_spice_encoding_threading = init_spice_encoding_threading;\
   threading->set_spice_encoding_threading_run = set_spice_encoding_threading_run;\
   threading->start_spice_encoding_threading = start_spice_encoding_threading;\
   threading->stop_wait_spice_encoding_threading = stop_wait_spice_encoding_threading;\
   threading->copy_bits_to_encoding_threading = copy_bits_to_encoding_threading;\
   threading->insert_h264_slice_in_encoding_threading = insert_h264_slice_in_encoding_threading;\
   threading->remove_h264_slice_from_encoding_threading = remove_h264_slice_from_encoding_threading;\
   threading->uninit_spice_encoding_threading = uninit_spice_encoding_threading;\
   threading->get_spice_encoding_threading_status = get_spice_encoding_threading_status;\
   threading->read_bits_with_cb_to_encoding_threading = read_bits_with_cb_to_encoding_threading;\
   threading->surface_ready_encoding_threading = surface_ready_encoding_threading;\
   threading->insert_vedio_data_in_encoding_threading = insert_vedio_data_in_encoding_threading;\
}while(0)



void init_spice_encoding_threading(RedEncodingThreading * threading,int bits_size,SpiceRect *bbox,
        LCXCALLBACK *call_sending,LCXCALLBACK *get_bits,LCXCALLBACK *capture_surface,enum AVPixelFormat pix_fmt,int fr,int br);
void set_spice_encoding_threading_run(RedEncodingThreading * threading,void* (*start_rtn)(void*));
void start_spice_encoding_threading(RedEncodingThreading * threading);
void stop_wait_spice_encoding_threading(RedEncodingThreading * threading);
void copy_bits_to_encoding_threading(RedEncodingThreading * threading,char* bits,int size,SpiceRect *bbox);
void insert_h264_slice_in_encoding_threading(RedEncodingThreading * threading,H264DataItem * dataitem);
H264DataItem * remove_h264_slice_from_encoding_threading(RedEncodingThreading * threading);
void uninit_spice_encoding_threading(RedEncodingThreading * threading);
void read_bits_with_cb_to_encoding_threading(RedEncodingThreading* threading,SpiceRect *bbox);
RedBaseThreadingStatus get_spice_encoding_threading_status(RedEncodingThreading * threading);
void surface_ready_encoding_threading(RedEncodingThreading * threading);
void insert_vedio_data_in_encoding_threading(RedEncodingThreading* threading,char* viedo_data);


static H264DataItem * get_h264_slice_data_from_encoding_threading(RedEncodingThreading * threading,VedioDataItem *dataitem)
{

    int width, height;
    int ret;
    int bpp;
    //pthread_mutex_lock(&threading->_lock);
    if (!threading->g_h264_encoder)
    {
        spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : %s",__FILE__,__FUNCTION__,__LINE__,"g_h264_encoder === NULL");
        return NULL;
        //goto down;
    }

    width = dataitem->vedio_info.bbox.right - dataitem->vedio_info.bbox.left;
    height = dataitem->vedio_info.bbox.bottom - dataitem->vedio_info.bbox.top;

    if (threading->g_h264_encoder->codec_context->width != width || 
            threading->g_h264_encoder->codec_context->height != height) 
    {
        spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : \
                width (%d,%d) , height ( %d ,%d)",
                __FILE__,__FUNCTION__,__LINE__,
                width,
                threading->g_h264_encoder->codec_context->width, 
                height,
                threading->g_h264_encoder->codec_context->height
                );
        return NULL;
        //goto down;
    }

    GET_FORMAT_BYTES(dataitem->vedio_info.pix_fmt,bpp) ;


    H264DataItem *h264_dataitem = (H264DataItem *)malloc(sizeof(char) * sizeof(H264DataItem));
    memset(h264_dataitem,0,sizeof(H264DataItem));
    h264_dataitem->slice = (char *)malloc(sizeof(char) * width * height * bpp );
    h264_dataitem->size = width * height * bpp;
    

    ret = h264_encode(threading->g_h264_encoder,dataitem->bits,width * height * bpp,
            h264_dataitem->slice,&h264_dataitem->size);
    
    if (H264_NOT_GET_SLICE == ret)
    {


    }
    else if(H264_ENCODER_OK == ret)
    {
#if 0
        char filename[1000] = "";
        sprintf(filename,"/home/lichenxiang/stream_before/h264_0x%xh264",threading->g_h264_encoder);
        FILE *fp = fopen(filename,"a");
        fwrite(h264_dataitem->slice,1,h264_dataitem->size,fp);
        fclose(fp);
#endif
        uint64_t time_now = red_now();
        spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : =================  %ld.%ld ========================",
                __FILE__,__FUNCTION__,__LINE__,
                time_now / (1000*1000*1000),time_now % (1000*1000*1000));
        return h264_dataitem; 
    }
    else 
    {
        spice_printerr("h264_encode error");
    }
    
    free_h264_data_item(h264_dataitem);
    return NULL;
//down:
//    pthread_mutex_unlock(&threading->_lock);
}

static void do_callback(RedEncodingThreading * threading)
{
    threading->call_sending._ud = NULL;
    threading->call_sending._cb(threading->call_sending._ctx,threading->call_sending._ud);
}

static void * start_run(void * args)
{
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : begin",__FILE__,__FUNCTION__,__LINE__);
    RedEncodingThreading * threading = (RedEncodingThreading * )args;
    int ret = 0;
    struct timespec  abstime;

    while(threading->tbase->get_spice_base_threading_close(threading->tbase) == false)
    {
        //clock_gettime(CLOCK_MONOTONIC, &abstime);
        abstime.tv_sec =  time(NULL) + 30;
        abstime.tv_nsec = 0;

        uint64_t time_now = red_now();
        spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : =================  %ld.%ld ========================",
                __FILE__,__FUNCTION__,__LINE__,
                time_now / (1000*1000*1000),time_now % (1000*1000*1000));
        
        ret = event_timedwait(threading->_ev,&abstime);
        if (threading->tbase->get_spice_base_threading_close(threading->tbase))
        {
            break;
        }

        if (ret == 0)
        {
            while(1)
            {
                VedioDataItem *dataitem = NULL;
                bool flag = threading->tcollection->remove_vedio_data_form_collection_threading(threading->tcollection,
                        &dataitem);
                if (!flag)
                {
                    break;
                }
                H264DataItem * h264_data_item = NULL;
                h264_data_item = get_h264_slice_data_from_encoding_threading(threading,dataitem);
                if (h264_data_item)
                {
                    free_vedio_data_item(dataitem);
                    threading->insert_h264_slice_in_encoding_threading(threading,h264_data_item);
                    do_callback(threading);
                }
            }

        }
        else if (ret == ETIMEDOUT)
        {
            continue;
        }
        else
        {
            spice_error("event_timedwait error");
        }
    }
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : end",__FILE__,__FUNCTION__,__LINE__);

    return 0Xdead;
}

static void create_spice_encoding_threading_h264_streaming(H264Encoder* h264_encoder,enum AVPixelFormat pix_fmt,
        int width,int height,int fr,int br)
{
    uint32_t frame_mm_time;
    int n;

    H264StreamInfo info;
    AVRational rate = { 1, fr};
    info.bit_rate = br; 
    info.rc_max_rate = info.bit_rate;
    info.rc_min_rate = info.bit_rate / 2;
    info.rc_buffer_size = RC_BUFFER_SIZE_FF;

    int n_pixel_bits; 

    //info.s_pix_fmt = AV_PIX_FMT_BGRA;
    info.s_pix_fmt = pix_fmt;
    GET_FORMAT_BITS(info.s_pix_fmt,n_pixel_bits); 

    info.width= (width + 1 ) / 2 * 2;
    info.height = (height + 1) / 2 * 2;


    info.time_base = rate;
    info.gop_size = GOP_SIZE; 
    info.max_b_frames = MAX_B_FRAME;
    info.thread_count = THREAD_COUNT;
    //info.profile = FF_PROFILE_H264_CONSTRAINED;
    info.profile = FF_PROFILE_H264_BASELINE;
    //info.s_pix_fmt = AV_PIX_FMT_BGRA;
    info.pix_fmt = AV_PIX_FMT_YUV420P;
    //info.qmin = 1;
    //info.qmax = 33;
    //info.qcompress = 0.5;

    h264_encoder_init(h264_encoder,&info);

}

static void* for_collection_cb(void* ctx,void * ud)
{
    RedEncodingThreading * threading = (RedEncodingThreading *)ctx;
    set_event(threading->_ev);
}

void surface_ready_encoding_threading(RedEncodingThreading * threading)
{
    threading->tcollection->surface_ready_collection_threading(threading->tcollection);
}

void insert_vedio_data_in_encoding_threading(RedEncodingThreading* threading,char* viedo_data)
{
    threading->tcollection->insert_vedio_data_in_colloction_threading(threading->tcollection,viedo_data);
}

void init_spice_encoding_threading(RedEncodingThreading * threading,int bits_size,SpiceRect *bbox,
        LCXCALLBACK *call_sending,LCXCALLBACK *get_bits,LCXCALLBACK *capture_surface,enum AVPixelFormat pix_fmt,int fr,int br)
{
    int ret = 0;
    threading->tbase->set_spice_base_threading_close(threading->tbase,false);
    //threading->g_h264_encoder = NULL;


    threading->tbase->init_spice_base_threading(threading->tbase,start_run);
    threading->tbase->init_spice_base_threading_data(threading->tbase,(void*)threading);


    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&threading->_lock,&attr);
    pthread_mutexattr_destroy(&attr);

    ring_init(&threading->h264_data_list);
    threading->list_size = 0;

    set_spice_encoding_threading_run(threading,start_run);

    threading->call_sending._cb = call_sending->_cb;
    threading->call_sending._ctx = call_sending->_ctx;
    threading->call_sending._ud = call_sending->_ud;

    threading->get_bits._cb = get_bits->_cb;
    threading->get_bits._ctx = get_bits->_ctx;
    threading->get_bits._ud = get_bits->_ud;

    //new event 

    threading->_ev = event_new();
    init_event(threading->_ev);


    
    //new collection threading
    threading->tcollection = collection_threading_new();
    if (!threading->tcollection)
    {
        spice_error("create collection threading error");
    }
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = ( 1000 * 1000) / fr; 

    LCXCALLBACK call_encoding;
    memset(&call_encoding,0,sizeof(LCXCALLBACK));
    call_encoding._cb = for_collection_cb;
    call_encoding._ctx = threading;
    call_encoding._ud = NULL;


    
    threading->tcollection->init_spice_collection_threading(threading->tcollection,&timeout,pix_fmt,
            bits_size,bbox,&call_encoding,capture_surface);

    //new h264 encoder
    threading->g_h264_encoder =  h264_encoder_new(); 
    create_spice_encoding_threading_h264_streaming(threading->g_h264_encoder,
            pix_fmt,
            bbox->right - bbox->left,
            bbox->bottom - bbox->top,fr,br);
   
}

void uninit_spice_encoding_threading(RedEncodingThreading * threading)
{
    RingItem *item_local, *next;
    H264DataItem *dataitem = NULL;
    
    if (threading->g_h264_encoder)
    {
        h264_encoder_uninit(threading->g_h264_encoder);
        h264_encoder_delete(threading->g_h264_encoder);
        threading->g_h264_encoder = NULL;
    }

    pthread_mutex_destroy(&threading->_lock);
    
    
    
    RING_FOREACH_SAFE(item_local,next,&threading->h264_data_list){
        dataitem = SPICE_CONTAINEROF(item_local, H264DataItem, item);
        if(dataitem)
        {
            if(dataitem->slice)
            {
                free(dataitem->slice);        
            }
            free(dataitem);
        }
    }
    threading->list_size = 0;

    if (threading->tcollection)
    {
        collection_threading_delete(threading->tcollection);
    }

    if (threading->_ev)
    {
        event_delete(threading->_ev);
    }

    
}



void set_spice_encoding_threading_run(RedEncodingThreading * threading,void* (*start_rtn)(void*))
{
    threading->tbase->set_spice_base_threading_run(threading->tbase,start_rtn);

}

void start_spice_encoding_threading(RedEncodingThreading * threading)
{
    int r;
    threading->tbase->set_spice_base_threading_close(threading->tbase,false);
    r = threading->tbase->start_spice_base_threading(threading->tbase);
    if (r)
    {
        spice_error("create encoding faild");
    }
    threading->tcollection->start_spice_collection_threading(threading->tcollection);
}

void stop_wait_spice_encoding_threading(RedEncodingThreading * threading)
{

    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : begin",__FILE__,__FUNCTION__,__LINE__);
    
    threading->tbase->set_spice_base_threading_close(threading->tbase,true);
    for_collection_cb((void*)threading,NULL);
    threading->tbase->stop_wait_spice_base_threading(threading->tbase);
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 222222222222",__FILE__,__FUNCTION__,__LINE__);
    threading->tcollection->stop_wait_spice_collection_threading(threading->tcollection);
    
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 33333333333",__FILE__,__FUNCTION__,__LINE__);
    uninit_spice_encoding_threading(threading);
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : end",__FILE__,__FUNCTION__,__LINE__);
}


void copy_bits_to_encoding_threading(RedEncodingThreading * threading,char* bits,int size,SpiceRect *bbox)
{
    threading->tcollection->copy_bits_to_collection_threading(threading->tcollection,bits,size,bbox);
}

void read_bits_with_cb_to_encoding_threading(RedEncodingThreading* threading,SpiceRect *bbox)
{
    threading->tcollection->read_bits_with_cb_to_collection_threading(threading->tcollection,&threading->get_bits,bbox);
}


void insert_h264_slice_in_encoding_threading(RedEncodingThreading * threading,H264DataItem * dataitem)
{
    if (dataitem)
    {
        pthread_mutex_lock(&threading->_lock);
        threading->list_size++;
        ring_add(&threading->h264_data_list,&dataitem->item);
        pthread_mutex_unlock(&threading->_lock);
    }
}

H264DataItem * remove_h264_slice_from_encoding_threading(RedEncodingThreading * threading)
{
    H264DataItem *dataitem = NULL;
    pthread_mutex_lock(&threading->_lock); 
    if (threading->list_size)
    {
        threading->list_size--;
        RingItem *ring_item = ring_get_tail(&threading->h264_data_list);
        ring_remove(ring_item);
        dataitem = SPICE_CONTAINEROF(ring_item, H264DataItem, item);
    }
    pthread_mutex_unlock(&threading->_lock);
    return dataitem;

}
RedBaseThreadingStatus get_spice_encoding_threading_status(RedEncodingThreading * threading)
{
    return threading->tbase->get_spice_base_threading_status(threading->tbase);

}


RedEncodingThreading * encoding_threading_new()
{
    RedBaseThreading * basethreading = spice_base_threading_new();
    if (!basethreading)
    {
        return NULL;
    }
    memset(basethreading,0,sizeof(RedBaseThreading));
    RedEncodingThreading * threading = (RedEncodingThreading *)malloc(sizeof(RedEncodingThreading));
    if (!threading)
    {
        if (basethreading)
        {
            spice_base_threading_delete(basethreading);
        }
        return NULL;
    }
    memset(threading,0,sizeof(RedEncodingThreading));
    threading->tbase = basethreading;
    RED_SPICE_ENCODING_THREADING_INIT(threading);
    threading->tbase->set_spice_base_threading_status(threading->tbase,RED_THREADING_STOPPING);
    return threading;
}

void encoding_threading_delete(RedEncodingThreading * threading)
{
    if(threading)
    {
        if(threading->tbase)
        {
            spice_base_threading_delete(threading->tbase);
            threading->tbase = NULL;
        }
        free(threading);
    }
}

void free_h264_data_item(H264DataItem *h264_dataitem)
{
    if (h264_dataitem)
    {
        if(h264_dataitem->slice)
        {
            free(h264_dataitem->slice);
        }
        free(h264_dataitem);
    }
}
