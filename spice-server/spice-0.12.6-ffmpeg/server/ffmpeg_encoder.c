#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
#include <stdbool.h>

#include "video_encoder.h"


extern void av_register_all(void); 
extern void avcodec_register_all(void);

typedef struct FFMpegEncoder {
    VideoEncoder base;

    /* Rate control callbacks */
    VideoEncoderRateControlCbs cbs;
    void *cbs_opaque;

    /* ---------- Video characteristics ---------- */
    int width;
    int height;
    uint64_t bit_rate;
    
    //context
    AVCodecContext* context;
    AVFrame* rgb_frame;
    AVFrame* yuv_frame;
    struct SwsContext* yuv_convertor; 
    
}FFMpegEncoder;

static void ffmpeg_free_video_buffer(VideoBuffer *video_buffer)
{   
		if(NULL != video_buffer->data){
			free(video_buffer->data);
		}
    free(video_buffer);
}

static VideoBuffer* ffmpeg_create_video_buffer(int size)
{
    VideoBuffer *buffer = spice_new0(VideoBuffer, 1);
    if( 0 != size)
    {
    	buffer->data = spice_malloc(size);
    	buffer->size = size;
    }
    else{
	    buffer->data = NULL;
  	  buffer->size = 0;
    }
    buffer->free = &ffmpeg_free_video_buffer;
    return buffer;
}

/* Releases the video encoder's resources */
static void ffmpeg_destroy(VideoEncoder *video_encoder){
	FFMpegEncoder* encoder = (FFMpegEncoder*)video_encoder;
	if (encoder->yuv_frame)
  {
      av_freep(&encoder->yuv_frame->data[0]);
      av_frame_free(&encoder->yuv_frame);
  }
  if (encoder->rgb_frame)
  {
      av_frame_free(&encoder->rgb_frame);
  }
  if(encoder->yuv_convertor)
  {
      sws_freeContext(encoder->yuv_convertor);
  }

  if(!encoder->context)
  {
      avcodec_close(encoder->context);
      av_free(encoder->context);
  }
  spice_info("ffmpeg encoder %p destroyed", encoder);
  free(encoder);
}

    /* Compresses the specified src image area into the outbuf buffer.
     *
     * @encoder:   The video encoder.
     * @bitmap:    The Spice screen.
     * @width:     The width of the video area. This always matches src.
     * @height:    The height of the video area. This always matches src.
     * @src:       A rectangle specifying the area occupied by the video.
     * @top_down:  If true the first video line is specified by src.top.
     * @buffer:    A pointer to a VideoBuffer structure containing the
     *             compressed frame if successful. Call the buffer's free()
     *             method as soon as it is no longer needed.
     * @return:
     *     VIDEO_ENCODER_FRAME_ENCODE_DONE if successful.
     *     VIDEO_ENCODER_FRAME_UNSUPPORTED if the frame cannot be encoded.
     *     VIDEO_ENCODER_FRAME_DROP if the frame was dropped. This value can
     *                              only happen if rate control is active.
     */
static int ffmpeg_encode_frame(VideoEncoder *video_encoder, const SpiceBitmap *bitmap,
                        int width, int height, const SpiceRect *src,
                        int top_down, uint32_t frame_mm_time,
                        VideoBuffer** buffer)
{
	FFMpegEncoder* encoder = (FFMpegEncoder*)video_encoder;
	
	//force bitmap format
	if((SPICE_BITMAP_FMT_32BIT != bitmap->format) &&(SPICE_BITMAP_FMT_RGBA != bitmap->format)){
		spice_error("unsupport bitmap format %d", bitmap->format);
		return VIDEO_ENCODER_FRAME_UNSUPPORTED;
	}
	if( (width != bitmap->x) || (height != bitmap->y )){
		spice_error("only fullscreen bitmap allowed");
		return VIDEO_ENCODER_FRAME_UNSUPPORTED;
	}
	//adopt resolution
	if( (0 == encoder->width) &&(0 == encoder->height))
	{
		encoder->width = width;
		encoder->height = height;
		encoder->context->width = width;
		encoder->context->height = height;		
    AVDictionary *opts = NULL;
    if (avcodec_open2(encoder->context, encoder->context->codec, &opts) < 0) 
    {
        spice_error("open avcodec fail");
				return VIDEO_ENCODER_FRAME_UNSUPPORTED;
    }
    encoder->rgb_frame = av_frame_alloc();
    if(!encoder->rgb_frame)
    { 
        spice_error("allocate rgb frame fail");
				return VIDEO_ENCODER_FRAME_UNSUPPORTED;
    }
    encoder->yuv_frame = av_frame_alloc();
    if (!encoder->yuv_frame)
    {
    		av_frame_free(&encoder->rgb_frame);
        spice_error("allocate yuv frame fail");
				return VIDEO_ENCODER_FRAME_UNSUPPORTED;
    }
    encoder->rgb_frame->width = width;
    encoder->rgb_frame->height = height;
    encoder->rgb_frame->format = AV_PIX_FMT_BGRA ;//fixed

    encoder->yuv_frame->width = width;
    encoder->yuv_frame->height = height;
    encoder->yuv_frame->format = AV_PIX_FMT_YUV420P;
    if(av_image_alloc(encoder->yuv_frame->data, encoder->yuv_frame->linesize, width, height, AV_PIX_FMT_YUV420P, 24) < 0){
    	av_frame_free(&encoder->rgb_frame);
    	av_frame_free(&encoder->yuv_frame);
    	spice_error("allocate yuv image fail");
			return VIDEO_ENCODER_FRAME_UNSUPPORTED;
    }
    encoder->rgb_frame->pts = 0;
    encoder->yuv_frame->pts = 0;
    if(NULL == (encoder->yuv_convertor = sws_getContext(width, height, AV_PIX_FMT_BGRA, 
    																										width, height, AV_PIX_FMT_YUV420P, 
                    																		SWS_BICUBIC, NULL, NULL, NULL)))
    {
    	av_freep(&encoder->yuv_frame->data[0]);
    	av_frame_free(&encoder->rgb_frame);
    	av_frame_free(&encoder->yuv_frame);
    	spice_error("allocate yuv convertor fail");
			return VIDEO_ENCODER_FRAME_UNSUPPORTED;
    }
		spice_info("new resolution adopted, %d x %d", width, height);
	}
	if( (encoder->width != width) || (encoder->height != height)){
		spice_error("invalid bitmap region %d x %d, must be %d x %d", width, height, encoder->width, encoder->height);
		return VIDEO_ENCODER_FRAME_UNSUPPORTED;
	}
	int filled_bytes = av_image_fill_arrays(encoder->rgb_frame->data, encoder->rgb_frame->linesize, bitmap->data->chunk[0].data, AV_PIX_FMT_BGRA, width, height, 4);
	if(filled_bytes < 0){
		spice_error("fill rgb frame fail");
		return VIDEO_ENCODER_FRAME_UNSUPPORTED;
	}
	int converted_height = sws_scale(encoder->yuv_convertor, (const uint8_t * const*)encoder->rgb_frame->data, encoder->rgb_frame->linesize, 0, height, encoder->yuv_frame->data, encoder->yuv_frame->linesize);
	if(converted_height != height){
		spice_error("not all rgb frame converted, %d/ %d", converted_height, height);
		return VIDEO_ENCODER_FRAME_UNSUPPORTED;
	}
	if (encoder->yuv_frame->pts == 0)
  {
      encoder->yuv_frame->key_frame = 1;
  }
  encoder->yuv_frame->pts++;
  AVPacket avpacket;
  av_init_packet(&avpacket);
  avpacket.data = NULL;//set allocate by encoder
  avpacket.size = 0;
  int has_ouput = 0;
  //encode
  if(0 != avcodec_encode_video2(encoder->context, &avpacket,encoder->yuv_frame, &has_ouput)){
  	//encode fail
  	spice_error("encode frame fail");
		return VIDEO_ENCODER_FRAME_UNSUPPORTED;
  }
  //success
  if(0 != has_ouput){
  	//copy 
  	(*buffer) = ffmpeg_create_video_buffer(avpacket.size);
  	//safe copy, use pointer swap instead? akumas
  	memcpy((*buffer)->data, avpacket.data, avpacket.size);  	
  }
  av_packet_unref(&avpacket);
  return VIDEO_ENCODER_FRAME_ENCODE_DONE;	
}

    /*
     * Bit rate control methods.
     */

    /* When rate control is active statistics are periodically obtained from
     * the client and sent to the video encoder through this method.
     *
     * @encoder:    The video encoder.
     * @num_frames: The number of frames that reached the client during the
     *              time period the report is referring to.
     * @num_drops:  The part of the above frames that was dropped by the client
     *              due to late arrival time.
     * @start_frame_mm_time: The mm_time of the first frame included in the
     *              report.
     * @end_frame_mm_time: The mm_time of the last frame included in the report.
     * @end_frame_delay: This indicates how long in advance the client received
     *              the last frame before having to display it.
     * @audio delay: The latency of the audio playback or MAX_UINT if it is not
     *              tracked.
     */
static void ffmpeg_client_stream_report(VideoEncoder *encoder,
                                 uint32_t num_frames, uint32_t num_drops,
                                 uint32_t start_frame_mm_time,
                                 uint32_t end_frame_mm_time,
                                 int32_t end_frame_delay, uint32_t audio_delay)
{
	//spice_warning("client stream report not implement");
}

    /* This notifies the video encoder each time a frame is dropped due to pipe
     * congestion.
     *
     * Note that frames are being dropped before they are encoded and that there
     * may be any number of encoded frames in the network queue.
     * The client reports provide richer and typically more reactive information
     * for fine tuning the playback parameters but this function provides a
     * fallback when client reports are getting delayed or are not supported
     * by the client.
     *
     * @encoder:    The video encoder.
     */
static void ffmpeg_notify_server_frame_drop(VideoEncoder *encoder){
	//spice_warning("notify server frame drop not implement");
}

    /* This queries the video encoder's current bit rate.
     *
     * @encoder:    The video encoder.
     * @return:     The current bit rate in bits per second.
     */
static uint64_t ffmpeg_get_bit_rate(VideoEncoder *video_encoder){
	FFMpegEncoder *encoder = (FFMpegEncoder*)video_encoder;
	return encoder->bit_rate;
	
}

    /* Collects video statistics.
     *
     * @encoder:    The video encoder.
     * @stats:      A VideoEncoderStats structure to fill with the collected
     *              statistics.
     */
static void ffmpeg_get_stats(VideoEncoder *encoder, VideoEncoderStats *stats){
	spice_warning("get stats not implement");
}

VideoEncoder* ffmpeg_encoder_new(SpiceVideoCodecType codec_type,
                                	uint64_t starting_bit_rate,
                                	VideoEncoderRateControlCbs *cbs,
                                	void *cbs_opaque)
{
	{
		//global initial
		static bool service_initialed = false;
		if(!service_initialed){
			av_register_all(); 
			avcodec_register_all();
			service_initialed = true;
		}		
	}
	if(codec_type != SPICE_VIDEO_CODEC_TYPE_H264){
		spice_error("unsupported video codec %d", codec_type);
		return NULL;
	}
	
	FFMpegEncoder *encoder = spice_new0(FFMpegEncoder, 1);
  encoder->base.destroy = &ffmpeg_destroy;
  encoder->base.encode_frame = &ffmpeg_encode_frame;
  encoder->base.client_stream_report = &ffmpeg_client_stream_report;
  encoder->base.notify_server_frame_drop = &ffmpeg_notify_server_frame_drop;
  encoder->base.get_bit_rate = &ffmpeg_get_bit_rate;
  encoder->base.get_stats = &ffmpeg_get_stats;
  encoder->base.codec_type = codec_type;
  
  if (cbs) {
      encoder->cbs = *cbs;
      encoder->cbs_opaque = cbs_opaque;
  }
  encoder->bit_rate = starting_bit_rate;
  encoder->width = 0;
  encoder->height = 0;
  AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if(!codec){
  	spice_error("can't find H264 encoder");
  	return NULL;
  }
  encoder->context = avcodec_alloc_context3(codec);
  if (!encoder->context)
  {  	
  	spice_error("allocate codec context fail");
  	return NULL;
  }
  enum {
  	default_bit_rate = 2 * 1024 * 1024, //2 mb
  	default_max_rate = 4 * 1024 * 1024 , //4mb
  	default_min_rate = 256 * 1024 , //256kb
  	default_gop_size = 200,
  	default_max_b_frames = 3,
  	default_thread_count = 1,
  	default_fps = 25,
  	default_qmin = 18,
  	default_qmax = 28,
  };
  const float default_qcompress = 0.5;
  const float default_qblur = 0.5;
  
  AVRational time_base = { 1, default_fps};
  
  //CRF
	encoder->context->bit_rate = default_bit_rate;
  encoder->context->rc_max_rate = default_max_rate;
  encoder->context->rc_min_rate = default_min_rate;
  encoder->context->bit_rate_tolerance = default_bit_rate;
  //decoder bitstream buffer size
  encoder->context->rc_buffer_size = default_max_rate * 2;
  
  //VBR
  encoder->context->qcompress = default_qcompress;
  encoder->context->qblur = default_qblur;
  encoder->context->qmin = default_qmin;
  encoder->context->qmax = default_qmax;
  
  
  //B frames
  encoder->context->b_frame_strategy = 1;//Fast but less accurate
  encoder->context->max_b_frames = default_max_b_frames;
  
  encoder->context->width = 0;
  encoder->context->height = 0;
  encoder->context->time_base = time_base;
  encoder->context->gop_size = default_gop_size; 
  encoder->context->thread_count = default_thread_count;
  encoder->context->profile = FF_PROFILE_H264_BASELINE;
  encoder->context->pix_fmt = AV_PIX_FMT_YUV420P;
  encoder->context->flags |= CODEC_FLAG_QSCALE;
  
  av_opt_set(encoder->context->priv_data, "preset", "superfast", 0);
  av_opt_set(encoder->context->priv_data, "tune", "zerolatency", 0);
  
  
  spice_info("FFMpeg encoder created, bit rate: %d ( %d ~ %d ), buffer size %d, max B frames %d, thread %d",
  					(int)encoder->context->bit_rate, (int)encoder->context->rc_max_rate, (int)encoder->context->rc_min_rate, 
  					(int)encoder->context->rc_buffer_size, (int)encoder->context->max_b_frames, (int)encoder->context->thread_count);  				
	return (VideoEncoder*)encoder;
}

