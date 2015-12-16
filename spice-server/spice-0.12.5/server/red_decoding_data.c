#include "red_decoding_data.h"
#include "h264_type.h"

#include <spice/protocol.h>
#include <errno.h>
#include<unistd.h>
#include <sys/time.h>
#include <sys/types.h>


static void get_h264_slice_data_from_decoding_threading(RedDecodingThreading * threading,
        char * slice,int *outbuf_size,char *bits)
{
    int width, height;
    int ret;
    int bpp;
    //pthread_mutex_lock(&threading->_lock);
    if (!threading->g_h264_encoder)
    {
        spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : %s",__FILE__,__FUNCTION__,__LINE__,"g_h264_encoder === NULL");
        return;
        //goto down;
    }
    
    width = threading->g_decoding_data.bbox.right - threading->g_decoding_data.bbox.left;
    height = threading->g_decoding_data.bbox.bottom - threading->g_decoding_data.bbox.top;

    if (threading->g_h264_encoder->codec_context->width != width || 
            threading->g_h264_encoder->codec_context->height != height || 
            threading->g_h264_encoder->codec_context->pix_fmt != threading->g_decoding_data.pix_fmt)
    {
        spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : \
                width (%d,%d) , height ( %d ,%d) fmt(%d,%d)",
                __FILE__,__FUNCTION__,__LINE__,
                width,
                threading->g_h264_encoder->codec_context->width, 
                height,
                threading->g_h264_encoder->codec_context->height,
                threading->g_h264_encoder->codec_context->pix_fmt,
                threading->g_decoding_data.pix_fmt
                );
        return;
        //goto down;
    }

    GET_FORMAT_BYTES(threading->g_decoding_data.pix_fmt,bpp) ;
    ret = h264_encode(threading->g_h264_encoder,bits,width * height * bpp,
            slice,&outbuf_size);
    if (H264_NOT_GET_SLICE == ret)
    {

    }
    else if(H264_ENCODER_OK == ret)
    {
#if 1
        char filename[1000] = "";
        sprintf(filename,"/home/lichenxiang/stream_before/h264_0x%xh264",threading->g_h264_encoder);
        FILE *fp = fopen(filename,"a");
        fwrite(slice,1,outbuf_size,fp);
        fclose(fp);
#endif
    }
    else 
    {
        spice_printerr("h264_encode error");
    }
//down:
//    pthread_mutex_unlock(&threading->_lock);
}

static void do_callback(RedDecodingThreading * threading,char * slice,int outbuf_size)
{
    int call_flag = 0;
    pthread_mutex_lock(&threading->_lock);
    
    call_flag =  threading->list_size; 
    insert_h264_slice_in_decoding_threading(threading,slice,outbuf_size);
    
    if(call_flag == 0)
    {
        threading->job._cb(threading->job._ctx,threading->job._ud);
    }
    
    pthread_mutex_unlock(&threading->_lock);

}

static void * start_run(void * args)
{
    RedDecodingThreading * threading = (RedDecodingThreading * )args;
    int ret = 0;
    fd_set rfds;
    struct timeval tv_si;
    struct timeval tv_ti0;
    struct timeval tv_ti1;
    int retval;
    int maxfd = 0;

    suseconds_t ofr_t = 0; 

    size_t outbuf_size;
    char * slice = NULL;
    char * bits = NULL;
    outbuf_size = OUTPUT_BUF_SIZE_H264;
    slice = malloc(OUTPUT_BUF_SIZE_H264);
    bits = malloc(threading->g_decoding_data.max_bits_size);

    memset(&tv_si,0,sizeof(struct timeval ));
    memset(&tv_ti0,0,sizeof(struct timeval ));
    memset(&tv_ti1,0,sizeof(struct timeval ));


    pthread_mutex_lock(&threading->_lock);

    threading->g_h264_encoder->codec_context->time_base;
    ofr_t = 1000 * 1000 / threading->g_h264_encoder->codec_context->time_base.den * 
        threading->g_h264_encoder->codec_context->time_base.num ; 

    pthread_mutex_unlock(&threading->_lock);

    while(threading->tbase->get_spice_base_threading_close(threading->tbase))
    {
        FD_ZERO(&rfds);
        FD_SET(threading->fd[0], &rfds);
        maxfd = threading->fd[0] + 1; 


        gettimeofday (&tv_ti0, NULL);
        retval = select(maxfd,&rfds,NULL,NULL,&tv_si);
        if (retval < 0)
        {
            if(errno == EINTR || errno == EAGAIN)
            {
                gettimeofday (&tv_ti1, NULL);
                suseconds_t diff = 0;
                diff = (tv_ti1.tv_sec * 1000 * 1000 + tv_ti1.tv_usec) - (tv_ti0.tv_sec * 1000 * 1000 + tv_ti0.tv_usec);
                tv_si.tv_sec = diff / (1000 * 1000);
                tv_si.tv_usec =  diff % (1000 * 1000);
            }
            else
            {
                tv_si.tv_sec = 1;
                tv_si.tv_usec = 0;
            }
            continue;
        }
        else if (retval == 0)
        {
            //time out begin to codec
            pthread_mutex_lock(&threading->_lock);
            memcpy(bits,threading->g_decoding_data.bits,threading->g_decoding_data.max_bits_size);
            pthread_mutex_unlock(&threading->_lock);

            get_h264_slice_data_from_decoding_threading(threading,slice,&outbuf_size,bits);
            do_callback(threading,slice,outbuf_size);

            gettimeofday (&tv_ti1, NULL);
            suseconds_t diff = 0;
            diff = (tv_ti1.tv_sec * 1000 * 1000 + tv_ti1.tv_usec) - (tv_ti0.tv_sec * 1000 * 1000 + tv_ti0.tv_usec);
            diff = ofr_t - diff; 
            if (diff >= 0)
            {
                tv_si.tv_sec = diff / (1000 * 1000);
                tv_si.tv_usec =  diff % (1000 * 1000);
            }
            else
            {
                tv_si.tv_sec = 0;
                tv_si.tv_usec = 0;
            }
        }
        else
        {
            spice_printerr("recv a signal from rfds");

            if (FD_ISSET(threading->fd[0],&rfds))
            {
                char buf[100] = "";
                read(threading->fd[0],buf,100);
            }
            continue;
        }
    }

    if(slice)
    {
        free(slice);
        slice = NULL;
    }
    if(bits)
    {
        free(bits);
        bits = NULL;
    }

    return 0Xdead;
}

static void create_spice_decoding_threading_h264_streaming(H264Encoder* h264_encoder,enum AVPixelFormat pix_fmt,
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
        LCXCALLBACK *job,enum AVPixelFormat pix_fmt,int fr,int br)
{
    int ret = 0;
    threading->tbase->set_spice_base_threading_close(threading->tbase,false);
    threading->worker = worker;
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

    init_spice_decoding_threading_data(threading,bits_size,bbox);
    set_spice_decoding_threading_run(threading,start_run);

    threading->job._cb = job->_cb;
    threading->job._ctx = job->_ctx;
    threading->job._ud = job->_ud;

    threading->g_h264_encoder =  h264_encoder_new(); 
    threading->g_decoding_data.pix_fmt = pix_fmt;
    create_spice_decoding_threading_h264_streaming(threading->g_h264_encoder,
            threading->g_decoding_data.pix_fmt,
            threading->g_decoding_data.bbox.right - threading->g_decoding_data.bbox.left,
            threading->g_decoding_data.bbox.bottom - threading->g_decoding_data.bbox.top,fr,br);
    ret = pipe(threading->fd);
    if(ret != 0)
    {
        spice_error("create pipe error = %d",errno);
    
    }
   
}

void uninit_spice_decoding_threading(RedDecodingThreading * threading)
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
    
    memset(&threading->g_decoding_data.bbox,0,sizeof(SpiceRect));
    if(threading->g_decoding_data.bits)
    {
        free(threading->g_decoding_data.bits);
        threading->g_decoding_data.bits = NULL;
    }
    
    threading->g_decoding_data.max_bits_size = 0;
    threading->worker = NULL;
    
    
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
    threading->tbase->set_spice_base_threading_close(threading->tbase,false);
    r = threading->tbase->start_spice_base_threading(threading->tbase);
    if (r)
    {
        spice_error("create decoding faild");
    }
}

void stop_wait_spice_decoding_threading(RedDecodingThreading * threading)
{
    threading->tbase->set_spice_base_threading_close(threading->tbase,true);
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
    memset(basethreading,0,sizeof(RedBaseThreading));
    RedDecodingThreading * threading = (RedDecodingThreading *)malloc(sizeof(RedDecodingThreading));
    if (!threading)
    {
        if (basethreading)
        {
            spice_base_threading_delete(basethreading);
        }
        return NULL;
    }
    memset(threading,0,sizeof(RedDecodingThreading));
    threading->tbase = basethreading;
    RED_SPICE_DECODING_THREADING_INIT(threading);
    threading->tbase->set_spice_base_threading_status(threading->tbase,RED_THREADING_STOPPING);
}

void decoding_threading_delete(RedDecodingThreading * threading)
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

