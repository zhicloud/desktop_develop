#ifndef RED_ENCODING_DATA_H
#define RED_ENCODING_DATA_H

/*********************************************************************************************************
 * 
 * 说明：编码线程定义
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


#include <pthread.h>
#include <stdbool.h>



#include "common/rect.h"
#include "common/region.h"
#include "common/ring.h"
#include "red_threading.h"
#include "h264_encoder.h"
#include "red_spice_common_defines.h"
#include "red_collection_media_data_threading.h"
#include "red_event.h"
#include "red_time.h"

typedef struct RedEncodingThreading RedEncodingThreading;
typedef struct H264DataItem H264DataItem;
/*
typedef struct SpiceEncodingData SpiceEncodingData;


struct SpiceEncodingData
{
   char * bits; 
   int max_bits_size;
   SpiceRect bbox;
   enum AVPixelFormat pix_fmt;
};
*/


struct H264DataItem
{
   RingItem  item;
   char * slice;
   int size;
   uint32_t frame_mm_time;
};

struct RedEncodingThreading
{
   RedBaseThreading *tbase;

   pthread_mutex_t _lock;
   //SpiceEncodingData g_encoding_data; 
   
   LCXCALLBACK call_sending;
   LCXCALLBACK get_bits;
   
   Ring h264_data_list;
   int list_size;

   H264Encoder *g_h264_encoder;
   
   RedCollectionThreading *tcollection; 
   Event *_ev;





   //interface
   void (*init_spice_encoding_threading)(RedEncodingThreading * threading,int bits_size,SpiceRect *bbox,
        LCXCALLBACK *call_sending,LCXCALLBACK *get_bits,LCXCALLBACK *capture_surface,enum AVPixelFormat pix_fmt,int fr,int br);
   void (*set_spice_encoding_threading_run)(RedEncodingThreading * threading,void* (*start_rtn)(void*));
   void (*start_spice_encoding_threading)(RedEncodingThreading * threading);
   void (*stop_wait_spice_encoding_threading)(RedEncodingThreading * threading);
   void (*copy_bits_to_encoding_threading)(RedEncodingThreading * threading,char* bits,int size,SpiceRect *bbox);
   void (*insert_h264_slice_in_encoding_threading)(RedEncodingThreading * threading,H264DataItem * dataitem);
   H264DataItem * (*remove_h264_slice_from_encoding_threading)(RedEncodingThreading * threading);
   void (*uninit_spice_encoding_threading)(RedEncodingThreading * threading);
   RedBaseThreadingStatus (*get_spice_encoding_threading_status)(RedEncodingThreading * threading);
   void (*read_bits_with_cb_to_encoding_threading)(RedEncodingThreading* threading,SpiceRect *bbox);
   void (*surface_ready_encoding_threading)(RedEncodingThreading * threading);
   void (*insert_vedio_data_in_encoding_threading)(RedEncodingThreading* threading,char* viedo_data);

};

RedEncodingThreading * encoding_threading_new();
void encoding_threading_delete(RedEncodingThreading * threading);
void free_h264_data_item(H264DataItem *h264_dataitem);


#endif
