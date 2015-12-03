/*
   Copyright (C) 2009-2015 Red Hat, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "stream.h"
#include "display-channel.h"

#define FPS_TEST_INTERVAL 1
#define FOREACH_STREAMS(display, item)                  \
    for (item = ring_get_head(&(display)->streams);     \
         item != NULL;                                  \
         item = ring_next(&(display)->streams, item))

void stream_agent_stats_print(StreamAgent *agent)
{
#ifdef STREAM_STATS
    StreamStats *stats = &agent->stats;
    double passed_mm_time = (stats->end - stats->start) / 1000.0;
    VideoEncoderStats encoder_stats = {0};

    if (agent->video_encoder) {
        agent->video_encoder->get_stats(agent->video_encoder, &encoder_stats);
    }

    spice_debug("stream=%p dim=(%dx%d) #in-frames=%lu #in-avg-fps=%.2f #out-frames=%lu "
                "out/in=%.2f #drops=%lu (#pipe=%lu #fps=%lu) out-avg-fps=%.2f "
                "passed-mm-time(sec)=%.2f size-total(MB)=%.2f size-per-sec(Mbps)=%.2f "
                "size-per-frame(KBpf)=%.2f avg-quality=%.2f "
                "start-bit-rate(Mbps)=%.2f end-bit-rate(Mbps)=%.2f",
                agent, agent->stream->width, agent->stream->height,
                stats->num_input_frames,
                stats->num_input_frames / passed_mm_time,
                stats->num_frames_sent,
                (stats->num_frames_sent + 0.0) / stats->num_input_frames,
                stats->num_drops_pipe +
                stats->num_drops_fps,
                stats->num_drops_pipe,
                stats->num_drops_fps,
                stats->num_frames_sent / passed_mm_time,
                passed_mm_time,
                stats->size_sent / 1024.0 / 1024.0,
                ((stats->size_sent * 8.0) / (1024.0 * 1024)) / passed_mm_time,
                stats->size_sent / 1000.0 / stats->num_frames_sent,
                encoder_stats.avg_quality,
                encoder_stats.starting_bit_rate / (1024.0 * 1024),
                encoder_stats.cur_bit_rate / (1024.0 * 1024));
#endif
}

void stream_stop(DisplayChannel *display, Stream *stream)
{
    DisplayChannelClient *dcc;
    RingItem *item, *next;

    spice_return_if_fail(ring_item_is_linked(&stream->link));
    spice_return_if_fail(!stream->current);

    spice_debug("stream %d", get_stream_id(display, stream));
    FOREACH_DCC(display, item, next, dcc) {
        StreamAgent *stream_agent;

        stream_agent = &dcc->stream_agents[get_stream_id(display, stream)];
        region_clear(&stream_agent->vis_region);
        region_clear(&stream_agent->clip);
        spice_assert(!pipe_item_is_linked(&stream_agent->destroy_item));
        if (stream_agent->video_encoder && dcc->use_video_encoder_rate_control) {
            uint64_t stream_bit_rate = stream_agent->video_encoder->get_bit_rate(stream_agent->video_encoder);

            if (stream_bit_rate > dcc->streams_max_bit_rate) {
                spice_debug("old max-bit-rate=%.2f new=%.2f",
                dcc->streams_max_bit_rate / 8.0 / 1024.0 / 1024.0,
                stream_bit_rate / 8.0 / 1024.0 / 1024.0);
                dcc->streams_max_bit_rate = stream_bit_rate;
            }
        }
        stream->refs++;
        red_channel_client_pipe_add(RED_CHANNEL_CLIENT(dcc), &stream_agent->destroy_item);
        stream_agent_stats_print(stream_agent);
    }
    display->streams_size_total -= stream->width * stream->height;
    ring_remove(&stream->link);
    stream_unref(display, stream);
}

static void stream_free(DisplayChannel *display, Stream *stream)
{
    stream->next = display->free_streams;
    display->free_streams = stream;
}

void display_channel_init_streams(DisplayChannel *display)
{
    int i;

    ring_init(&display->streams);
    display->free_streams = NULL;
    for (i = 0; i < NUM_STREAMS; i++) {
        Stream *stream = &display->streams_buf[i];
        ring_item_init(&stream->link);
        stream_free(display, stream);
    }
    display->fullstream_id = 0;
}

void stream_unref(DisplayChannel *display, Stream *stream)
{
    if (--stream->refs != 0)
        return;

    spice_warn_if_fail(!ring_item_is_linked(&stream->link));

    stream_free(display, stream);
    display->stream_count--;
}

void stream_agent_unref(DisplayChannel *display, StreamAgent *agent)
{
    stream_unref(display, agent->stream);
}

//StreamClipItem *stream_clip_item_new(DisplayChannelClient* dcc, StreamAgent *agent)
//{
//    StreamClipItem *item = spice_new(StreamClipItem, 1);
//    red_channel_pipe_item_init(RED_CHANNEL_CLIENT(dcc)->channel,
//                               (PipeItem *)item, PIPE_ITEM_TYPE_STREAM_CLIP);
//
//    item->stream_agent = agent;
//    agent->stream->refs++;
//    item->refs = 1;
//    return item;
//}

//static int is_stream_start(Drawable *drawable)
//{
//    return ((drawable->frames_count >= RED_STREAM_FRAMES_START_CONDITION) &&
//            (drawable->gradual_frames_count >=
//             (RED_STREAM_GRADUAL_FRAMES_START_CONDITION * drawable->frames_count)));
//}

static void update_copy_graduality(DisplayChannel *display, Drawable *drawable)
{
    SpiceBitmap *bitmap;
    spice_return_if_fail(drawable->red_drawable->type == QXL_DRAW_COPY);

    if (display->stream_video != SPICE_STREAM_VIDEO_FILTER) {
        drawable->copy_bitmap_graduality = BITMAP_GRADUAL_INVALID;
        return;
    }

    if (drawable->copy_bitmap_graduality != BITMAP_GRADUAL_INVALID) {
        return; // already set
    }

    bitmap = &drawable->red_drawable->u.copy.src_bitmap->u.bitmap;

    if (!bitmap_fmt_has_graduality(bitmap->format) || bitmap_has_extra_stride(bitmap) ||
        (bitmap->data->flags & SPICE_CHUNKS_FLAGS_UNSTABLE)) {
        drawable->copy_bitmap_graduality = BITMAP_GRADUAL_NOT_AVAIL;
    } else  {
        drawable->copy_bitmap_graduality = bitmap_get_graduality_level(bitmap);
    }
}

static int is_next_stream_frame(DisplayChannel *display,
                                const Drawable *candidate,
                                const int other_src_width,
                                const int other_src_height,
                                const SpiceRect *other_dest,
                                const red_time_t other_time,
                                const Stream *stream,
                                int container_candidate_allowed)
{
    RedDrawable *red_drawable;
    int is_frame_container = FALSE;
		
    if (!candidate->streamable) {
        return STREAM_FRAME_NONE;
    }

//    if (candidate->creation_time - other_time >
//            (stream ? RED_STREAM_CONTINUS_MAX_DELTA : RED_STREAM_DETACTION_MAX_DELTA)) {
//        if(stream)
//        	spice_info("create time expire for stream %d", (int)(stream - display->streams_buf));
//        return STREAM_FRAME_NONE;
//    }

    red_drawable = candidate->red_drawable;
    if (!rect_contains(other_dest, &red_drawable->bbox)){
    	if(stream)
    	{
    		spice_info("not in range for stream %d, source (%d, %d) ( %d, %d) to dest (%d, %d) (%d, %d)", 
      						(int)(stream - display->streams_buf), 
      						red_drawable->bbox.left, red_drawable->bbox.top, red_drawable->bbox.right, red_drawable->bbox.bottom, 
      						other_dest->left, other_dest->top, other_dest->right, other_dest->bottom);
      }
    	return STREAM_FRAME_NONE;
    }
//    
//    if (!container_candidate_allowed) {
//        SpiceRect* candidate_src;
//
//        if (!rect_is_equal(&red_drawable->bbox, other_dest)) {
//        		if(stream)
//        			spice_info("rect is not equal for stream %d", (int)(stream - display->streams_buf));
//            return STREAM_FRAME_NONE;
//        }
//
//        candidate_src = &red_drawable->u.copy.src_area;
//        if (candidate_src->right - candidate_src->left != other_src_width ||
//            candidate_src->bottom - candidate_src->top != other_src_height) {
//            if(stream)
//            	spice_info("source size unmatched for stream %d", (int)(stream - display->streams_buf));
//            return STREAM_FRAME_NONE;
//        }
//    } else {
//        if (rect_contains(&red_drawable->bbox, other_dest)) {
//            int candidate_area = rect_get_area(&red_drawable->bbox);
//            int other_area = rect_get_area(other_dest);
//            /* do not stream drawables that are significantly
//             * bigger than the original frame */
//            if (candidate_area > 2 * other_area) {
//                spice_debug("too big candidate:");
//                spice_debug("prev box ==>");
//                rect_debug(other_dest);
//                spice_debug("new box ==>");
//                rect_debug(&red_drawable->bbox);
//                if(stream)
//                	spice_info("drawable is too large for stream %d", (int)(stream - display->streams_buf));
//                return STREAM_FRAME_NONE;
//            }
//
//            if (candidate_area > other_area) {
//                is_frame_container = TRUE;
//            }
//        } else {
//        		if(stream)
//        		{
//        			spice_info("not in range for stream %d, source (%d, %d) ( %d, %d) to dest (%d, %d) (%d, %d)", 
//        								(int)(stream - display->streams_buf), 
//        								red_drawable->bbox.left, red_drawable->bbox.top, red_drawable->bbox.right, red_drawable->bbox.bottom, 
//        								other_dest->left, other_dest->top, other_dest->right, other_dest->bottom);
//        		}
//        			
//            return STREAM_FRAME_NONE;
//        }
//    }

    if (stream) {
        SpiceBitmap *bitmap = &red_drawable->u.copy.src_bitmap->u.bitmap;
        if (stream->top_down != !!(bitmap->flags & SPICE_BITMAP_FLAGS_TOP_DOWN)) {
        		if(stream)
        			spice_info("top down property unmatched for stream %d", (int)(stream - display->streams_buf));
            return STREAM_FRAME_NONE;
        }
    }
    if (is_frame_container) {
        return STREAM_FRAME_CONTAINER;
    } else {
        return STREAM_FRAME_NATIVE;
    }
}

static void before_reattach_stream(DisplayChannel *display,
                                   Stream *stream, Drawable *new_frame)
{
    DrawablePipeItem *dpi;
    DisplayChannelClient *dcc;
    int index;
    StreamAgent *agent;
    RingItem *ring_item, *next;

    spice_return_if_fail(stream->current);

    if (!red_channel_is_connected(RED_CHANNEL(display))) {
        return;
    }

    if (new_frame->process_commands_generation == stream->current->process_commands_generation) {
        spice_debug("ignoring drop, same process_commands_generation as previous frame");
        return;
    }

    index = get_stream_id(display, stream);
    DRAWABLE_FOREACH_DPI_SAFE(stream->current, ring_item, next, dpi) {
        dcc = dpi->dcc;
        agent = &dcc->stream_agents[index];

        if (!dcc->use_video_encoder_rate_control &&
            !dcc->common.is_low_bandwidth) {
            continue;
        }

        if (pipe_item_is_linked(&dpi->dpi_pipe_item)) {
#ifdef STREAM_STATS
            agent->stats.num_drops_pipe++;
#endif
            if (dcc->use_video_encoder_rate_control) {
                agent->video_encoder->notify_server_frame_drop(agent->video_encoder);
            } else {
                ++agent->drops;
            }
        }
    }


    FOREACH_DCC(display, ring_item, next, dcc) {
        double drop_factor;

        agent = &dcc->stream_agents[index];

        if (dcc->use_video_encoder_rate_control) {
            continue;
        }
        if (agent->frames / agent->fps < FPS_TEST_INTERVAL) {
            agent->frames++;
            continue;
        }
        drop_factor = ((double)agent->frames - (double)agent->drops) /
            (double)agent->frames;
        spice_debug("stream %d: #frames %u #drops %u", index, agent->frames, agent->drops);
        if (drop_factor == 1) {
            if (agent->fps < MAX_FPS) {
                agent->fps++;
                spice_debug("stream %d: fps++ %u", index, agent->fps);
            }
        } else if (drop_factor < 0.9) {
            if (agent->fps > 1) {
                agent->fps--;
                spice_debug("stream %d: fps--%u", index, agent->fps);
            }
        }
        agent->frames = 1;
        agent->drops = 0;
    }
}

static Stream *display_channel_stream_try_new(DisplayChannel *display)
{
    Stream *stream;
    if (!display->free_streams) {
        return NULL;
    }
    stream = display->free_streams;
    display->free_streams = display->free_streams->next;
    return stream;
}

//void display_channel_create_stream(DisplayChannel *display, Drawable *drawable)
//{
//    DisplayChannelClient *dcc;
//    RingItem *dcc_ring_item, *next;
//    Stream *stream;
//    SpiceRect* src_rect;
//
//    spice_assert(!drawable->stream);
//
//    if (!(stream = display_channel_stream_try_new(display))) {
//        return;
//    }
//
//    spice_assert(drawable->red_drawable->type == QXL_DRAW_COPY);
//    src_rect = &drawable->red_drawable->u.copy.src_area;
//
//    ring_add(&display->streams, &stream->link);
//    stream->current = drawable;
//    stream->last_time = drawable->creation_time;
//    stream->width = src_rect->right - src_rect->left;
//    stream->height = src_rect->bottom - src_rect->top;
//    stream->dest_area = drawable->red_drawable->bbox;
//    stream->refs = 1;
//    SpiceBitmap *bitmap = &drawable->red_drawable->u.copy.src_bitmap->u.bitmap;
//    stream->top_down = !!(bitmap->flags & SPICE_BITMAP_FLAGS_TOP_DOWN);
//    drawable->stream = stream;
//    /* Provide an fps estimate the video encoder can use when initializing
//     * based on the frames that lead to the creation of the stream. Round to
//     * the nearest integer, for instance 24 for 23.976.
//     */
//    uint64_t duration = drawable->creation_time - drawable->first_frame_time;
//    if (duration > (uint64_t)drawable->frames_count * 1000 * 1000 * 1000 / MAX_FPS) {
//        stream->input_fps = ((uint64_t)drawable->frames_count * 1000 * 1000 * 1000 + duration / 2) / duration;
//    } else {
//        stream->input_fps = MAX_FPS;
//    }
//    stream->num_input_frames = 0;
//    stream->input_fps_start_time = drawable->creation_time;
//    display->streams_size_total += stream->width * stream->height;
//    display->stream_count++;
//    FOREACH_DCC(display, dcc_ring_item, next, dcc) {
//        if (!dcc_create_stream(dcc, stream)) {
//            drawable->stream = NULL;
//            stream->current = NULL;
//            stream_stop(display, stream);
//            return;
//        }
//    }
//    spice_info("stream %d %dx%d (%d, %d) (%d, %d) %u fps",
//                (int)(stream - display->streams_buf), stream->width,
//                stream->height, stream->dest_area.left, stream->dest_area.top,
//                stream->dest_area.right, stream->dest_area.bottom,
//                stream->input_fps);
//    return;
//}

void create_full_screen_stream(DisplayChannel *display)
{
    Stream *stream;
    if (!(stream = display_channel_stream_try_new(display))) {
    		spice_error("allocate new stream object fail");
        return;
    }

    ring_add(&display->streams, &stream->link);
    
    stream->current = NULL;
    stream->last_time = red_get_monotonic_time();
    stream->width = stream->dest_area.right = display->surfaces[0].context.width;
    stream->height = stream->dest_area.bottom = display->surfaces[0].context.height;
    stream->dest_area.top = stream->dest_area.left = 0;
    stream->refs = 1;
    stream->top_down = 1;
    stream->input_fps = MAX_FPS;
    stream->num_input_frames = 0;
    stream->input_fps_start_time = red_get_monotonic_time();
    display->streams_size_total += stream->width * stream->height;
    display->stream_count++;
    
    DisplayChannelClient *dcc;
    RingItem *dcc_ring_item, *next;
    FOREACH_DCC(display, dcc_ring_item, next, dcc) {
        if (!dcc_create_stream(dcc, stream)) {
        		spice_error("create fullscreen stream fail");
            stream_stop(display, stream);
            return;
        }
    }
    display->fullstream_id = (int)(stream - display->streams_buf);
    spice_info("fullstreen stream %d created, %d x %d (%d, %d) (%d, %d) %u fps",
                display->fullstream_id, stream->width,
                stream->height, stream->dest_area.left, stream->dest_area.top,
                stream->dest_area.right, stream->dest_area.bottom,
                stream->input_fps);
    return;
}

bool resize_all_client_render(DisplayChannel *display, const uint32_t render_type, DrawContext* surface_context){
	DisplayChannelClient *dcc;
  RingItem *dcc_ring_item, *next;
  FOREACH_DCC(display, dcc_ring_item, next, dcc) {
  	if(!dcc_allocate_render(dcc, render_type, surface_context, &display->image_cache.base,&display->image_surfaces)){
  		spice_error("resize client render for client %p fail", dcc);
  		return false;
  	}     
  }
  return true;
}

// returns whether a stream was created
static int stream_add_frame(DisplayChannel *display,
                            Drawable *frame_drawable,
                            red_time_t first_frame_time,
                            int frames_count,
                            int gradual_frames_count,
                            int last_gradual_frame)
{
    update_copy_graduality(display, frame_drawable);
    frame_drawable->first_frame_time = first_frame_time;
    frame_drawable->frames_count = frames_count + 1;
    frame_drawable->gradual_frames_count  = gradual_frames_count;

    if (frame_drawable->copy_bitmap_graduality != BITMAP_GRADUAL_LOW) {
        if ((frame_drawable->frames_count - last_gradual_frame) >
            RED_STREAM_FRAMES_RESET_CONDITION) {
            frame_drawable->frames_count = 1;
            frame_drawable->gradual_frames_count = 1;
        } else {
            frame_drawable->gradual_frames_count++;
        }

        frame_drawable->last_gradual_frame = frame_drawable->frames_count;
    } else {
        frame_drawable->last_gradual_frame = last_gradual_frame;
    }

//    if (is_stream_start(frame_drawable)) {
//        display_channel_create_stream(display, frame_drawable);
//        return TRUE;
//    }
    return FALSE;
}

/* TODO: document the difference between the 2 functions below */
void stream_trace_update(DisplayChannel *display, Drawable *drawable)
{
    ItemTrace *trace;
    ItemTrace *trace_end;
    RingItem *item;

    if (drawable->stream || !drawable->streamable || drawable->frames_count) {
        return;
    }

    FOREACH_STREAMS(display, item) {
        Stream *stream = SPICE_CONTAINEROF(item, Stream, link);
        int is_next_frame = is_next_stream_frame(display,
                                                 drawable,
                                                 stream->width,
                                                 stream->height,
                                                 &stream->dest_area,
                                                 stream->last_time,
                                                 stream,
                                                 TRUE);
        if (is_next_frame != STREAM_FRAME_NONE) {
//        		spice_info("Drawable %p is next frame for stream %d", 
//        								drawable, (int)(stream - display->streams_buf));
            if (stream->current) {
                stream->current->streamable = FALSE; //prevent item trace
                before_reattach_stream(display, stream, drawable);
                detach_stream(display, stream, FALSE);
            }
            attach_stream(display, drawable, stream);
            if (is_next_frame == STREAM_FRAME_CONTAINER) {
                drawable->sized_stream = stream;
            }
            return;
        }
    }

    trace = display->items_trace;
    trace_end = trace + NUM_TRACE_ITEMS;
    for (; trace < trace_end; trace++) {
        if (is_next_stream_frame(display, drawable, trace->width, trace->height,
                                       &trace->dest_area, trace->time, NULL, FALSE) !=
                                       STREAM_FRAME_NONE) {
            if (stream_add_frame(display, drawable,
                                 trace->first_frame_time,
                                 trace->frames_count,
                                 trace->gradual_frames_count,
                                 trace->last_gradual_frame)) {
                return;
            }
        }
    }
}

void stream_maintenance(DisplayChannel *display,
                        Drawable *candidate, Drawable *prev)
{
    int is_next_frame;

    if (candidate->stream) {
        return;
    }

    if (prev->stream) {
        Stream *stream = prev->stream;

        is_next_frame = is_next_stream_frame(display, candidate,
                                             stream->width, stream->height,
                                             &stream->dest_area, stream->last_time,
                                             stream, TRUE);
        if (is_next_frame != STREAM_FRAME_NONE) {
            before_reattach_stream(display, stream, candidate);
            detach_stream(display, stream, FALSE);
            prev->streamable = FALSE; //prevent item trace
            attach_stream(display, candidate, stream);
            if (is_next_frame == STREAM_FRAME_CONTAINER) {
                candidate->sized_stream = stream;
            }
        }
    } else if (candidate->streamable) {
        SpiceRect* prev_src = &prev->red_drawable->u.copy.src_area;

        is_next_frame =
            is_next_stream_frame(display, candidate, prev_src->right - prev_src->left,
                                 prev_src->bottom - prev_src->top,
                                 &prev->red_drawable->bbox, prev->creation_time,
                                 prev->stream,
                                 FALSE);
        if (is_next_frame != STREAM_FRAME_NONE) {
            stream_add_frame(display, candidate,
                             prev->first_frame_time,
                             prev->frames_count,
                             prev->gradual_frames_count,
                             prev->last_gradual_frame);
        }
    }
}

static void dcc_update_streams_max_latency(DisplayChannelClient *dcc, StreamAgent *remove_agent)
{
    uint32_t new_max_latency = 0;
    int i;

    if (dcc->streams_max_latency != remove_agent->client_required_latency) {
        return;
    }

    dcc->streams_max_latency = 0;
    if (DCC_TO_DC(dcc)->stream_count == 1) {
        return;
    }
    for (i = 0; i < NUM_STREAMS; i++) {
        StreamAgent *other_agent = &dcc->stream_agents[i];
        if (other_agent == remove_agent || !other_agent->video_encoder) {
            continue;
        }
        if (other_agent->client_required_latency > new_max_latency) {
            new_max_latency = other_agent->client_required_latency;
        }
    }
    dcc->streams_max_latency = new_max_latency;
}

static uint64_t red_stream_get_initial_bit_rate(DisplayChannelClient *dcc,
                                                Stream *stream)
{
    char *env_bit_rate_str;
    uint64_t bit_rate = 0;

    env_bit_rate_str = getenv("SPICE_BIT_RATE");
    if (env_bit_rate_str != NULL) {
        double env_bit_rate;

        errno = 0;
        env_bit_rate = strtod(env_bit_rate_str, NULL);
        if (errno == 0) {
            bit_rate = env_bit_rate * 1024 * 1024;
        } else {
            spice_warning("error parsing SPICE_BIT_RATE: %s", strerror(errno));
        }
    }

    if (!bit_rate) {
        MainChannelClient *mcc;
        uint64_t net_test_bit_rate;

        mcc = red_client_get_main(RED_CHANNEL_CLIENT(dcc)->client);
        net_test_bit_rate = main_channel_client_is_network_info_initialized(mcc) ?
                                main_channel_client_get_bitrate_per_sec(mcc) :
                                0;
        bit_rate = MAX(dcc->streams_max_bit_rate, net_test_bit_rate);
        if (bit_rate == 0) {
            /*
             * In case we are after a spice session migration,
             * the low_bandwidth flag is retrieved from migration data.
             * If the network info is not initialized due to another reason,
             * the low_bandwidth flag is FALSE.
             */
            bit_rate = dcc->common.is_low_bandwidth ?
                RED_STREAM_DEFAULT_LOW_START_BIT_RATE :
                RED_STREAM_DEFAULT_HIGH_START_BIT_RATE;
        }
    }

    spice_debug("base-bit-rate %.2f (Mbps)", bit_rate / 1024.0 / 1024.0);
    /* dividing the available bandwidth among the active streams, and saving
     * (1-RED_STREAM_CHANNEL_CAPACITY) of it for other messages */
    return (RED_STREAM_CHANNEL_CAPACITY * bit_rate *
            stream->width * stream->height) / DCC_TO_DC(dcc)->streams_size_total;
}

static uint32_t red_stream_video_encoder_get_roundtrip(void *opaque)
{
    StreamAgent *agent = opaque;
    int roundtrip;

    roundtrip = red_channel_client_get_roundtrip_ms(RED_CHANNEL_CLIENT(agent->dcc));
    if (roundtrip < 0) {
        MainChannelClient *mcc = red_client_get_main(RED_CHANNEL_CLIENT(agent->dcc)->client);

        /*
         * the main channel client roundtrip might not have been
         * calculated (e.g., after migration). In such case,
         * main_channel_client_get_roundtrip_ms returns 0.
         */
        roundtrip = main_channel_client_get_roundtrip_ms(mcc);
    }

    return roundtrip;
}

static uint32_t red_stream_video_encoder_get_source_fps(void *opaque)
{
    StreamAgent *agent = opaque;

    return agent->stream->input_fps;
}

static void red_stream_update_client_playback_latency(void *opaque, uint32_t delay_ms)
{
    StreamAgent *agent = opaque;
    DisplayChannelClient *dcc = agent->dcc;

    dcc_update_streams_max_latency(dcc, agent);

    agent->client_required_latency = delay_ms;
    if (delay_ms > agent->dcc->streams_max_latency) {
        agent->dcc->streams_max_latency = delay_ms;
    }
    spice_debug("resetting client latency: %u", agent->dcc->streams_max_latency);
    main_dispatcher_set_mm_time_latency(RED_CHANNEL_CLIENT(agent->dcc)->client, agent->dcc->streams_max_latency);
}

/* A helper for dcc_create_stream(). */
static VideoEncoder* dcc_create_video_encoder(DisplayChannelClient *dcc,
                                              uint64_t starting_bit_rate,
                                              VideoEncoderRateControlCbs *cbs,
                                              void *cbs_opaque)
{
    DisplayChannel *display = DCC_TO_DC(dcc);
    RedChannelClient *rcc = RED_CHANNEL_CLIENT(dcc);
    int client_has_multi_codec = red_channel_client_test_remote_cap(rcc, SPICE_DISPLAY_CAP_MULTI_CODEC);
    int i;

    for (i = 0; i < display->num_video_codecs; i++) {
        RedVideoCodec* video_codec = display->video_codecs+i;

        if (!client_has_multi_codec &&
            video_codec->type != SPICE_VIDEO_CODEC_TYPE_MJPEG) {
            /* Old clients only support MJPEG */
            spice_warning("client don't have multi codec support for codec %d", video_codec->type);
            continue;
        }
        if (client_has_multi_codec &&
            !red_channel_client_test_remote_cap(rcc, video_codec->cap)) {
            /* The client is recent but does not support this codec */
            spice_warning("client don't support codec %d", video_codec->type);
            continue;
        }

        VideoEncoder* video_encoder = video_codec->create(video_codec->type, starting_bit_rate, cbs, cbs_opaque);
        if (video_encoder) {
        		spice_warning("new video encoder created for codec %d", video_codec->type);
            return video_encoder;
        }
    }

		spice_warning("force ffmpeg H264 encoder");
    return ffmpeg_encoder_new(SPICE_VIDEO_CODEC_TYPE_H264, starting_bit_rate, cbs, cbs_opaque);
        
    /* Try to use the builtin MJPEG video encoder as a fallback */
//    if (!client_has_multi_codec || red_channel_client_test_remote_cap(rcc, SPICE_DISPLAY_CAP_CODEC_MJPEG)) {
//    		spice_warning("initial old mpjeg encoder");
//        return mjpeg_encoder_new(SPICE_VIDEO_CODEC_TYPE_MJPEG, starting_bit_rate, cbs, cbs_opaque);
//    }

    return NULL;
}

int dcc_create_stream(DisplayChannelClient *dcc, Stream *stream)
{
    StreamAgent *agent = &dcc->stream_agents[get_stream_id(DCC_TO_DC(dcc), stream)];

    spice_return_val_if_fail(region_is_empty(&agent->vis_region), FALSE);

    stream->refs++;
    if (stream->current) {
        agent->frames = 1;
        region_clone(&agent->vis_region, &stream->current->tree_item.base.rgn);
        region_clone(&agent->clip, &agent->vis_region);
    } else {
        agent->frames = 0;
    }
    agent->drops = 0;
    agent->fps = MAX_FPS;
    agent->dcc = dcc;

    if (dcc->use_video_encoder_rate_control) {
        VideoEncoderRateControlCbs video_cbs;
        uint64_t initial_bit_rate;

        video_cbs.get_roundtrip_ms = red_stream_video_encoder_get_roundtrip;
        video_cbs.get_source_fps = red_stream_video_encoder_get_source_fps;
        video_cbs.update_client_playback_delay = red_stream_update_client_playback_latency;

        initial_bit_rate = red_stream_get_initial_bit_rate(dcc, stream);
        agent->video_encoder = dcc_create_video_encoder(dcc, initial_bit_rate, &video_cbs, agent);
    } else {
        agent->video_encoder = dcc_create_video_encoder(dcc, 0, NULL, NULL);
    }
    if (agent->video_encoder == NULL) {
        stream->refs--;
        return FALSE;
    }
    red_channel_client_pipe_add(RED_CHANNEL_CLIENT(dcc), &agent->create_item);

    if (red_channel_client_test_remote_cap(RED_CHANNEL_CLIENT(dcc), SPICE_DISPLAY_CAP_STREAM_REPORT)) {
        StreamActivateReportItem *report_pipe_item = spice_malloc0(sizeof(*report_pipe_item));

        agent->report_id = rand();
        red_channel_pipe_item_init(RED_CHANNEL_CLIENT(dcc)->channel, &report_pipe_item->pipe_item,
                                   PIPE_ITEM_TYPE_STREAM_ACTIVATE_REPORT);
        report_pipe_item->stream_id = get_stream_id(DCC_TO_DC(dcc), stream);
        red_channel_client_pipe_add(RED_CHANNEL_CLIENT(dcc), &report_pipe_item->pipe_item);
    }
#ifdef STREAM_STATS
    memset(&agent->stats, 0, sizeof(StreamStats));
    if (stream->current) {
        agent->stats.start = stream->current->red_drawable->mm_time;
    }
#endif
    return TRUE;
}

void stream_agent_stop(DisplayChannelClient *dcc, StreamAgent *agent)
{
    dcc_update_streams_max_latency(dcc, agent);
    if (agent->video_encoder) {
    		spice_info("stream stopped by agent");
        agent->video_encoder->destroy(agent->video_encoder);
        agent->video_encoder = NULL;
    }
}
