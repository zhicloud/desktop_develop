#include "red_collection_media_data_threading.h"


#define RED_SPICE_COLLECTION_THREADING_INIT(threading) do{\
   RED_SPICE_BASE_THREADING_INIT(threading->tbase);\
   threading->init_spice_collection_threading = init_spice_collection_threading;\
   threading->set_spice_collection_threading_run = set_spice_collection_threading_run;\
   threading->stop_wait_spice_collection_threading = stop_wait_spice_collection_threading;\
   threading->start_spice_collection_threading = start_spice_collection_threading;\
   threading->remove_vedio_data_form_collection_threading = remove_vedio_data_form_collection_threading;\
   threading->copy_bits_to_collection_threading = copy_bits_to_collection_threading;\
   threading->read_bits_with_cb_to_collection_threading = read_bits_with_cb_to_collection_threading;\
   threading->surface_ready_collection_threading = surface_ready_collection_threading;\
   threading->insert_vedio_data_in_colloction_threading = insert_vedio_data_in_colloction_threading;\
}while(0)

void init_spice_collection_threading(RedCollectionThreading * threading,struct timeval *timeout,enum AVPixelFormat pix_fmt,
        int bits_size,SpiceRect *bbox,LCXCALLBACK *call_encoding,LCXCALLBACK *capture_surface);
void init_spice_collection_threading_data(RedCollectionThreading* threading,int bits_size,SpiceRect *bbox,enum AVPixelFormat pix_fmt);
void set_spice_collection_threading_run(RedCollectionThreading * threading,void* (*start_rtn)(void*));
void start_spice_collection_threading(RedCollectionThreading * threading);
void stop_wait_spice_collection_threading(RedCollectionThreading * threading);
void uninit_spice_collection_threading(RedCollectionThreading * threading);
void copy_bits_to_collection_threading(RedCollectionThreading * threading,char* bits,int size,SpiceRect *bbox);
void read_bits_with_cb_to_collection_threading(RedCollectionThreading * threading,LCXCALLBACK *read_bits,SpiceRect *bbox);
void surface_ready_collection_threading(RedCollectionThreading * threading);
void insert_vedio_data_in_colloction_threading(RedCollectionThreading * threading,char* viedo_data);

bool remove_vedio_data_form_collection_threading(RedCollectionThreading * threading,VedioDataItem ** vditem);


RedCollectionThreading* collection_threading_new()
{
    RedBaseThreading * basethreading = spice_base_threading_new();
    if (!basethreading)
    {
        return NULL;
    }
    memset(basethreading,0,sizeof(RedBaseThreading));
    RedCollectionThreading* threading = (RedCollectionThreading*)malloc(sizeof(RedCollectionThreading));
    if (!threading)
    {
        if (basethreading)
        {
            spice_base_threading_delete(basethreading);
        }
        return NULL;
    }
    memset(threading,0,sizeof(RedCollectionThreading));
    threading->tbase = basethreading;
    RED_SPICE_COLLECTION_THREADING_INIT(threading);
    threading->tbase->set_spice_base_threading_status(threading->tbase,RED_THREADING_STOPPING);
    return threading;
}

void collection_threading_delete(RedCollectionThreading* threading)
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



bool remove_vedio_data_form_collection_threading(RedCollectionThreading * threading,VedioDataItem ** vditem)
{
    bool flag = false;
    VedioDataItem *dataitem = NULL;
    pthread_mutex_lock(&threading->_lock); 
    if (threading->list_size > 0)
    {
        threading->list_size--;
        RingItem *ring_item = ring_get_tail(&threading->vedio_data_list);
        ring_remove(ring_item);
        dataitem = SPICE_CONTAINEROF(ring_item, VedioDataItem, item);
        *vditem = dataitem;
        flag = true;
    }
    pthread_mutex_unlock(&threading->_lock);

    return flag;
}

bool get_audio_data_form_collection_threading(RedCollectionThreading * threading)
{
    //TODO
}

void insert_vedio_data_in_colloction_threading(RedCollectionThreading * threading,char* viedo_data)
{
    if(viedo_data)
    {
        VedioDataItem *dataitem = (VedioDataItem *)malloc(sizeof(char) * sizeof(VedioDataItem));
        dataitem->bits = viedo_data;
        dataitem->item.prev = NULL;
        dataitem->item.next = NULL;
        memcpy(&dataitem->vedio_info,&threading->vedio_info,sizeof(SpiceVedioInfo));

        pthread_mutex_lock(&threading->_lock);
        threading->list_size++;
        if (threading->list_size > MAX_LIST_SIZE)
        {
            spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : colloction Vedio size is to big (%d)",__FILE__,__FUNCTION__,__LINE__,
                    threading->list_size);
        }
        ring_add(&threading->vedio_data_list,&dataitem->item);
        pthread_mutex_unlock(&threading->_lock);
    }
}

static void do_callback(RedCollectionThreading * threading)
{
    /*
    int call_flag = 0;
    pthread_mutex_lock(&threading->_lock);
    
    call_flag =  threading->list_size; 
    insert_h264_slice_in_encoding_threading(threading,slice,outbuf_size);
    
    if(call_flag == 0)
    {
        threading->job._cb(threading->job._ctx,threading->job._ud);
    }
    
    pthread_mutex_unlock(&threading->_lock);
*/
    if (threading->call_encoding._cb)
    {
        threading->call_encoding._ud = NULL;
        threading->call_encoding._cb(threading->call_encoding._ctx,threading->call_encoding._ud);
    }
}




static void capture_vedio_data(RedCollectionThreading* threading)
{
    pthread_mutex_lock(&threading->_lock);
    char * video_bits = NULL;
    video_bits = malloc(threading->vedio_info.max_bits_size);

    memcpy(video_bits,threading->g_vedio_data.bits,threading->vedio_info.max_bits_size);
    insert_vedio_data_in_colloction_threading(threading,video_bits);
    pthread_mutex_unlock(&threading->_lock);
}


static void capture_vedio_data_sync(RedCollectionThreading* threading)
{
   if (threading->capture_surface._cb)
   {
       threading->capture_surface._ud = NULL;
       threading->capture_surface._cb(threading->capture_surface._ctx,threading->capture_surface._ud);
       int ret = event_wait(threading->_dev);
   }
}



static void capture_audio_data(RedCollectionThreading* threading)
{
    pthread_mutex_lock(&threading->_lock);


    pthread_mutex_unlock(&threading->_lock);


}

static void * start_run(void * args)
{
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : begin",__FILE__,__FUNCTION__,__LINE__);
    RedCollectionThreading* threading = (RedCollectionThreading* )args;
    int ret = 0;
    fd_set rfds;
    struct timeval tv_si;
    struct timeval tv_ti0;
    struct timeval tv_ti1;
    int retval;
    int maxfd = 0;



    suseconds_t ofr_t = 0; 

    memset(&tv_si,0,sizeof(struct timeval ));
    memset(&tv_ti0,0,sizeof(struct timeval ));
    memset(&tv_ti1,0,sizeof(struct timeval ));


    pthread_mutex_lock(&threading->_lock);
    ofr_t = 1000 * 1000 * threading->timeout.tv_sec + threading->timeout.tv_usec; 
    pthread_mutex_unlock(&threading->_lock);

    while(threading->tbase->get_spice_base_threading_close(threading->tbase) == false)
    {
        FD_ZERO(&rfds);
        FD_SET(threading->fd[0], &rfds);
        maxfd = threading->fd[0] + 1; 


        gettimeofday (&tv_ti0, NULL);
        /*
        spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : tv_sec = %d tv_usec =%d",__FILE__,__FUNCTION__,__LINE__,
                tv_si.tv_sec,
                tv_si.tv_usec);*/
        uint64_t time_now = red_now();
        spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : =================  %ld.%ld ========================",
                __FILE__,__FUNCTION__,__LINE__,
                time_now / (1000*1000*1000),time_now % (1000*1000*1000));
        
        retval = select(maxfd,&rfds,NULL,NULL,&tv_si);
        if (retval < 0)
        {
            if(errno == EINTR || errno == EAGAIN)
            {
                gettimeofday (&tv_ti1, NULL);
                suseconds_t diff = 0;
                diff = (tv_ti1.tv_sec * 1000 * 1000 + tv_ti1.tv_usec) - (tv_ti0.tv_sec * 1000 * 1000 + tv_ti0.tv_usec);
                diff = ofr_t - diff;
                tv_si.tv_sec = diff / (1000 * 1000);
                tv_si.tv_usec =  diff % (1000 * 1000);
            }
            else
            {
                tv_si.tv_sec = 1;
                tv_si.tv_usec = 0;
            }
            spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : ",__FILE__,__FUNCTION__,__LINE__);
            continue;
        }
        else if (retval == 0)
        {
            //time out begin to codec
            gettimeofday (&tv_ti0, NULL);
#if GET_DATA_SYNC
            capture_vedio_data_sync(threading);
#else
            capture_vedio_data(threading);
#endif
            capture_audio_data(threading);
            
            do_callback(threading);

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
                spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : ?????????",__FILE__,__FUNCTION__,__LINE__);
                tv_si.tv_sec = 0;
                tv_si.tv_usec = 0;
            }
        }
        else
        {
            spice_printerr("colloction threading recv a signal from rfds");
            if (FD_ISSET(threading->fd[0],&rfds))
            {
                char buf[100] = "";
                read(threading->fd[0],buf,100);
            }

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
            continue;
        }
    }

    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : end",__FILE__,__FUNCTION__,__LINE__);
    return 0Xdead;

}

void surface_ready_collection_threading(RedCollectionThreading * threading)
{
    set_event(threading->_dev);
}

void init_spice_collection_threading(RedCollectionThreading * threading,struct timeval *timeout,enum AVPixelFormat pix_fmt,
        int bits_size,SpiceRect *bbox,LCXCALLBACK *call_encoding,LCXCALLBACK *capture_surface)
{
    int ret = 0;
    threading->tbase->set_spice_base_threading_close(threading->tbase,false);


    threading->tbase->init_spice_base_threading(threading->tbase,start_run);
    threading->tbase->init_spice_base_threading_data(threading->tbase,(void*)threading);


    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&threading->_lock,&attr);
    pthread_mutexattr_destroy(&attr);

    memcpy(&threading->timeout,timeout,sizeof(struct timeval));

    ring_init(&threading->vedio_data_list);
    threading->list_size = 0;
    
    
    threading->_dev = event_new();
    init_event(threading->_dev);

    memcpy(&threading->call_encoding,call_encoding,sizeof(LCXCALLBACK));
    memcpy(&threading->capture_surface,capture_surface,sizeof(LCXCALLBACK));

    set_spice_collection_threading_run(threading,start_run);
    init_spice_collection_threading_data(threading,bits_size,bbox,pix_fmt);

    ret = pipe(threading->fd);
    if(ret != 0)
    {
        spice_error("create pipe error = %d",errno);

    }
    
}

void set_spice_collection_threading_run(RedCollectionThreading * threading,void* (*start_rtn)(void*))
{
    threading->tbase->set_spice_base_threading_run(threading->tbase,start_rtn);
}

void init_spice_collection_threading_data(RedCollectionThreading* threading,int bits_size,SpiceRect *bbox,enum AVPixelFormat pix_fmt)
{
    threading->vedio_info.max_bits_size = bits_size;
    threading->vedio_info.bbox.left = bbox->left;
    threading->vedio_info.bbox.top = bbox->top;
    threading->vedio_info.bbox.right = bbox->right;
    threading->vedio_info.bbox.bottom = bbox->bottom;
    threading->vedio_info.pix_fmt = pix_fmt;
    memcpy(&threading->g_vedio_data.vedio_info,&threading->vedio_info,sizeof(SpiceVedioInfo));
    threading->g_vedio_data.bits = malloc(bits_size);
}

void uninit_spice_collection_threading(RedCollectionThreading * threading)
{
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : begin",__FILE__,__FUNCTION__,__LINE__);
    RingItem *item_local, *next;
    VedioDataItem *dataitem = NULL;

    pthread_mutex_destroy(&threading->_lock);
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 11111111111111111111111",__FILE__,__FUNCTION__,__LINE__);

    RING_FOREACH_SAFE(item_local,next,&threading->vedio_data_list){
        dataitem = SPICE_CONTAINEROF(item_local, VedioDataItem, item);
        if(dataitem)
        {
            if(dataitem->bits)
            {
                free(dataitem->bits);        
            }
            free(dataitem);
        }
    }
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 222222222222222222222222222",__FILE__,__FUNCTION__,__LINE__);

    threading->list_size = 0;
    close(threading->fd[0]);
    close(threading->fd[1]);
    
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 3333333333333333333333333333",__FILE__,__FUNCTION__,__LINE__);
    if (threading->_dev)
    {
        spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 444444444444444444444",__FILE__,__FUNCTION__,__LINE__);
        event_delete(threading->_dev);
        spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 55555555555555555555555",__FILE__,__FUNCTION__,__LINE__);
    }
    if (threading->g_vedio_data.bits)
    {
        free(threading->g_vedio_data.bits);
        threading->g_vedio_data.bits = NULL;
    
    }
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : end",__FILE__,__FUNCTION__,__LINE__);
}

void stop_wait_spice_collection_threading(RedCollectionThreading * threading)
{
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : begin",__FILE__,__FUNCTION__,__LINE__);
    threading->tbase->set_spice_base_threading_close(threading->tbase,true);
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 11111111111111",__FILE__,__FUNCTION__,__LINE__);
    write(threading->fd[1],WITRE_CLOSE_MSG,strlen(WITRE_CLOSE_MSG));
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 2222222222222222222",__FILE__,__FUNCTION__,__LINE__);
    surface_ready_collection_threading(threading);
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 3333333333333333333",__FILE__,__FUNCTION__,__LINE__);
    threading->tbase->stop_wait_spice_base_threading(threading->tbase);
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : 444444444444444444444",__FILE__,__FUNCTION__,__LINE__);
    uninit_spice_collection_threading(threading);
    spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : end",__FILE__,__FUNCTION__,__LINE__);
}

void start_spice_collection_threading(RedCollectionThreading * threading)
{
    int r;
    threading->tbase->set_spice_base_threading_close(threading->tbase,false);
    r = threading->tbase->start_spice_base_threading(threading->tbase);
    if (r)
    {
        spice_error("create encoding faild");
    }
}

void free_vedio_data_item(VedioDataItem *item)
{
    if (item)
    {
        if(item->bits)
        {
            free(item->bits);
        }
        free(item);
    }
}

void copy_bits_to_collection_threading(RedCollectionThreading * threading,char* bits,int size,SpiceRect *bbox)
{
    if(bits)
    {
        pthread_mutex_lock(&threading->_lock);
        if(threading->g_vedio_data.vedio_info.max_bits_size >= size && 
                threading->g_vedio_data.vedio_info.bbox.left == bbox->left &&
                threading->g_vedio_data.vedio_info.bbox.top == bbox->top &&
                threading->g_vedio_data.vedio_info.bbox.right == bbox->right &&
                threading->g_vedio_data.vedio_info.bbox.bottom == bbox->bottom)
        {
            memcpy(threading->g_vedio_data.bits,bits,size);

        }
        else
        {
            spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : %s  max_bits_size(%d,%d) bbox[left(%d,%d) top(%d,%d) right(%d,%d) bottom(%d,%d)]",
                    __FILE__,__FUNCTION__,__LINE__,
                    "the size or bbox is wrong",
                    threading->g_vedio_data.vedio_info.max_bits_size,size,
                    threading->g_vedio_data.vedio_info.bbox.left,bbox->left,
                    threading->g_vedio_data.vedio_info.bbox.top,bbox->top,
                    threading->g_vedio_data.vedio_info.bbox.right,bbox->right,
                    threading->g_vedio_data.vedio_info.bbox.bottom,bbox->bottom
                    );
        }
        pthread_mutex_unlock(&threading->_lock);
    }

}



void read_bits_with_cb_to_collection_threading(RedCollectionThreading * threading,LCXCALLBACK *read_bits,SpiceRect *bbox)
{
    if(read_bits)
    {
        pthread_mutex_lock(&threading->_lock);
        if(threading->g_vedio_data.vedio_info.bbox.left == bbox->left &&
                threading->g_vedio_data.vedio_info.bbox.top == bbox->top &&
                threading->g_vedio_data.vedio_info.bbox.right == bbox->right &&
                threading->g_vedio_data.vedio_info.bbox.bottom == bbox->bottom)
        {
            if(read_bits->_cb)
            {
                read_bits->_ud = &(threading->g_vedio_data);
                read_bits->_cb(read_bits->_ctx,read_bits->_ud);
            }

        }
        else
        {
            spice_printerr("[FILE : %s][FUNCTION : %s][LINE : %d] : %s  bbox[left(%d,%d) top(%d,%d) right(%d,%d) bottom(%d,%d)]",
                    __FILE__,__FUNCTION__,__LINE__,
                    "the size or bbox is wrong",
                    threading->g_vedio_data.vedio_info.bbox.left,bbox->left,
                    threading->g_vedio_data.vedio_info.bbox.top,bbox->top,
                    threading->g_vedio_data.vedio_info.bbox.right,bbox->right,
                    threading->g_vedio_data.vedio_info.bbox.bottom,bbox->bottom
                    );
        }
        pthread_mutex_unlock(&threading->_lock);
    }


}


