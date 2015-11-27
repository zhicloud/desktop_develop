#include "h264_encoder.h"

void  h264_encoder_env_init()
{
    H264_ENV_INIT(H264_VERSION);
}

int h264_encoder_init(H264Encoder * encoder,H264StreamInfo * info)
{
    assert(encoder != NULL);
    assert(info != NULL);
    
    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        goto last_err;
    }
    encoder->codec_context = avcodec_alloc_context3(codec);
    if (!encoder->codec_context)
    {
        goto last_err;
    }
    encoder->rgb_fmt = info->s_pix_fmt; 

    encoder->codec_context->bit_rate = info->bit_rate;
    encoder->codec_context->rc_max_rate = info->rc_max_rate;
    encoder->codec_context->rc_min_rate = info->rc_min_rate;
    encoder->codec_context->rc_buffer_size = info->rc_buffer_size;
    encoder->codec_context->width = info->width;
    encoder->codec_context->height = info->height;
    encoder->codec_context->time_base = info->time_base;
    encoder->codec_context->gop_size = info->gop_size; 
    encoder->codec_context->max_b_frames = info->max_b_frames;
    encoder->codec_context->thread_count = info->thread_count;
    encoder->codec_context->profile = info->profile;
    encoder->codec_context->pix_fmt = info->pix_fmt;
    //encoder->codec_context->qmin = info->qmin;
    //encoder->codec_context->qmax = info->qmax;
    //encoder->codec_context->qcompress = info->qcompress;
    encoder->codec_context->flags |= CODEC_FLAG_QSCALE;
    av_opt_set(encoder->codec_context->priv_data, "preset", "superfast", 0);
    av_opt_set(encoder->codec_context->priv_data, "tune", "zerolatency", 0);
    AVDictionary *opts = NULL;
    //av_dict_set(&opts, "profile", "baseline", 0); /* open the codec */ 
    if (avcodec_open2(encoder->codec_context, codec, &opts) < 0) 
    //if (avcodec_open2(encoder->codec_context, codec, ) < 0)
    {
        goto codec_open_err;
    }
    encoder->rgbframe = av_frame_alloc();
    if(!encoder->rgbframe)
    { 
        goto do_err;
    }
    encoder->yuvframe = av_frame_alloc();
    if (!encoder->yuvframe)
    {
        goto do_err;
    }
    encoder->rgbframe->width = encoder->codec_context->width;
    encoder->rgbframe->height = encoder->codec_context->height;
    encoder->rgbframe->format = info->s_pix_fmt;

    encoder->yuvframe->width = encoder->codec_context->width;
    encoder->yuvframe->height = encoder->codec_context->height;
    encoder->yuvframe->format = encoder->codec_context->pix_fmt;
    int ret = av_image_alloc(encoder->yuvframe->data, encoder->yuvframe->linesize, encoder->codec_context->width,
                    encoder->codec_context->height, encoder->codec_context->pix_fmt, 24);

    encoder->yuvframe->pts = 0;
    encoder->rgbframe->pts = 0;
    encoder->sws_context = sws_getContext(encoder->codec_context->width, encoder->codec_context->height, info->s_pix_fmt,
                    encoder->codec_context->width, encoder->codec_context->height, encoder->codec_context->pix_fmt, 
                    SWS_BICUBIC, NULL, NULL, NULL);
    return H264_OK;
do_err:
    if (encoder->yuvframe)
    {
        av_freep(&encoder->yuvframe->data[0]);
        av_frame_free(&encoder->yuvframe);
    }
    if (encoder->rgbframe)
    {
        av_frame_free(&encoder->rgbframe);
    }
    if(encoder->sws_context)
    {
        sws_freeContext(encoder->sws_context);
    }

    if(!encoder->codec_context)
    {
        avcodec_close(encoder->codec_context);
    }
codec_open_err:
    
    if(!encoder->codec_context)
    {
        av_free(encoder->codec_context);
    }
last_err:
    encoder->yuvframe = NULL;
    encoder->rgbframe = NULL;
    encoder->sws_context = NULL;
    encoder->codec_context = NULL;
    
    return H264_ERROR;
}

H264Encoder * h264_encoder_new()
{
    H264Encoder * encoder = NULL;
    encoder = (H264Encoder *)malloc(sizeof(H264Encoder));
    if(!encoder)
        return NULL;
    memset(encoder,0,sizeof(H264Encoder));
    return encoder;
}


int h264_encode(H264Encoder * encoder,char * rgb,int rgb_len,char *slice,int *slice_len)
{
    int got = 0, ret = 0;
    AVPacket avpkt;
    if (rgb)
    {
        avpicture_fill((AVPicture*)encoder->rgbframe, rgb, encoder->rgbframe->format, encoder->codec_context->width, 
                encoder->codec_context->height);
        sws_scale(encoder->sws_context, (const uint8_t * const*)encoder->rgbframe->data, encoder->rgbframe->linesize,0,
                encoder->codec_context->height, encoder->yuvframe->data, encoder->yuvframe->linesize);
    }
    if (encoder->yuvframe->pts == 0)
    {
        encoder->yuvframe->key_frame = 1;
    }
    encoder->yuvframe->pts++;
    av_init_packet(&avpkt);

    avpkt.data = slice;
    avpkt.size = *slice_len;
    if (rgb)
    {
        ret = avcodec_encode_video2(encoder->codec_context, &avpkt,encoder->yuvframe, &got);
    }
    else 
    {
        ret = avcodec_encode_video2(encoder->codec_context, &avpkt,NULL, &got);
    }
    if(ret < 0)
    {
        return H264_ENCODER_ERROR;
    }
    if(got)
    {
        *slice_len = avpkt.size;
        return H264_ENCODER_OK;
    }
    else
    {
        *slice_len = 0;
        return  H264_NOT_GET_SLICE;
    }
}


void h264_encoder_uninit(H264Encoder * encoder)
{
    if(encoder->sws_context)
    {
        sws_freeContext(encoder->sws_context);
    }
    if(encoder->codec_context)
    {
        avcodec_close(encoder->codec_context);
        av_free(encoder->codec_context);
    }
    if (encoder->yuvframe)
    {
        av_freep(&encoder->yuvframe->data[0]);
        av_frame_free(&encoder->yuvframe);
    }
    if (encoder->rgbframe)
    {
        av_frame_free(&encoder->rgbframe);
    }
    encoder->yuvframe = NULL;
    encoder->rgbframe = NULL;
    encoder->sws_context = NULL;
    encoder->codec_context = NULL;

}

int h264_encoder_reinit(H264Encoder * encoder,H264StreamInfo * info)
{

    h264_encoder_uninit(encoder);
    return h264_encoder_init(encoder,info);
}

void h264_encoder_get_stream_info(H264Encoder * encoder,H264StreamInfo * info)
{
    info->bit_rate = encoder->codec_context->bit_rate;
    info->rc_max_rate = encoder->codec_context->rc_max_rate;
    info->rc_min_rate = encoder->codec_context->rc_min_rate;
    info->rc_buffer_size = encoder->codec_context->rc_buffer_size;
    info->width = encoder->codec_context->width;
    info->height = encoder->codec_context->height;
    info->time_base = encoder->codec_context->time_base;
    info->gop_size = encoder->codec_context->gop_size; 
    info->max_b_frames = encoder->codec_context->max_b_frames;
    info->thread_count = encoder->codec_context->thread_count;
    info->profile = encoder->codec_context->profile;
    info->pix_fmt = encoder->codec_context->pix_fmt;
    info->qmin = encoder->codec_context->qmin;
    info->qmax = encoder->codec_context->qmax;
    info->qcompress = encoder->codec_context->qcompress;
}

int h264_encoder_get_bits_per_pixel(H264Encoder * encoder)
{
    int pixel;
    GET_FORMAT_BITS(encoder->rgb_fmt,pixel);
    return pixel;


}

int h264_encoder_get_bytes_per_pixel(H264Encoder * encoder)
{
    int pixel;
    GET_FORMAT_BYTES(encoder->rgb_fmt,pixel);
    return pixel;


}






