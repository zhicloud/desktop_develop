#include "config.h"

#include "spice-client.h"
#include "spice-common.h"
#include "spice-channel-priv.h"

#include "channel-display-priv.h"

#define DEFAULT_MAX_FRAME_SIZE 1920 * 1080 * 4

void stream_h264_init(display_stream* st)
{
    int ret = 0;

    h264_decoder_env_init();

    st->h264_info = (H264StreamInfo*)malloc(sizeof(H264StreamInfo));
    if(st->h264_info == NULL)
    {
        printf("allocation memory for struct H264StreamInfo failed!\n");
        return;
    }    
    memset(st->h264_info, 0, sizeof(H264StreamInfo));
    st->h264_info->pix_fmt = AV_PIX_FMT_BGRA;                              //PIX_FMT_BGR24;

    st->h264_decoder = h264_decoder_new();
    if(st->h264_decoder == NULL)
    {
        printf("allocation memory for struct H264Decoder failed!\n");
        return;
    }
 
    ret=  h264_decoder_init(st->h264_decoder, st->h264_info);
    if(H264_ERROR == ret)      
    {
        printf("init decoder failed!\n");
        return;
    } 
}

void stream_h264_data(display_stream* st, SpiceRect* rc)
{
    int width = 0;
    int height = 0;
    int hpp = 0;
    int ret = 0;
    
    char* rgb_frame = NULL;
    int rgb_frame_size = 0;

    char* h264_frame = NULL;
    int* h264_frame_size = NULL;

    if(st->msg_data->psize <= 0)
    {
       printf("msg_data' s psize <= 0\n");
       return;   
    }

    h264_frame = (char*)(st->msg_data->data + 12);
    h264_frame_size =(int*)(st->msg_data->data + 8);   


    rgb_frame = (char*)g_malloc0(DEFAULT_MAX_FRAME_SIZE);
    ret = h264_decode(st->h264_decoder, rgb_frame, &rgb_frame_size, h264_frame, *h264_frame_size, &width, &height, &hpp);
    if(ret == 0)
        st->out_frame = (uint8_t*)rgb_frame;
    else
        g_free(rgb_frame);
}

void stream_h264_cleanup(display_stream* st)
{
   if(st->h264_decoder != NULL)
   {
       h264_decoder_uninit(st->h264_decoder);
       h264_decoder_release(st->h264_decoder);
       st->h264_decoder = NULL;
   }

   if(st->h264_info != NULL)
   {
      free(st->h264_info);
      st->h264_info = NULL;
   }
}
