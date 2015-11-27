#ifndef h264_encoder_h
#define h264_encoder_h
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

int h264_encode(H264Encoder * encoder,char * rgb,int rgb_len,char *slice ,int *slice_len);

void h264_encoder_uninit(H264Encoder * encoder);

int h264_encoder_reinit(H264Encoder * encoder,H264StreamInfo * info);

void h264_encoder_get_stream_info(H264Encoder * encoder,H264StreamInfo * info);

int h264_encoder_get_bits_per_pixel(H264Encoder * encoder);

int h264_encoder_get_bytes_per_pixel(H264Encoder * encoder);


#endif
