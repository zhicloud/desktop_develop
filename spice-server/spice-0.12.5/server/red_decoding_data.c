#include "red_decoding_data.h"
#include <spice/protocol.h>


static void create_spice_decoding_threading_h264_streaming(H264Encoder* h264_encoder,enum AVPixelFormat pix_fmt,
        int width,int height)
{
    uint32_t frame_mm_time;
    int n;
    int ret;

    H264StreamInfo info;
    AVRational rate = { 1, 30};
    info.bit_rate = BIT_RATE; 
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
    info.profile = FF_PROFILE_H264_CONSTRAINED;
    //info.profile = FF_PROFILE_H264_BASELINE;
    //info.s_pix_fmt = AV_PIX_FMT_BGRA;
    info.pix_fmt = AV_PIX_FMT_YUV420P;
    //info.qmin = 1;
    //info.qmax = 33;
    //info.qcompress = 0.5;

    h264_encoder_init(h264_encoder,&info);
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : n_pixel_bits = %d",__FILE__,__FUNCTION__,__LINE__,n_pixel_bits);

}

void init_spice_decoding_threading(RedDecodingThreading * threading,void *worker,int bits_size,SpiceRect *bbox,
        void* (*start_rtn)(void*),enum AVPixelFormat pix_fmt)
{
    threading->bClose = false;
    threading->worker = worker;
    //threading->g_h264_encoder = NULL;

    
    threading->tbase->init_spice_base_threading(threading->tbase,start_rtn);
    threading->tbase->init_spice_base_threading_data(threading->tbase,(void*)threading);


    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&threading->_lock,&attr);
    pthread_mutexattr_destroy(&attr);

    ring_init(&threading->h264_data_list);
    threading->list_size = 0;

    init_spice_decoding_threading_data(threading,bits_size,bbox);
    set_spice_decoding_threading_run(threading,start_rtn);
    
    threading->g_h264_encoder =  h264_encoder_new(); 
    threading->g_decoding_data.pix_fmt = pix_fmt;
    create_spice_decoding_threading_h264_streaming(threading->g_h264_encoder,
            threading->g_decoding_data.pix_fmt,
            threading->g_decoding_data.bbox.right - threading->g_decoding_data.bbox.left,
            threading->g_decoding_data.bbox.bottom - threading->g_decoding_data.bbox.top);
   
}

void uninit_spice_decoding_threading(RedDecodingThreading * threading)
{
    if (threading->g_h264_encoder)
    {
        h264_encoder_uninit(threading->g_h264_encoder);
        h264_encoder_delete(threading->g_h264_encoder);
        threading->g_h264_encoder = NULL;
    }

    pthread_mutex_destroy(&threading->_lock);
    
    memset(&threading->g_decoding_data.bbox,0,sizeof(SpiceRect));
    if(threading->g_decoding_data.bits)
    {
        free(threading->g_decoding_data.bits);
        threading->g_decoding_data.bits = NULL;
    }
    if(threading->tbase)
    {
        free(threading->tbase);
        threading->tbase = NULL;
    }
    threading->g_decoding_data.max_bits_size = 0;
    threading->worker = NULL;

}


void init_spice_decoding_threading_data(RedDecodingThreading* threading,int bits_size,SpiceRect *bbox)
{
    threading->g_decoding_data.max_bits_size = bits_size;
    threading->g_decoding_data.bits = malloc(sizeof(char) * bits_size);
    if(!threading->g_decoding_data.bits)
    {
        spice_error("[FILE : %s][FUNCTION : %s][LINE : %d] : %s",
                __FILE__,__FUNCTION__,__LINE__,
                "malloc bits error");
    }
    threading->g_decoding_data.bbox.left = bbox->left;
    threading->g_decoding_data.bbox.top = bbox->top;
    threading->g_decoding_data.bbox.right = bbox->right;
    threading->g_decoding_data.bbox.bottom = bbox->bottom;

}

void set_spice_decoding_threading_run(RedDecodingThreading * threading,void* (*start_rtn)(void*))
{
    threading->tbase->set_spice_base_threading_run(threading->tbase,start_rtn);

}

void start_spice_decoding_threading(RedDecodingThreading * threading)
{
    int r;
    threading->bClose = false;
    r = threading->tbase->start_spice_base_threading(threading->tbase);
    if (r)
    {
        spice_error("create decoding faild");
    }
}

void stop_wait_spice_decoding_threading(RedDecodingThreading * threading)
{
    threading->bClose = true;
    threading->tbase->stop_wait_spice_base_threading(threading->tbase);
    uninit_spice_decoding_threading(threading);
}


void copy_bits_to_decoding_threading(RedDecodingThreading * threading,char* bits,int size,SpiceRect *bbox)
{
    if(bits)
    {
        pthread_mutex_lock(&threading->_lock);
        if(threading->g_decoding_data.max_bits_size >= size && 
                threading->g_decoding_data.bbox.left == bbox->left &&
                threading->g_decoding_data.bbox.top == bbox->top &&
                threading->g_decoding_data.bbox.right == bbox->right &&
                threading->g_decoding_data.bbox.bottom == bbox->bottom)
        {
            memcpy(threading->g_decoding_data.bits,bits,size);
        
        }
        else
        {
            spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : %s  max_bits_size(%d,%d) bbox[left(%d,%d) top(%d,%d) right(%d,%d) bottom(%d,%d)]",
                    __FILE__,__FUNCTION__,__LINE__,
                    "the size or bbox is wrong",
                    threading->g_decoding_data.max_bits_size,size,
                    threading->g_decoding_data.bbox.left,bbox->left,
                    threading->g_decoding_data.bbox.top,bbox->top,
                    threading->g_decoding_data.bbox.right,bbox->right,
                    threading->g_decoding_data.bbox.bottom,bbox->bottom
                    );
        }
        pthread_mutex_unlock(&threading->_lock);
    }

}


void insert_h264_slice_in_decoding_threading(RedDecodingThreading * threading,char* slice,int size)
{
    if(slice)
    {
        H264DataItem *dataitem = (H264DataItem *)malloc(sizeof(char) * sizeof(H264DataItem));
        dataitem->slice = (char *)malloc(sizeof(char) * size);
        dataitem->size = size;
        memcpy(dataitem->slice,slice,size);
        pthread_mutex_lock(&threading->_lock);
        threading->list_size++;
        ring_add(&threading->h264_data_list,&dataitem->item);
        pthread_mutex_unlock(&threading->_lock);
    }
}

void get_h264_slice_from_decoding_threading(RedDecodingThreading * threading,char * slice , int *size)
{
    H264DataItem *dataitem = NULL;
    pthread_mutex_lock(&threading->_lock); 
    if (threading->list_size)
    {
        threading->list_size--;
        RingItem *ring_item = ring_get_tail(&threading->h264_data_list);
        ring_remove(ring_item);
        dataitem = SPICE_CONTAINEROF(ring_item, H264DataItem, item);
        *size = dataitem->size;
        memcpy(slice,dataitem->slice,*size);

    }
    pthread_mutex_unlock(&threading->_lock);

    if(dataitem)
    {
        if(dataitem->slice)
        {
            free(dataitem->slice);        
        }
        free(dataitem);
    }
}


RedDecodingThreading * decoding_threading_new()
{
    RedBaseThreading * basethreading = spice_base_threading_new();
    if (!basethreading)
    {
        return NULL;
    }

    RedDecodingThreading * threading = (RedDecodingThreading *)malloc(sizeof(RedDecodingThreading));
    if (!threading)
    {
        if (basethreading)
        {
            free(basethreading);
        }
        return NULL;
    }
    threading->tbase = basethreading;
    RED_SPICE_DECODING_THREADING_INIT(threading);
}

void decoding_threading_delete(RedDecodingThreading * threading)
{
    if(threading)
    {
        free(threading);
    }
}

