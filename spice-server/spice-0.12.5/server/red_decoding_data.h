#ifndef RED_DECODING_DATA_H
#define RED_DECODING_DATA_H

#include <pthread.h>
#include <stdbool.h>



#include "common/rect.h"
#include "common/region.h"
#include "common/ring.h"
#include "red_threading.h"
#include "h264_encoder.h"
#include "red_spice_common_defines.h"

#define RED_SPICE_DECODING_THREADING_INIT(threading) do{\
   RED_SPICE_BASE_THREADING_INIT(threading->tbase);\
   threading->init_spice_decoding_threading = init_spice_decoding_threading;\
   threading->init_spice_decoding_threading_data = init_spice_decoding_threading_data;\
   threading->set_spice_decoding_threading_run = set_spice_decoding_threading_run;\
   threading->start_spice_decoding_threading = start_spice_decoding_threading;\
   threading->stop_wait_spice_decoding_threading = stop_wait_spice_decoding_threading;\
   threading->copy_bits_to_decoding_threading = copy_bits_to_decoding_threading;\
   threading->insert_h264_slice_in_decoding_threading = insert_h264_slice_in_decoding_threading;\
   threading->get_h264_slice_from_decoding_threading = get_h264_slice_from_decoding_threading;\
   threading->uninit_spice_decoding_threading = uninit_spice_decoding_threading;\
}while(0)

typedef struct RedDecodingThreading RedDecodingThreading;
typedef struct SpiceDecodingData SpiceDecodingData;
typedef struct H264DataItem H264DataItem;

struct SpiceDecodingData
{
   char * bits; 
   int max_bits_size;
   SpiceRect bbox;
   enum AVPixelFormat pix_fmt;
};


struct H264DataItem
{
   RingItem  item;
   char * slice;
   int size;
   uint32_t frame_mm_time;
};

struct RedDecodingThreading
{
   RedBaseThreading *tbase;
   int fd[2];
   void *worker;
   pthread_mutex_t _lock;
   SpiceDecodingData g_decoding_data; 
   
   LCXCALLBACK job;
   
   Ring h264_data_list;
   int list_size;

   H264Encoder *g_h264_encoder;

   //interface
   void (*init_spice_decoding_threading)(RedDecodingThreading * threading,void *worker,int bits_size,SpiceRect *bbox,LCXCALLBACK *job,enum AVPixelFormat pix_fmt,int fr,int br);
   void (*init_spice_decoding_threading_data)(RedDecodingThreading* threading,int bits_size,SpiceRect *bbox);
   void (*set_spice_decoding_threading_run)(RedDecodingThreading * threading,void* (*start_rtn)(void*));
   void (*start_spice_decoding_threading)(RedDecodingThreading * threading);
   void (*stop_wait_spice_decoding_threading)(RedDecodingThreading * threading);
   void (*copy_bits_to_decoding_threading)(RedDecodingThreading * threading,char* bits,int size,SpiceRect *bbox);
   void (*insert_h264_slice_in_decoding_threading)(RedDecodingThreading * threading,char* slice,int size);
   void (*get_h264_slice_from_decoding_threading)(RedDecodingThreading * threading,char ** slice , int *size);
   void (*uninit_spice_decoding_threading)(RedDecodingThreading * threading);

};

RedDecodingThreading * decoding_threading_new();
void decoding_threading_delete(RedDecodingThreading * threading);


void init_spice_decoding_threading(RedDecodingThreading * threading,void *worker,int bits_size,SpiceRect *bbox,LCXCALLBACK *job,enum AVPixelFormat pix_fmt,int fr,int br);
void init_spice_decoding_threading_data(RedDecodingThreading* threading,int bits_size,SpiceRect *bbox);
void set_spice_decoding_threading_run(RedDecodingThreading * threading,void* (*start_rtn)(void*));
void start_spice_decoding_threading(RedDecodingThreading * threading);
void stop_wait_spice_decoding_threading(RedDecodingThreading * threading);
void copy_bits_to_decoding_threading(RedDecodingThreading * threading,char* bits,int size,SpiceRect *bbox);
void insert_h264_slice_in_decoding_threading(RedDecodingThreading * threading,char* slice,int size);
void get_h264_slice_from_decoding_threading(RedDecodingThreading * threading,char * slice , int *size);
void uninit_spice_decoding_threading(RedDecodingThreading * threading);


#endif
