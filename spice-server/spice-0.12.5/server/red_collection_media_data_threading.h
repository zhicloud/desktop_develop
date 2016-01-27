#ifndef RED_COLLECTION_MEDIA_DATA_THREADING_H
#define RED_COLLECTION_MEDIA_DATA_THREADING_H
/*********************************************************************************************************
 * 
 * 说明：媒体数据采集线程
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
#include "common/rect.h"
#include "common/region.h"
#include "common/ring.h"
#include "red_threading.h"
#include "red_spice_common_defines.h"
#include "red_event.h"
#include "red_time.h"

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>

#include <string.h>



#define GET_DATA_SYNC 0
#define MAX_LIST_SIZE 100

typedef struct RedCollectionThreading RedCollectionThreading;
typedef struct VedioDataItem VedioDataItem;
typedef struct SpiceVedioInfo SpiceVedioInfo;


struct SpiceVedioInfo
{
   int max_bits_size;
   SpiceRect bbox;
   enum AVPixelFormat pix_fmt;
};

struct VedioDataItem
{
   RingItem  item;
   char * bits; 
   SpiceVedioInfo vedio_info;
};


struct RedCollectionThreading
{
   RedBaseThreading *tbase;
   int fd[2];
   
   pthread_mutex_t _lock;
   Ring vedio_data_list;
   int list_size;
   struct timeval timeout;
   SpiceVedioInfo vedio_info;
   VedioDataItem g_vedio_data;
   VedioDataItem g_vedio_data_sync;
   LCXCALLBACK call_encoding;
   LCXCALLBACK capture_surface;
   Event *_dev;
   




   void (*init_spice_collection_threading)(RedCollectionThreading * threading,struct timeval *timeout,enum AVPixelFormat pix_fmt,
        int bits_size,SpiceRect *bbox,LCXCALLBACK *call_encoding,LCXCALLBACK *capture_surface);
   void (*set_spice_collection_threading_run)(RedCollectionThreading * threading,void* (*start_rtn)(void*));
   void (*stop_wait_spice_collection_threading)(RedCollectionThreading * threading);
   void (*start_spice_collection_threading)(RedCollectionThreading * threading);
   void (*uninit_spice_collection_threading)(RedCollectionThreading * threading);
   bool (*remove_vedio_data_form_collection_threading)(RedCollectionThreading * threading,VedioDataItem ** vditem);
   void (*copy_bits_to_collection_threading)(RedCollectionThreading * threading,char* bits,int size,SpiceRect *bbox);
   void (*read_bits_with_cb_to_collection_threading)(RedCollectionThreading * threading,LCXCALLBACK *read_bits,SpiceRect *bbox);
   void (*surface_ready_collection_threading)(RedCollectionThreading * threading);
   void (*insert_vedio_data_in_colloction_threading)(RedCollectionThreading * threading,char* viedo_data);
};


RedCollectionThreading* collection_threading_new();
void collection_threading_delete(RedCollectionThreading* threading);
void free_vedio_data_item(VedioDataItem *item);




#endif
