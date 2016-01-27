#ifndef h264_encoder_h
#define h264_encoder_h

/*********************************************************************************************************
 * 
 * 说明：此文件为h264编码接口
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


#include <math.h>
#include <assert.h>

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include "libswscale/swscale.h"

#include "h264_type.h"

//add by lcx
#define BIT_RATE  1024 * 1024 * 2 
#define BIT_RATE_MAX 1024 * 1024 * 20
#define FRAME_RATE 25 

//#define RC_BUFFER_SIZE_FF ( (float) (BIT_RATE) / 6000000 * 56 * 1024 * 8) //6M时为56kbytes
#define RC_BUFFER_SIZE_FF ( (float) (BIT_RATE_MAX) / 6000000 * 56 * 1024 * 8) //6M时为56kbytes
#define GOP_SIZE 200 
#define MAX_B_FRAME 10 
#define THREAD_COUNT 9 

#ifndef OUTPUT_BUF_SIZE_H264
#define OUTPUT_BUF_SIZE_H264  4096 * 4096 * 2
#endif 
#ifndef MAX_RGB_SIZE
#define MAX_RGB_SIZE 1920 * 1080 * 4 * 9 
#endif


typedef struct _H264Encoder
{
   AVCodecContext *codec_context;
   enum AVPixelFormat rgb_fmt;
   AVFrame *rgbframe;
   AVFrame *yuvframe;
   struct SwsContext *sws_context;
}H264Encoder;


void  h264_encoder_env_init();

int h264_encoder_init(H264Encoder * encoder,H264StreamInfo * info);

H264Encoder * h264_encoder_new();
void h264_encoder_delete(H264Encoder * encoder);

int h264_encode(H264Encoder * encoder,char * rgb,int rgb_len,char *slice ,int *slice_len);

void h264_encoder_uninit(H264Encoder * encoder);

int h264_encoder_reinit(H264Encoder * encoder,H264StreamInfo * info);

void h264_encoder_get_stream_info(H264Encoder * encoder,H264StreamInfo * info);

int h264_encoder_get_bits_per_pixel(H264Encoder * encoder);

int h264_encoder_get_bytes_per_pixel(H264Encoder * encoder);


#endif
