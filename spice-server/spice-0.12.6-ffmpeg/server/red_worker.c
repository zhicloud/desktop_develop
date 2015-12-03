/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
   Copyright (C) 2009 Red Hat, Inc.
   Copyright (C) 2015 Francois Gouget

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

#define SPICE_LOG_DOMAIN "SpiceWorker"

/* Common variable abbreviations:
 *
 * rcc - RedChannelClient
 * ccc - CursorChannelClient (not to be confused with common_cc)
 * common_cc - CommonChannelClient
 * dcc - DisplayChannelClient
 * cursor_red_channel - downcast of CursorChannel to RedChannel
 * display_red_channel - downcast of DisplayChannel to RedChannel
 */

#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <openssl/ssl.h>
#include <inttypes.h>
#include <sys/time.h> 
#include <glib.h>

#include <spice/protocol.h>
#include <spice/qxl_dev.h>
#include "common/lz.h"
#include "common/marshaller.h"
#include "common/quic.h"
#include "common/rect.h"
#include "common/region.h"
#include "common/ring.h"
#include "common/generated_server_marshallers.h"

#include "display-channel.h"
#include "stream.h"

#include "spice.h"
#include "red_worker.h"
#include "spice_timer_queue.h"
#include "cursor-channel.h"
#include "tree.h"

//#define COMPRESS_STAT
//#define DUMP_BITMAP
//#define COMPRESS_DEBUG

#define CMD_RING_POLL_TIMEOUT 10 //milli
#define CMD_RING_POLL_RETRIES 200

#define DISPLAY_CLIENT_SHORT_TIMEOUT 15000000000ULL //nano
#define DISPLAY_CLIENT_MIGRATE_DATA_TIMEOUT 10000000000ULL //nano, 10 sec
#define DISPLAY_CLIENT_RETRY_INTERVAL 10000 //micro

#define DISPLAY_FREE_LIST_DEFAULT_SIZE 128

#define ZLIB_DEFAULT_COMPRESSION_LEVEL 3
#define MIN_GLZ_SIZE_FOR_ZLIB 100

#define VALIDATE_SURFACE_RET(worker, surface_id) \
    if (!validate_surface(worker, surface_id)) { \
        rendering_incorrect(__func__); \
        return; \
    }

#define VALIDATE_SURFACE_RETVAL(worker, surface_id, ret) \
    if (!validate_surface(worker, surface_id)) { \
        rendering_incorrect(__func__); \
        return ret; \
    }

#define VALIDATE_SURFACE_BREAK(worker, surface_id) \
    if (!validate_surface(worker, surface_id)) { \
        rendering_incorrect(__func__); \
        break; \
    }

static void rendering_incorrect(const char *msg)
{
    spice_warning("rendering incorrect from now on: %s", msg);
}

#define MAX_EVENT_SOURCES 20
#define INF_EVENT_WAIT ~0

struct SpiceWatch {
    struct RedWorker *worker;
    SpiceWatchFunc watch_func;
    void *watch_func_opaque;
};

#define MAX_LZ_ENCODERS MAX_CACHE_CLIENTS

#define MAX_PIPE_SIZE 50

#define WIDE_CLIENT_ACK_WINDOW 40
#define NARROW_CLIENT_ACK_WINDOW 20

typedef struct ImageItem {
    PipeItem link;
    int refs;
    SpicePoint pos;
    int width;
    int height;
    int stride;
    int top_down;
    int surface_id;
    int image_format;
    uint32_t image_flags;
    int can_lossy;
    uint8_t data[0];
} ImageItem;

typedef struct {
    QuicUsrContext usr;
    EncoderData data;
} QuicData;

typedef struct {
    LzUsrContext usr;
    EncoderData data;
} LzData;

typedef struct {
    JpegEncoderUsrContext usr;
    EncoderData data;
} JpegData;

#ifdef USE_LZ4
typedef struct {
    Lz4EncoderUsrContext usr;
    EncoderData data;
} Lz4Data;
#endif

typedef struct {
    ZlibEncoderUsrContext usr;
    EncoderData data;
} ZlibData;

/**********************************/
/* LZ dictionary related entities */
/**********************************/
#define MAX_GLZ_DRAWABLE_INSTANCES 2

typedef struct RedGlzDrawable RedGlzDrawable;

/* for each qxl drawable, there may be several instances of lz drawables */
/* TODO - reuse this stuff for the top level. I just added a second level of multiplicity
 * at the Drawable by keeping a ring, so:
 * Drawable -> (ring of) RedGlzDrawable -> (up to 2) GlzDrawableInstanceItem
 * and it should probably (but need to be sure...) be
 * Drawable -> ring of GlzDrawableInstanceItem.
 */
typedef struct GlzDrawableInstanceItem {
    RingItem glz_link;
    RingItem free_link;
    GlzEncDictImageContext *glz_instance;
    RedGlzDrawable         *red_glz_drawable;
} GlzDrawableInstanceItem;

struct RedGlzDrawable {
    RingItem link;    // ordered by the time it was encoded
    RingItem drawable_link;
    RedDrawable *red_drawable;
    Drawable    *drawable;
    uint32_t     group_id;
    GlzDrawableInstanceItem instances_pool[MAX_GLZ_DRAWABLE_INSTANCES];
    Ring instances;
    uint8_t instances_count;
    DisplayChannelClient *dcc;
};

pthread_mutex_t glz_dictionary_list_lock = PTHREAD_MUTEX_INITIALIZER;
Ring glz_dictionary_list = {&glz_dictionary_list, &glz_dictionary_list};

typedef struct UpgradeItem {
    PipeItem base;
    int refs;
    Drawable *drawable;
    SpiceClipRects *rects;
} UpgradeItem;

struct RedWorker {
    pthread_t thread;
    clockid_t clockid;
    QXLInstance *qxl;
    RedDispatcher *red_dispatcher;
    int running;
    struct pollfd poll_fds[MAX_EVENT_SOURCES];
    struct SpiceWatch watches[MAX_EVENT_SOURCES];
    unsigned int event_timeout;

    DisplayChannel *display_channel;
    uint32_t display_poll_tries;

    CursorChannel *cursor_channel;
    uint32_t cursor_poll_tries;

    uint32_t red_drawable_count;
    uint32_t glz_drawable_count;
    uint32_t bits_unique;

    RedMemSlotInfo mem_slots;

    SpiceImageCompression image_compression;
    spice_wan_compression_t jpeg_state;
    spice_wan_compression_t zlib_glz_state;

    QuicData quic_data;
    QuicContext *quic;

    LzData lz_data;
    LzContext  *lz;

    JpegData jpeg_data;
    JpegEncoderContext *jpeg;

#ifdef USE_LZ4
    Lz4Data lz4_data;
    Lz4EncoderContext *lz4;
#endif

    ZlibData zlib_data;
    ZlibEncoder *zlib;

    uint32_t process_commands_generation;
#ifdef RED_STATISTICS
    StatNodeRef stat;
    uint64_t *wakeup_counter;
    uint64_t *command_counter;
#endif

    int driver_cap_monitors_config;

    FILE *record_fd;
};

typedef enum {
    BITMAP_DATA_TYPE_INVALID,
    BITMAP_DATA_TYPE_CACHE,
    BITMAP_DATA_TYPE_SURFACE,
    BITMAP_DATA_TYPE_BITMAP,
    BITMAP_DATA_TYPE_BITMAP_TO_CACHE,
} BitmapDataType;

typedef struct BitmapData {
    BitmapDataType type;
    uint64_t id; // surface id or cache item id
    SpiceRect lossy_rect;
} BitmapData;

static inline int validate_surface(DisplayChannel *display, uint32_t surface_id);

static void red_draw_qxl_drawable(DisplayChannel *display, Drawable *drawable);
static void red_current_flush(DisplayChannel *display, int surface_id);
static void red_draw_drawable(DisplayChannel *display, Drawable *item);
static void red_update_area(DisplayChannel *display, const SpiceRect *area, int surface_id);
static void red_update_area_till(DisplayChannel *display, const SpiceRect *area, int surface_id,
                                 Drawable *last);
static inline void display_begin_send_message(RedChannelClient *rcc);
static void red_release_glz(DisplayChannelClient *dcc);
static void red_freeze_glz(DisplayChannelClient *dcc);
static void display_channel_push_release(DisplayChannelClient *dcc, uint8_t type, uint64_t id,
                                         uint64_t* sync_data);
static int red_display_free_some_independent_glz_drawables(DisplayChannelClient *dcc);
static void dcc_free_glz_drawable(DisplayChannelClient *dcc, RedGlzDrawable *drawable);
static ImageItem *red_add_surface_area_image(DisplayChannelClient *dcc, int surface_id,
                                             SpiceRect *area, PipeItem *pos, int can_lossy);
static void display_channel_client_release_item_before_push(DisplayChannelClient *dcc,
                                                            PipeItem *item);
static void display_channel_client_release_item_after_push(DisplayChannelClient *dcc,
                                                           PipeItem *item);
static void red_create_surface(DisplayChannel *display, uint32_t surface_id, uint32_t width,
                               uint32_t height, int32_t stride, uint32_t format,
                               void *line_0, int data_is_valid, int send_client);


#define LINK_TO_GLZ(ptr) SPICE_CONTAINEROF((ptr), RedGlzDrawable, \
                                           drawable_link)
#define DRAWABLE_FOREACH_GLZ_SAFE(drawable, link, next, glz) \
    SAFE_FOREACH(link, next, drawable, &(drawable)->glz_ring, glz, LINK_TO_GLZ(link))


static void display_stream_clip_unref(DisplayChannel *display, StreamClipItem *item)
{
    if (--item->refs != 0)
        return;

    stream_agent_unref(display, item->stream_agent);
    free(item->rects);
    free(item);
}

void attach_stream(DisplayChannel *display, Drawable *drawable, Stream *stream)
{
    DisplayChannelClient *dcc;
    RingItem *item, *next;

    spice_assert(!drawable->stream && !stream->current);
    spice_assert(drawable && stream);
    stream->current = drawable;
    drawable->stream = stream;
    stream->last_time = drawable->creation_time;
    //spice_info("Drawable %p attach to stream %d", drawable, (int)(stream - display->streams_buf));

    uint64_t duration = drawable->creation_time - stream->input_fps_start_time;
    if (duration >= RED_STREAM_INPUT_FPS_TIMEOUT) {
        /* Round to the nearest integer, for instance 24 for 23.976 */
        stream->input_fps = ((uint64_t)stream->num_input_frames * 1000 * 1000 * 1000 + duration / 2) / duration;
        spice_debug("input-fps=%u", stream->input_fps);
        stream->num_input_frames = 0;
        stream->input_fps_start_time = drawable->creation_time;
    } else {
        stream->num_input_frames++;
    }

    FOREACH_DCC(display, item, next, dcc) {
        StreamAgent *agent;
        QRegion clip_in_draw_dest;

        agent = &dcc->stream_agents[get_stream_id(display, stream)];
        region_or(&agent->vis_region, &drawable->tree_item.base.rgn);

        region_init(&clip_in_draw_dest);
        region_add(&clip_in_draw_dest, &drawable->red_drawable->bbox);
        region_and(&clip_in_draw_dest, &agent->clip);

        if (!region_is_equal(&clip_in_draw_dest, &drawable->tree_item.base.rgn)) {
            region_remove(&agent->clip, &drawable->red_drawable->bbox);
            region_or(&agent->clip, &drawable->tree_item.base.rgn);
            dcc_add_stream_agent_clip(dcc, agent);
        }
#ifdef STREAM_STATS
        agent->stats.num_input_frames++;
#endif
    }
}

/* fixme: move to display channel */
DrawablePipeItem *drawable_pipe_item_new(DisplayChannelClient *dcc,
                                         Drawable *drawable)
{
    DrawablePipeItem *dpi;

    dpi = spice_malloc0(sizeof(*dpi));
    dpi->drawable = drawable;
    dpi->dcc = dcc;
    ring_item_init(&dpi->base);
    ring_add(&drawable->pipes, &dpi->base);
    red_channel_pipe_item_init(RED_CHANNEL_CLIENT(dcc)->channel,
                               &dpi->dpi_pipe_item, PIPE_ITEM_TYPE_DRAW);
    dpi->refs++;
    drawable->refs++;
    return dpi;
}

DrawablePipeItem *drawable_pipe_item_ref(DrawablePipeItem *dpi)
{
    dpi->refs++;
    return dpi;
}

void drawable_pipe_item_unref(DrawablePipeItem *dpi)
{
    DisplayChannel *display = DCC_TO_DC(dpi->dcc);

    if (--dpi->refs != 0) {
        return;
    }

    spice_warn_if_fail(!ring_item_is_linked(&dpi->dpi_pipe_item.link));
    spice_warn_if_fail(!ring_item_is_linked(&dpi->base));
    display_channel_drawable_unref(display, dpi->drawable);
    free(dpi);
}

QXLInstance* red_worker_get_qxl(RedWorker *worker)
{
    spice_return_val_if_fail(worker != NULL, NULL);

    return worker->qxl;
}

static int validate_drawable_bbox(RedWorker *worker, RedDrawable *drawable)
{
        DrawContext *context;
        uint32_t surface_id = drawable->surface_id;

        /* surface_id must be validated before calling into
         * validate_drawable_bbox
         */
        VALIDATE_SURFACE_RETVAL(worker->display_channel, surface_id, FALSE);
        context = &worker->display_channel->surfaces[surface_id].context;

        if (drawable->bbox.top < 0)
                return FALSE;
        if (drawable->bbox.left < 0)
                return FALSE;
        if (drawable->bbox.bottom < 0)
                return FALSE;
        if (drawable->bbox.right < 0)
                return FALSE;
        if (drawable->bbox.bottom > context->height)
                return FALSE;
        if (drawable->bbox.right > context->width)
                return FALSE;

        return TRUE;
}

static inline int validate_surface(DisplayChannel *display, uint32_t surface_id)
{
    if SPICE_UNLIKELY(surface_id >= display->n_surfaces) {
        spice_warning("invalid surface_id %u", surface_id);
        return 0;
    }
    if (!display->surfaces[surface_id].context.canvas) {
        spice_warning("canvas address is %p for %d (and is NULL)\n",
                   &(display->surfaces[surface_id].context.canvas), surface_id);
        spice_warning("failed on %d", surface_id);
        return 0;
    }
    return 1;
}

static inline void red_create_surface_item(DisplayChannelClient *dcc, int surface_id);
static void red_push_surface_image(DisplayChannelClient *dcc, int surface_id);

static inline void red_handle_drawable_surfaces_client_synced(
                        DisplayChannelClient *dcc, Drawable *drawable)
{
    DisplayChannel *display = DCC_TO_DC(dcc);
    int x;

    for (x = 0; x < 3; ++x) {
        int surface_id;

        surface_id = drawable->surface_deps[x];
        if (surface_id != -1) {
            if (dcc->surface_client_created[surface_id] == TRUE) {
                continue;
            }
            red_create_surface_item(dcc, surface_id);
            red_current_flush(display, surface_id);
            red_push_surface_image(dcc, surface_id);
        }
    }

    if (dcc->surface_client_created[drawable->surface_id] == TRUE) {
        return;
    }

    red_create_surface_item(dcc, drawable->surface_id);
    red_current_flush(display, drawable->surface_id);
    red_push_surface_image(dcc, drawable->surface_id);
}

static int display_is_connected(RedWorker *worker)
{
    return (worker->display_channel && red_channel_is_connected(
        &worker->display_channel->common.base));
}

static int cursor_is_connected(RedWorker *worker)
{
    return worker->cursor_channel &&
        red_channel_is_connected(RED_CHANNEL(worker->cursor_channel));
}

void dcc_add_drawable(DisplayChannelClient *dcc, Drawable *drawable)
{
    DrawablePipeItem *dpi;

    red_handle_drawable_surfaces_client_synced(dcc, drawable);
    dpi = drawable_pipe_item_new(dcc, drawable);
    red_channel_client_pipe_add(RED_CHANNEL_CLIENT(dcc), &dpi->dpi_pipe_item);
}

void red_pipes_add_drawable(DisplayChannel *display, Drawable *drawable)
{
    DisplayChannelClient *dcc;
    RingItem *dcc_ring_item, *next;

    spice_warn_if(!ring_is_empty(&drawable->pipes));
    FOREACH_DCC(display, dcc_ring_item, next, dcc) {
        dcc_add_drawable(dcc, drawable);
    }
}

static void dcc_add_drawable_to_tail(DisplayChannelClient *dcc, Drawable *drawable)
{
    DrawablePipeItem *dpi;

    if (!dcc) {
        return;
    }
    red_handle_drawable_surfaces_client_synced(dcc, drawable);
    dpi = drawable_pipe_item_new(dcc, drawable);
    red_channel_client_pipe_add_tail(RED_CHANNEL_CLIENT(dcc), &dpi->dpi_pipe_item);
}

void red_pipes_add_drawable_after(DisplayChannel *display,
                                  Drawable *drawable, Drawable *pos_after)
{
    DrawablePipeItem *dpi, *dpi_pos_after;
    RingItem *dpi_link, *dpi_next;
    DisplayChannelClient *dcc;
    int num_other_linked = 0;

    DRAWABLE_FOREACH_DPI_SAFE(pos_after, dpi_link, dpi_next, dpi_pos_after) {
        num_other_linked++;
        dcc = dpi_pos_after->dcc;
        red_handle_drawable_surfaces_client_synced(dcc, drawable);
        dpi = drawable_pipe_item_new(dcc, drawable);
        red_channel_client_pipe_add_after(RED_CHANNEL_CLIENT(dcc), &dpi->dpi_pipe_item,
                                          &dpi_pos_after->dpi_pipe_item);
    }
    if (num_other_linked == 0) {
        red_pipes_add_drawable(display, drawable);
        return;
    }
    if (num_other_linked != display->common.base.clients_num) {
        RingItem *item, *next;
        spice_debug("TODO: not O(n^2)");
        FOREACH_DCC(display, item, next, dcc) {
            int sent = 0;
            DRAWABLE_FOREACH_DPI_SAFE(pos_after, dpi_link, dpi_next, dpi_pos_after) {
                if (dpi_pos_after->dcc == dcc) {
                    sent = 1;
                    break;
                }
            }
            if (!sent) {
                dcc_add_drawable(dcc, drawable);
            }
        }
    }
}

static inline PipeItem *red_pipe_get_tail(DisplayChannelClient *dcc)
{
    if (!dcc) {
        return NULL;
    }

    return (PipeItem*)ring_get_tail(&RED_CHANNEL_CLIENT(dcc)->pipe);
}

void red_pipes_remove_drawable(Drawable *drawable)
{
    DrawablePipeItem *dpi;
    RingItem *item, *next;

    RING_FOREACH_SAFE(item, next, &drawable->pipes) {
        dpi = SPICE_CONTAINEROF(item, DrawablePipeItem, base);
        if (pipe_item_is_linked(&dpi->dpi_pipe_item)) {
            red_channel_client_pipe_remove_and_release(RED_CHANNEL_CLIENT(dpi->dcc),
                                                       &dpi->dpi_pipe_item);
        }
    }
}

static inline void red_pipe_add_image_item(DisplayChannelClient *dcc, ImageItem *item)
{
    if (!dcc) {
        return;
    }
    item->refs++;
    red_channel_client_pipe_add(RED_CHANNEL_CLIENT(dcc), &item->link);
}

static inline void red_pipe_add_image_item_after(DisplayChannelClient *dcc, ImageItem *item,
                                                 PipeItem *pos)
{
    if (!dcc) {
        return;
    }
    item->refs++;
    red_channel_client_pipe_add_after(RED_CHANNEL_CLIENT(dcc), &item->link, pos);
}

static void release_image_item(ImageItem *item)
{
    if (!--item->refs) {
        free(item);
    }
}

static void upgrade_item_unref(DisplayChannel *display, UpgradeItem *item)
{
    if (--item->refs != 0)
        return;

    display_channel_drawable_unref(display, item->drawable);
    free(item->rects);
    free(item);
}

static uint8_t *common_alloc_recv_buf(RedChannelClient *rcc, uint16_t type, uint32_t size)
{
    CommonChannel *common = SPICE_CONTAINEROF(rcc->channel, CommonChannel, base);

    /* SPICE_MSGC_MIGRATE_DATA is the only client message whose size is dynamic */
    if (type == SPICE_MSGC_MIGRATE_DATA) {
        return spice_malloc(size);
    }

    if (size > CHANNEL_RECEIVE_BUF_SIZE) {
        spice_critical("unexpected message size %u (max is %d)", size, CHANNEL_RECEIVE_BUF_SIZE);
        return NULL;
    }
    return common->recv_buf;
}

static void common_release_recv_buf(RedChannelClient *rcc, uint16_t type, uint32_t size,
                                    uint8_t* msg)
{
    if (type == SPICE_MSGC_MIGRATE_DATA) {
        free(msg);
    }
}

#define CLIENT_PALETTE_CACHE
#include "cache_item.tmpl.c"
#undef CLIENT_PALETTE_CACHE

static void red_reset_palette_cache(DisplayChannelClient *dcc)
{
    red_palette_cache_reset(dcc, CLIENT_PALETTE_CACHE_SIZE);
}

static Drawable* drawable_try_new(DisplayChannel *display)
{
    Drawable *drawable;

    if (!display->free_drawables)
        return NULL;

    drawable = &display->free_drawables->u.drawable;
    display->free_drawables = display->free_drawables->u.next;
    display->drawable_count++;

    return drawable;
}

static void drawable_free(DisplayChannel *display, Drawable *drawable)
{
    ((_Drawable *)drawable)->u.next = display->free_drawables;
    display->free_drawables = (_Drawable *)drawable;
}

static void drawables_init(DisplayChannel *display)
{
    int i;

    display->free_drawables = NULL;
    for (i = 0; i < NUM_DRAWABLES; i++) {
        drawable_free(display, &display->drawables[i].u.drawable);
    }
}


static inline void set_surface_release_info(QXLReleaseInfoExt *release_info_ext,
                                            QXLReleaseInfo *release_info, uint32_t group_id)
{
    release_info_ext->info = release_info;
    release_info_ext->group_id = group_id;
}

static RedDrawable *red_drawable_ref(RedDrawable *drawable)
{
    drawable->refs++;
    return drawable;
}


static void red_drawable_unref(RedWorker *worker, RedDrawable *red_drawable,
                               uint32_t group_id)
{
    QXLReleaseInfoExt release_info_ext;

    if (--red_drawable->refs) {
        return;
    }
    worker->red_drawable_count--;
    release_info_ext.group_id = group_id;
    release_info_ext.info = red_drawable->release_info;
    worker->qxl->st->qif->release_resource(worker->qxl, release_info_ext);
    red_put_drawable(red_drawable);
    free(red_drawable);
}

static void remove_depended_item(DependItem *item)
{
    spice_assert(item->drawable);
    spice_assert(ring_item_is_linked(&item->ring_item));
    item->drawable = NULL;
    ring_remove(&item->ring_item);
}

static void drawable_unref_surface_deps(DisplayChannel *display, Drawable *drawable)
{
    int x;
    int surface_id;

    for (x = 0; x < 3; ++x) {
        surface_id = drawable->surface_deps[x];
        if (surface_id == -1) {
            continue;
        }
        display_channel_surface_unref(display, surface_id);
    }
}

static void drawable_remove_dependencies(DisplayChannel *display, Drawable *drawable)
{
    int x;
    int surface_id;

    for (x = 0; x < 3; ++x) {
        surface_id = drawable->surface_deps[x];
        if (surface_id != -1 && drawable->depend_items[x].drawable) {
            remove_depended_item(&drawable->depend_items[x]);
        }
    }
}

void display_channel_drawable_unref(DisplayChannel *display, Drawable *drawable)
{
    RingItem *item, *next;

    if (--drawable->refs != 0)
        return;

    spice_warn_if_fail(!drawable->tree_item.shadow);
    spice_warn_if_fail(ring_is_empty(&drawable->pipes));

    if (drawable->stream) {
        detach_stream(display, drawable->stream, TRUE);
    }
    region_destroy(&drawable->tree_item.base.rgn);

    drawable_remove_dependencies(display, drawable);
    drawable_unref_surface_deps(display, drawable);
    display_channel_surface_unref(display, drawable->surface_id);

    RING_FOREACH_SAFE(item, next, &drawable->glz_ring) {
        SPICE_CONTAINEROF(item, RedGlzDrawable, drawable_link)->drawable = NULL;
        ring_remove(item);
    }
    red_drawable_unref(COMMON_CHANNEL(display)->worker, drawable->red_drawable, drawable->group_id);
    drawable_free(display, drawable);
    display->drawable_count--;
}

static void display_stream_trace_add_drawable(DisplayChannel *display, Drawable *item)
{
    ItemTrace *trace;

    if (item->stream || !item->streamable) {
        return;
    }

    trace = &display->items_trace[display->next_item_trace++ & ITEMS_TRACE_MASK];
    trace->time = item->creation_time;
    trace->first_frame_time = item->first_frame_time;
    trace->frames_count = item->frames_count;
    trace->gradual_frames_count = item->gradual_frames_count;
    trace->last_gradual_frame = item->last_gradual_frame;
    SpiceRect* src_area = &item->red_drawable->u.copy.src_area;
    trace->width = src_area->right - src_area->left;
    trace->height = src_area->bottom - src_area->top;
    trace->dest_area = item->red_drawable->bbox;
}

static void surface_flush(DisplayChannel *display, int surface_id, SpiceRect *rect)
{
    red_update_area(display, rect, surface_id);
}

static void red_flush_source_surfaces(DisplayChannel *display, Drawable *drawable)
{
    int x;
    int surface_id;

    for (x = 0; x < 3; ++x) {
        surface_id = drawable->surface_deps[x];
        if (surface_id != -1 && drawable->depend_items[x].drawable) {
            remove_depended_item(&drawable->depend_items[x]);
            surface_flush(display, surface_id, &drawable->red_drawable->surfaces_rects[x]);
        }
    }
}

void current_remove_drawable(DisplayChannel *display, Drawable *item)
{
    /* todo: move all to unref? */
    display_stream_trace_add_drawable(display, item);
    draw_item_remove_shadow(&item->tree_item);
    ring_remove(&item->tree_item.base.siblings_link);
    ring_remove(&item->list_link);
    ring_remove(&item->surface_list_link);
    display_channel_drawable_unref(display, item);
    display->current_size--;
}

void current_remove(DisplayChannel *display, TreeItem *item)
{
    TreeItem *now = item;

    /* depth-first tree traversal, TODO: do a to tree_foreach()? */
    for (;;) {
        Container *container = now->container;
        RingItem *ring_item;

        if (now->type == TREE_ITEM_TYPE_DRAWABLE) {
            Drawable *drawable = SPICE_CONTAINEROF(now, Drawable, tree_item);
            ring_item = now->siblings_link.prev;
            red_pipes_remove_drawable(drawable);
            current_remove_drawable(display, drawable);
        } else {
            Container *container = (Container *)now;

            spice_assert(now->type == TREE_ITEM_TYPE_CONTAINER);

            if ((ring_item = ring_get_head(&container->items))) {
                now = SPICE_CONTAINEROF(ring_item, TreeItem, siblings_link);
                continue;
            }
            ring_item = now->siblings_link.prev;
            container_free(container);
        }
        if (now == item) {
            return;
        }

        if ((ring_item = ring_next(&container->items, ring_item))) {
            now = SPICE_CONTAINEROF(ring_item, TreeItem, siblings_link);
        } else {
            now = (TreeItem *)container;
        }
    }
}

static void current_remove_all(DisplayChannel *display, int surface_id)
{
    Ring *ring = &display->surfaces[surface_id].current;
    RingItem *ring_item;

    while ((ring_item = ring_get_head(ring))) {
        TreeItem *now = SPICE_CONTAINEROF(ring_item, TreeItem, siblings_link);
        current_remove(display, now);
    }
}

/*
 * Return: TRUE if wait_if_used == FALSE, or otherwise, if all of the pipe items that
 * are related to the surface have been cleared (or sent) from the pipe.
 */
static int red_clear_surface_drawables_from_pipe(DisplayChannelClient *dcc, int surface_id,
                                                 int wait_if_used)
{
    Ring *ring;
    PipeItem *item;
    int x;
    RedChannelClient *rcc;

    if (!dcc) {
        return TRUE;
    }

    /* removing the newest drawables that their destination is surface_id and
       no other drawable depends on them */

    rcc = RED_CHANNEL_CLIENT(dcc);
    ring = &rcc->pipe;
    item = (PipeItem *) ring;
    while ((item = (PipeItem *)ring_next(ring, (RingItem *)item))) {
        Drawable *drawable;
        DrawablePipeItem *dpi = NULL;
        int depend_found = FALSE;

        if (item->type == PIPE_ITEM_TYPE_DRAW) {
            dpi = SPICE_CONTAINEROF(item, DrawablePipeItem, dpi_pipe_item);
            drawable = dpi->drawable;
        } else if (item->type == PIPE_ITEM_TYPE_UPGRADE) {
            drawable = ((UpgradeItem *)item)->drawable;
        } else {
            continue;
        }

        if (drawable->surface_id == surface_id) {
            PipeItem *tmp_item = item;
            item = (PipeItem *)ring_prev(ring, (RingItem *)item);
            red_channel_client_pipe_remove_and_release(rcc, tmp_item);
            if (!item) {
                item = (PipeItem *)ring;
            }
            continue;
        }

        for (x = 0; x < 3; ++x) {
            if (drawable->surface_deps[x] == surface_id) {
                depend_found = TRUE;
                break;
            }
        }

        if (depend_found) {
            spice_debug("surface %d dependent item found %p, %p", surface_id, drawable, item);
            if (wait_if_used) {
                break;
            } else {
                return TRUE;
            }
        }
    }

    if (!wait_if_used) {
        return TRUE;
    }

    if (item) {
        return red_channel_client_wait_pipe_item_sent(RED_CHANNEL_CLIENT(dcc), item,
                                                      DISPLAY_CLIENT_TIMEOUT);
    } else {
        /*
         * in case that the pipe didn't contain any item that is dependent on the surface, but
         * there is one during sending. Use a shorter timeout, since it is just one item
         */
        return red_channel_client_wait_outgoing_item(RED_CHANNEL_CLIENT(dcc),
                                                     DISPLAY_CLIENT_SHORT_TIMEOUT);
    }
    return TRUE;
}

static void red_clear_surface_drawables_from_pipes(DisplayChannel *display,
                                                   int surface_id,
                                                   int wait_if_used)
{
    RingItem *item, *next;
    DisplayChannelClient *dcc;

    FOREACH_DCC(display, item, next, dcc) {
        if (!red_clear_surface_drawables_from_pipe(dcc, surface_id, wait_if_used)) {
            red_channel_client_disconnect(RED_CHANNEL_CLIENT(dcc));
        }
    }
}

void detach_stream(DisplayChannel *display, Stream *stream,
                   int detach_sized)
{
    spice_assert(stream->current && stream->current->stream);
    spice_assert(stream->current->stream == stream);
    stream->current->stream = NULL;
    if (detach_sized) {
        stream->current->sized_stream = NULL;
    }
    stream->current = NULL;
}

static int red_display_drawable_is_in_pipe(DisplayChannelClient *dcc, Drawable *drawable)
{
    DrawablePipeItem *dpi;
    RingItem *dpi_link, *dpi_next;

    DRAWABLE_FOREACH_DPI_SAFE(drawable, dpi_link, dpi_next, dpi) {
        if (dpi->dcc == dcc) {
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * after dcc_detach_stream_gracefully is called for all the display channel clients,
 * detach_stream should be called. See comment (1).
 */
static void dcc_detach_stream_gracefully(DisplayChannelClient *dcc,
                                         Stream *stream,
                                         Drawable *update_area_limit)
{
    DisplayChannel *display = DCC_TO_DC(dcc);
    int stream_id = get_stream_id(display, stream);
    StreamAgent *agent = &dcc->stream_agents[stream_id];

    /* stopping the client from playing older frames at once*/
    region_clear(&agent->clip);
    dcc_add_stream_agent_clip(dcc, agent);

    if (region_is_empty(&agent->vis_region)) {
        spice_debug("stream %d: vis region empty", stream_id);
        return;
    }

    if (stream->current &&
        region_contains(&stream->current->tree_item.base.rgn, &agent->vis_region)) {
        RedChannel *channel;
        RedChannelClient *rcc;
        UpgradeItem *upgrade_item;
        int n_rects;

        /* (1) The caller should detach the drawable from the stream. This will
         * lead to sending the drawable losslessly, as an ordinary drawable. */
        if (red_display_drawable_is_in_pipe(dcc, stream->current)) {
            spice_debug("stream %d: upgrade by linked drawable. sized %d, box ==>",
                        stream_id, stream->current->sized_stream != NULL);
            rect_debug(&stream->current->red_drawable->bbox);
            goto clear_vis_region;
        }
        spice_debug("stream %d: upgrade by drawable. sized %d, box ==>",
                    stream_id, stream->current->sized_stream != NULL);
        rect_debug(&stream->current->red_drawable->bbox);
        rcc = RED_CHANNEL_CLIENT(dcc);
        channel = rcc->channel;
        upgrade_item = spice_new(UpgradeItem, 1);
        upgrade_item->refs = 1;
        red_channel_pipe_item_init(channel,
                &upgrade_item->base, PIPE_ITEM_TYPE_UPGRADE);
        upgrade_item->drawable = stream->current;
        upgrade_item->drawable->refs++;
        n_rects = pixman_region32_n_rects(&upgrade_item->drawable->tree_item.base.rgn);
        upgrade_item->rects = spice_malloc_n_m(n_rects, sizeof(SpiceRect), sizeof(SpiceClipRects));
        upgrade_item->rects->num_rects = n_rects;
        region_ret_rects(&upgrade_item->drawable->tree_item.base.rgn,
                         upgrade_item->rects->rects, n_rects);
        red_channel_client_pipe_add(rcc, &upgrade_item->base);

    } else {
        SpiceRect upgrade_area;

        region_extents(&agent->vis_region, &upgrade_area);
        spice_debug("stream %d: upgrade by screenshot. has current %d. box ==>",
                    stream_id, stream->current != NULL);
        rect_debug(&upgrade_area);
        if (update_area_limit) {
            red_update_area_till(DCC_TO_DC(dcc), &upgrade_area, 0, update_area_limit);
        } else {
            red_update_area(DCC_TO_DC(dcc), &upgrade_area, 0);
        }
        red_add_surface_area_image(dcc, 0, &upgrade_area, NULL, FALSE);
    }
clear_vis_region:
    region_clear(&agent->vis_region);
}

static void detach_stream_gracefully(DisplayChannel *display, Stream *stream,
                                     Drawable *update_area_limit)
{
    RingItem *item, *next;
    DisplayChannelClient *dcc;

    FOREACH_DCC(display, item, next, dcc) {
        dcc_detach_stream_gracefully(dcc, stream, update_area_limit);
    }
    if (stream->current) {
        detach_stream(display, stream, TRUE);
    }
}

/*
 * region  : a primary surface region. Streams that intersects with the given
 *           region will be detached.
 * drawable: If detaching the stream is triggered by the addition of a new drawable
 *           that is dependent on the given region, and the drawable is already a part
 *           of the "current tree", the drawable parameter should be set with
 *           this drawable, otherwise, it should be NULL. Then, if detaching the stream
 *           involves sending an upgrade image to the client, this drawable won't be rendered
 *           (see dcc_detach_stream_gracefully).
 */
void detach_streams_behind(DisplayChannel *display, QRegion *region, Drawable *drawable)
{
    Ring *ring = &display->streams;
    RingItem *item = ring_get_head(ring);
    RingItem *dcc_ring_item, *next;
    DisplayChannelClient *dcc;
    bool is_connected = red_channel_is_connected(RED_CHANNEL(display));

    while (item) {
        Stream *stream = SPICE_CONTAINEROF(item, Stream, link);
        int detach = 0;
        item = ring_next(ring, item);

        FOREACH_DCC(display, dcc_ring_item, next, dcc) {
            StreamAgent *agent = &dcc->stream_agents[get_stream_id(display, stream)];

            if (region_intersects(&agent->vis_region, region)) {
                dcc_detach_stream_gracefully(dcc, stream, drawable);
                detach = 1;
                spice_debug("stream %d", get_stream_id(display, stream));
            }
        }
        if (detach && stream->current) {
            detach_stream(display, stream, TRUE);
        } else if (!is_connected) {
            if (stream->current &&
                region_intersects(&stream->current->tree_item.base.rgn, region)) {
                detach_stream(display, stream, TRUE);
            }
        }
    }
}

//static void display_channel_streams_timeout(DisplayChannel *display)
//{
//    Ring *ring = &display->streams;
//    RingItem *item;
//
//    red_time_t now = red_get_monotonic_time();
//    item = ring_get_head(ring);
//    while (item) {
//        Stream *stream = SPICE_CONTAINEROF(item, Stream, link);
//        item = ring_next(ring, item);
//        if (now >= (stream->last_time + RED_STREAM_TIMEOUT)) {
//            detach_stream_gracefully(display, stream, NULL);
//            stream_stop(display, stream);
//        }
//    }
//}

//static void dcc_create_all_streams(DisplayChannelClient *dcc)
//{
//    Ring *ring = &DCC_TO_DC(dcc)->streams;
//    RingItem *item = ring;
//
//    while ((item = ring_next(ring, item))) {
//        Stream *stream = SPICE_CONTAINEROF(item, Stream, link);
//        if (!dcc_create_stream(dcc, stream)) {
//            stream_stop(DCC_TO_DC(dcc), stream);
//            return;
//        }
//    }
//}

static void dcc_init_stream_agents(DisplayChannelClient *dcc)
{
    int i;
    DisplayChannel *display = DCC_TO_DC(dcc);
    RedChannel *channel = RED_CHANNEL_CLIENT(dcc)->channel;

    for (i = 0; i < NUM_STREAMS; i++) {
        StreamAgent *agent = &dcc->stream_agents[i];
        agent->stream = &display->streams_buf[i];
        region_init(&agent->vis_region);
        region_init(&agent->clip);
        red_channel_pipe_item_init(channel, &agent->create_item, PIPE_ITEM_TYPE_STREAM_CREATE);
        red_channel_pipe_item_init(channel, &agent->destroy_item, PIPE_ITEM_TYPE_STREAM_DESTROY);
    }
    dcc->use_video_encoder_rate_control =
        red_channel_client_test_remote_cap(RED_CHANNEL_CLIENT(dcc), SPICE_DISPLAY_CAP_STREAM_REPORT);
}

static void dcc_destroy_stream_agents(DisplayChannelClient *dcc)
{
    int i;

    for (i = 0; i < NUM_STREAMS; i++) {
        StreamAgent *agent = &dcc->stream_agents[i];
        region_destroy(&agent->vis_region);
        region_destroy(&agent->clip);
        if (agent->video_encoder) {
            agent->video_encoder->destroy(agent->video_encoder);
            agent->video_encoder = NULL;
        }
    }
}

static void red_get_area(DisplayChannel *display, int surface_id, const SpiceRect *area,
                         uint8_t *dest, int dest_stride, int update)
{
    SpiceCanvas *canvas;
    RedSurface *surface;

    surface = &display->surfaces[surface_id];
    if (update) {
        red_update_area(display, area, surface_id);
    }

    canvas = surface->context.canvas;
    canvas->ops->read_bits(canvas, dest, dest_stride, area);
}

static int rgb32_data_has_alpha(int width, int height, size_t stride,
                                uint8_t *data, int *all_set_out)
{
    uint32_t *line, *end, alpha;
    int has_alpha;

    has_alpha = FALSE;
    while (height-- > 0) {
        line = (uint32_t *)data;
        end = line + width;
        data += stride;
        while (line != end) {
            alpha = *line & 0xff000000U;
            if (alpha != 0) {
                has_alpha = TRUE;
                if (alpha != 0xff000000U) {
                    *all_set_out = FALSE;
                    return TRUE;
                }
            }
            line++;
        }
    }

    *all_set_out = has_alpha;
    return has_alpha;
}

static inline int red_handle_self_bitmap(RedWorker *worker, Drawable *drawable)
{
    DisplayChannel *display = worker->display_channel;
    SpiceImage *image;
    int32_t width;
    int32_t height;
    uint8_t *dest;
    int dest_stride;
    RedSurface *surface;
    int bpp;
    int all_set;
    RedDrawable *red_drawable = drawable->red_drawable;

    if (!red_drawable->self_bitmap) {
        return TRUE;
    }

    surface = &display->surfaces[drawable->surface_id];

    bpp = SPICE_SURFACE_FMT_DEPTH(surface->context.format) / 8;

    width = red_drawable->self_bitmap_area.right
            - red_drawable->self_bitmap_area.left;
    height = red_drawable->self_bitmap_area.bottom
            - red_drawable->self_bitmap_area.top;
    dest_stride = SPICE_ALIGN(width * bpp, 4);

    image = spice_new0(SpiceImage, 1);
    image->descriptor.type = SPICE_IMAGE_TYPE_BITMAP;
    image->descriptor.flags = 0;

    QXL_SET_IMAGE_ID(image, QXL_IMAGE_GROUP_RED, ++worker->bits_unique);
    image->u.bitmap.flags = surface->context.top_down ? SPICE_BITMAP_FLAGS_TOP_DOWN : 0;
    image->u.bitmap.format = spice_bitmap_from_surface_type(surface->context.format);
    image->u.bitmap.stride = dest_stride;
    image->descriptor.width = image->u.bitmap.x = width;
    image->descriptor.height = image->u.bitmap.y = height;
    image->u.bitmap.palette = NULL;

    dest = (uint8_t *)spice_malloc_n(height, dest_stride);
    image->u.bitmap.data = spice_chunks_new_linear(dest, height * dest_stride);
    image->u.bitmap.data->flags |= SPICE_CHUNKS_FLAGS_FREE;

    red_get_area(display, drawable->surface_id,
                 &red_drawable->self_bitmap_area, dest, dest_stride, TRUE);

    /* For 32bit non-primary surfaces we need to keep any non-zero
       high bytes as the surface may be used as source to an alpha_blend */
    if (!is_primary_surface(display, drawable->surface_id) &&
        image->u.bitmap.format == SPICE_BITMAP_FMT_32BIT &&
        rgb32_data_has_alpha(width, height, dest_stride, dest, &all_set)) {
        if (all_set) {
            image->descriptor.flags |= SPICE_IMAGE_FLAGS_HIGH_BITS_SET;
        } else {
            image->u.bitmap.format = SPICE_BITMAP_FMT_RGBA;
        }
    }

    red_drawable->self_bitmap_image = image;
    return TRUE;
}

static bool free_one_drawable(DisplayChannel *display, int force_glz_free)
{
    RingItem *ring_item = ring_get_tail(&display->current_list);
    Drawable *drawable;
    Container *container;

    if (!ring_item) {
        return FALSE;
    }
    drawable = SPICE_CONTAINEROF(ring_item, Drawable, list_link);
    if (force_glz_free) {
        RingItem *glz_item, *next_item;
        RedGlzDrawable *glz;
        DRAWABLE_FOREACH_GLZ_SAFE(drawable, glz_item, next_item, glz) {
            dcc_free_glz_drawable(glz->dcc, glz);
        }
    }
    red_draw_drawable(display, drawable);
    container = drawable->tree_item.base.container;

    current_remove_drawable(display, drawable);
    container_cleanup(container);
    return TRUE;
}

static Drawable *get_drawable(RedWorker *worker, uint8_t effect, RedDrawable *red_drawable,
                              uint32_t group_id)
{
    DisplayChannel *display = worker->display_channel;
    Drawable *drawable;
    int x;

    VALIDATE_SURFACE_RETVAL(display, red_drawable->surface_id, NULL)
    if (!validate_drawable_bbox(worker, red_drawable)) {
        rendering_incorrect(__func__);
        return NULL;
    }
    for (x = 0; x < 3; ++x) {
        if (red_drawable->surface_deps[x] != -1) {
            VALIDATE_SURFACE_RETVAL(display, red_drawable->surface_deps[x], NULL)
        }
    }

    while (!(drawable = drawable_try_new(display))) {
        if (!free_one_drawable(display, FALSE))
            return NULL;
    }

    bzero(drawable, sizeof(Drawable));
    drawable->refs = 1;
    drawable->creation_time = drawable->first_frame_time = red_get_monotonic_time();
    ring_item_init(&drawable->list_link);
    ring_item_init(&drawable->surface_list_link);
    ring_item_init(&drawable->tree_item.base.siblings_link);
    drawable->tree_item.base.type = TREE_ITEM_TYPE_DRAWABLE;
    region_init(&drawable->tree_item.base.rgn);
    drawable->tree_item.effect = effect;
    drawable->red_drawable = red_drawable_ref(red_drawable);
    drawable->group_id = group_id;

    drawable->surface_id = red_drawable->surface_id;
    memcpy(drawable->surface_deps, red_drawable->surface_deps, sizeof(drawable->surface_deps));
    ring_init(&drawable->pipes);
    ring_init(&drawable->glz_ring);

    drawable->process_commands_generation = worker->process_commands_generation;
    return drawable;
}

static inline int red_handle_depends_on_target_surface(DisplayChannel *display, uint32_t surface_id)
{
    RedSurface *surface;
    RingItem *ring_item;

    surface = &display->surfaces[surface_id];

    while ((ring_item = ring_get_tail(&surface->depend_on_me))) {
        Drawable *drawable;
        DependItem *depended_item = SPICE_CONTAINEROF(ring_item, DependItem, ring_item);
        drawable = depended_item->drawable;
        surface_flush(display, drawable->surface_id, &drawable->red_drawable->bbox);
    }

    return TRUE;
}

static inline void add_to_surface_dependency(DisplayChannel *display, int depend_on_surface_id,
                                             DependItem *depend_item, Drawable *drawable)
{
    RedSurface *surface;

    if (depend_on_surface_id == -1) {
        depend_item->drawable = NULL;
        return;
    }

    surface = &display->surfaces[depend_on_surface_id];

    depend_item->drawable = drawable;
    ring_add(&surface->depend_on_me, &depend_item->ring_item);
}

static inline int red_handle_surfaces_dependencies(DisplayChannel *display, Drawable *drawable)
{
    int x;

    for (x = 0; x < 3; ++x) {
        // surface self dependency is handled by shadows in "current", or by
        // handle_self_bitmap
        if (drawable->surface_deps[x] != drawable->surface_id) {
            add_to_surface_dependency(display, drawable->surface_deps[x],
                                      &drawable->depend_items[x], drawable);

            if (drawable->surface_deps[x] == 0) {
                QRegion depend_region;
                region_init(&depend_region);
                region_add(&depend_region, &drawable->red_drawable->surfaces_rects[x]);
                detach_streams_behind(display, &depend_region, NULL);
            }
        }
    }

    return TRUE;
}

static inline void red_inc_surfaces_drawable_dependencies(DisplayChannel *display, Drawable *drawable)
{
    int x;
    int surface_id;
    RedSurface *surface;

    for (x = 0; x < 3; ++x) {
        surface_id = drawable->surface_deps[x];
        if (surface_id == -1) {
            continue;
        }
        surface = &display->surfaces[surface_id];
        surface->refs++;
    }
}

static inline void red_process_draw(RedWorker *worker, RedDrawable *red_drawable,
                                    uint32_t group_id)
{
    DisplayChannel *display = worker->display_channel;
    int surface_id;
    Drawable *drawable = get_drawable(worker, red_drawable->effect, red_drawable, group_id);
		
    if (!drawable) {
        return;
    }

    red_drawable->mm_time = reds_get_mm_time();
    surface_id = drawable->surface_id;

    display->surfaces[surface_id].refs++;

    region_add(&drawable->tree_item.base.rgn, &red_drawable->bbox);

    if (red_drawable->clip.type == SPICE_CLIP_TYPE_RECTS) {
        QRegion rgn;

        region_init(&rgn);
        region_add_clip_rects(&rgn, red_drawable->clip.rects);
        region_and(&drawable->tree_item.base.rgn, &rgn);
        region_destroy(&rgn);
    }

    /*
        surface->refs is affected by a drawable (that is
        dependent on the surface) as long as the drawable is alive.
        However, surface->depend_on_me is affected by a drawable only
        as long as it is in the current tree (hasn't been rendered yet).
    */
    red_inc_surfaces_drawable_dependencies(worker->display_channel, drawable);

    if (region_is_empty(&drawable->tree_item.base.rgn)) {
        goto cleanup;
    }

    if (!red_handle_self_bitmap(worker, drawable)) {
        goto cleanup;
    }

    if (!red_handle_depends_on_target_surface(worker->display_channel, surface_id)) {
        goto cleanup;
    }

    if (!red_handle_surfaces_dependencies(worker->display_channel, drawable)) {
        goto cleanup;
    }

    if (display_channel_add_drawable(worker->display_channel, drawable)) {
        red_pipes_add_drawable(worker->display_channel, drawable);
        //force render all
        //akumas 2015.12.2
        //red_draw_qxl_drawable(worker->display_channel, drawable);
    }
cleanup:
    display_channel_drawable_unref(display, drawable);
}


static inline void red_process_surface(RedWorker *worker, RedSurfaceCmd *surface,
                                       uint32_t group_id, int loadvm)
{
    DisplayChannel *display = worker->display_channel;
    uint32_t surface_id;
    RedSurface *red_surface;
    uint8_t *data;

    surface_id = surface->surface_id;
    if SPICE_UNLIKELY(surface_id >= display->n_surfaces) {
        goto exit;
    }

    red_surface = &display->surfaces[surface_id];

    switch (surface->type) {
    case QXL_SURFACE_CMD_CREATE: {
        uint32_t height = surface->u.surface_create.height;
        int32_t stride = surface->u.surface_create.stride;
        int reloaded_surface = loadvm || (surface->flags & QXL_SURF_FLAG_KEEP_DATA);

        if (red_surface->refs) {
            spice_warning("avoiding creating a surface twice");
            break;
        }
        data = surface->u.surface_create.data;
        if (stride < 0) {
            data -= (int32_t)(stride * (height - 1));
        }
        red_create_surface(worker->display_channel, surface_id, surface->u.surface_create.width,
                           height, stride, surface->u.surface_create.format, data,
                           reloaded_surface,
                           // reloaded surfaces will be sent on demand
                           !reloaded_surface);
        set_surface_release_info(&red_surface->create, surface->release_info, group_id);
        break;
    }
    case QXL_SURFACE_CMD_DESTROY:
        if (!red_surface->refs) {
            spice_warning("avoiding destroying a surface twice");
            break;
        }
        set_surface_release_info(&red_surface->destroy, surface->release_info, group_id);
        red_handle_depends_on_target_surface(display, surface_id);
        /* note that red_handle_depends_on_target_surface must be called before current_remove_all.
           otherwise "current" will hold items that other drawables may depend on, and then
           current_remove_all will remove them from the pipe. */
        current_remove_all(display, surface_id);
        red_clear_surface_drawables_from_pipes(display, surface_id, FALSE);
        display_channel_surface_unref(display, surface_id);
        break;
    default:
            spice_error("unknown surface command");
    };
exit:
    red_put_surface_cmd(surface);
    free(surface);
}

static SpiceCanvas *image_surfaces_get(SpiceImageSurfaces *surfaces,
                                       uint32_t surface_id)
{
    DisplayChannel *display;

    display = SPICE_CONTAINEROF(surfaces, DisplayChannel, image_surfaces);
    VALIDATE_SURFACE_RETVAL(display, surface_id, NULL);

    return display->surfaces[surface_id].context.canvas;
}

static void image_surface_init(DisplayChannel *display)
{
    static SpiceImageSurfacesOps image_surfaces_ops = {
        image_surfaces_get,
    };

    display->image_surfaces.ops = &image_surfaces_ops;
}

static void red_draw_qxl_drawable(DisplayChannel *display, Drawable *drawable)
{
    RedSurface *surface;
    SpiceCanvas *canvas;
    SpiceClip clip = drawable->red_drawable->clip;

    surface = &display->surfaces[drawable->surface_id];
    canvas = surface->context.canvas;

    image_cache_aging(&display->image_cache);

    region_add(&surface->draw_dirty_region, &drawable->red_drawable->bbox);

    switch (drawable->red_drawable->type) {
    case QXL_DRAW_FILL: {
        SpiceFill fill = drawable->red_drawable->u.fill;
        SpiceImage img1, img2;
        image_cache_localize_brush(&display->image_cache, &fill.brush, &img1);
        image_cache_localize_mask(&display->image_cache, &fill.mask, &img2);
        canvas->ops->draw_fill(canvas, &drawable->red_drawable->bbox,
                               &clip, &fill);
        //spice_info("draw fill in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_OPAQUE: {
        SpiceOpaque opaque = drawable->red_drawable->u.opaque;
        SpiceImage img1, img2, img3;
        image_cache_localize_brush(&display->image_cache, &opaque.brush, &img1);
        image_cache_localize(&display->image_cache, &opaque.src_bitmap, &img2, drawable);
        image_cache_localize_mask(&display->image_cache, &opaque.mask, &img3);
        canvas->ops->draw_opaque(canvas, &drawable->red_drawable->bbox, &clip, &opaque);
        //spice_info("draw opaque in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_COPY: {
        SpiceCopy copy = drawable->red_drawable->u.copy;
        SpiceImage img1, img2;
        image_cache_localize(&display->image_cache, &copy.src_bitmap, &img1, drawable);
        image_cache_localize_mask(&display->image_cache, &copy.mask, &img2);
        canvas->ops->draw_copy(canvas, &drawable->red_drawable->bbox,
                               &clip, &copy);
				//spice_info("draw copy in surface %d", drawable->surface_id);                               
        break;
    }
    case QXL_DRAW_TRANSPARENT: {
        SpiceTransparent transparent = drawable->red_drawable->u.transparent;
        SpiceImage img1;
        image_cache_localize(&display->image_cache, &transparent.src_bitmap, &img1, drawable);
        canvas->ops->draw_transparent(canvas,
                                      &drawable->red_drawable->bbox, &clip, &transparent);
        //spice_info("draw transparent in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_ALPHA_BLEND: {
        SpiceAlphaBlend alpha_blend = drawable->red_drawable->u.alpha_blend;
        SpiceImage img1;
        image_cache_localize(&display->image_cache, &alpha_blend.src_bitmap, &img1, drawable);
        canvas->ops->draw_alpha_blend(canvas,
                                      &drawable->red_drawable->bbox, &clip, &alpha_blend);
        //spice_info("draw alpha blend in surface %d", drawable->surface_id);
        break;
    }
    case QXL_COPY_BITS: {
        canvas->ops->copy_bits(canvas, &drawable->red_drawable->bbox,
                               &clip, &drawable->red_drawable->u.copy_bits.src_pos);
        //spice_info("copy bits in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_BLEND: {
        SpiceBlend blend = drawable->red_drawable->u.blend;
        SpiceImage img1, img2;
        image_cache_localize(&display->image_cache, &blend.src_bitmap, &img1, drawable);
        image_cache_localize_mask(&display->image_cache, &blend.mask, &img2);
        canvas->ops->draw_blend(canvas, &drawable->red_drawable->bbox,
                                &clip, &blend);
        //spice_info("draw blend in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_BLACKNESS: {
        SpiceBlackness blackness = drawable->red_drawable->u.blackness;
        SpiceImage img1;
        image_cache_localize_mask(&display->image_cache, &blackness.mask, &img1);
        canvas->ops->draw_blackness(canvas,
                                    &drawable->red_drawable->bbox, &clip, &blackness);
        //spice_info("draw blackness in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_WHITENESS: {
        SpiceWhiteness whiteness = drawable->red_drawable->u.whiteness;
        SpiceImage img1;
        image_cache_localize_mask(&display->image_cache, &whiteness.mask, &img1);
        canvas->ops->draw_whiteness(canvas,
                                    &drawable->red_drawable->bbox, &clip, &whiteness);
        //spice_info("draw whiteness in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_INVERS: {
        SpiceInvers invers = drawable->red_drawable->u.invers;
        SpiceImage img1;
        image_cache_localize_mask(&display->image_cache, &invers.mask, &img1);
        canvas->ops->draw_invers(canvas,
                                 &drawable->red_drawable->bbox, &clip, &invers);
        //spice_info("draw invers in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_ROP3: {
        SpiceRop3 rop3 = drawable->red_drawable->u.rop3;
        SpiceImage img1, img2, img3;
        image_cache_localize_brush(&display->image_cache, &rop3.brush, &img1);
        image_cache_localize(&display->image_cache, &rop3.src_bitmap, &img2, drawable);
        image_cache_localize_mask(&display->image_cache, &rop3.mask, &img3);
        canvas->ops->draw_rop3(canvas, &drawable->red_drawable->bbox,
                               &clip, &rop3);
        //spice_info("draw rop3 in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_COMPOSITE: {
        SpiceComposite composite = drawable->red_drawable->u.composite;
        SpiceImage src, mask;
        image_cache_localize(&display->image_cache, &composite.src_bitmap, &src, drawable);
        if (composite.mask_bitmap)
            image_cache_localize(&display->image_cache, &composite.mask_bitmap, &mask, drawable);
        canvas->ops->draw_composite(canvas, &drawable->red_drawable->bbox,
                                    &clip, &composite);
        //spice_info("draw composite in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_STROKE: {
        SpiceStroke stroke = drawable->red_drawable->u.stroke;
        SpiceImage img1;
        image_cache_localize_brush(&display->image_cache, &stroke.brush, &img1);
        canvas->ops->draw_stroke(canvas,
                                 &drawable->red_drawable->bbox, &clip, &stroke);
        //spice_info("draw stroke in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_TEXT: {
        SpiceText text = drawable->red_drawable->u.text;
        SpiceImage img1, img2;
        image_cache_localize_brush(&display->image_cache, &text.fore_brush, &img1);
        image_cache_localize_brush(&display->image_cache, &text.back_brush, &img2);
        canvas->ops->draw_text(canvas, &drawable->red_drawable->bbox,
                               &clip, &text);
        //spice_info("draw text in surface %d", drawable->surface_id);
        break;
    }
    default:
        spice_warning("invalid type");
    }
}

static void red_draw_drawable(DisplayChannel *display, Drawable *drawable)
{
    red_flush_source_surfaces(display, drawable);
    red_draw_qxl_drawable(display, drawable);
}

static void validate_area(DisplayChannel *display, const SpiceRect *area, uint32_t surface_id)
{
    RedSurface *surface;

    surface = &display->surfaces[surface_id];
    if (!surface->context.canvas_draws_on_surface) {
        SpiceCanvas *canvas = surface->context.canvas;
        int h;
        int stride = surface->context.stride;
        uint8_t *line_0 = surface->context.line_0;

        if (!(h = area->bottom - area->top)) {
            return;
        }

        spice_assert(stride < 0);
        uint8_t *dest = line_0 + (area->top * stride) + area->left * sizeof(uint32_t);
        dest += (h - 1) * stride;
        canvas->ops->read_bits(canvas, dest, -stride, area);
    }
}

/*
    Renders drawables for updating the requested area, but only drawables that are older
    than 'last' (exclusive).
*/
static void red_update_area_till(DisplayChannel *display, const SpiceRect *area, int surface_id,
                                 Drawable *last)
{
    RedSurface *surface;
    Drawable *surface_last = NULL;
    Ring *ring;
    RingItem *ring_item;
    Drawable *now;
    QRegion rgn;

    spice_assert(last);
    spice_assert(ring_item_is_linked(&last->list_link));

    surface = &display->surfaces[surface_id];

    if (surface_id != last->surface_id) {
        // find the nearest older drawable from the appropriate surface
        ring = &display->current_list;
        ring_item = &last->list_link;
        while ((ring_item = ring_next(ring, ring_item))) {
            now = SPICE_CONTAINEROF(ring_item, Drawable, list_link);
            if (now->surface_id == surface_id) {
                surface_last = now;
                break;
            }
        }
    } else {
        ring_item = ring_next(&surface->current_list, &last->surface_list_link);
        if (ring_item) {
            surface_last = SPICE_CONTAINEROF(ring_item, Drawable, surface_list_link);
        }
    }

    if (!surface_last) {
        return;
    }

    ring = &surface->current_list;
    ring_item = &surface_last->surface_list_link;

    region_init(&rgn);
    region_add(&rgn, area);

    // find the first older drawable that intersects with the area
    do {
        now = SPICE_CONTAINEROF(ring_item, Drawable, surface_list_link);
        if (region_intersects(&rgn, &now->tree_item.base.rgn)) {
            surface_last = now;
            break;
        }
    } while ((ring_item = ring_next(ring, ring_item)));

    region_destroy(&rgn);

    if (!surface_last) {
        return;
    }

    do {
        Container *container;

        ring_item = ring_get_tail(&surface->current_list);
        now = SPICE_CONTAINEROF(ring_item, Drawable, surface_list_link);
        now->refs++;
        container = now->tree_item.base.container;
        current_remove_drawable(display, now);
        container_cleanup(container);
        /* red_draw_drawable may call red_update_area for the surfaces 'now' depends on. Notice,
           that it is valid to call red_update_area in this case and not red_update_area_till:
           It is impossible that there was newer item then 'last' in one of the surfaces
           that red_update_area is called for, Otherwise, 'now' would have already been rendered.
           See the call for red_handle_depends_on_target_surface in red_process_draw */
        red_draw_drawable(display, now);
        display_channel_drawable_unref(display, now);
    } while (now != surface_last);
    validate_area(display, area, surface_id);
}

static void red_update_area(DisplayChannel *display, const SpiceRect *area, int surface_id)
{
    RedSurface *surface;
    Ring *ring;
    RingItem *ring_item;
    QRegion rgn;
    Drawable *last;
    Drawable *now;
    spice_debug("surface %d: area ==>", surface_id);
    rect_debug(area);

    spice_return_if_fail(surface_id >= 0 && surface_id < NUM_SURFACES);
    spice_return_if_fail(area);
    spice_return_if_fail(area->left >= 0 && area->top >= 0 &&
                         area->left < area->right && area->top < area->bottom);

    surface = &display->surfaces[surface_id];

    last = NULL;
    ring = &surface->current_list;
    ring_item = ring;

    region_init(&rgn);
    region_add(&rgn, area);
    while ((ring_item = ring_next(ring, ring_item))) {
        now = SPICE_CONTAINEROF(ring_item, Drawable, surface_list_link);
        if (region_intersects(&rgn, &now->tree_item.base.rgn)) {
            last = now;
            break;
        }
    }
    region_destroy(&rgn);

    if (!last) {
        validate_area(display, area, surface_id);
        return;
    }

    do {
        Container *container;

        ring_item = ring_get_tail(&surface->current_list);
        now = SPICE_CONTAINEROF(ring_item, Drawable, surface_list_link);
        now->refs++;
        container = now->tree_item.base.container;
        current_remove_drawable(display, now);
        container_cleanup(container);
        red_draw_drawable(display, now);
        display_channel_drawable_unref(display, now);
    } while (now != last);
    validate_area(display, area, surface_id);
}

static int red_process_cursor(RedWorker *worker, uint32_t max_pipe_size, int *ring_is_empty)
{
    QXLCommandExt ext_cmd;
    int n = 0;

    if (!worker->running) {
        *ring_is_empty = TRUE;
        return n;
    }

    *ring_is_empty = FALSE;
    while (!cursor_is_connected(worker) ||
           red_channel_min_pipe_size(RED_CHANNEL(worker->cursor_channel)) <= max_pipe_size) {
        if (!worker->qxl->st->qif->get_cursor_command(worker->qxl, &ext_cmd)) {
            *ring_is_empty = TRUE;
            if (worker->cursor_poll_tries < CMD_RING_POLL_RETRIES) {
                worker->cursor_poll_tries++;
                worker->event_timeout = MIN(worker->event_timeout, CMD_RING_POLL_TIMEOUT);
                return n;
            }
            if (worker->cursor_poll_tries > CMD_RING_POLL_RETRIES ||
                worker->qxl->st->qif->req_cursor_notification(worker->qxl)) {
                worker->cursor_poll_tries++;
                return n;
            }
            continue;
        }
        worker->cursor_poll_tries = 0;
        switch (ext_cmd.cmd.type) {
        case QXL_CMD_CURSOR: {
            RedCursorCmd *cursor = spice_new0(RedCursorCmd, 1);

            if (red_get_cursor_cmd(&worker->mem_slots, ext_cmd.group_id,
                                    cursor, ext_cmd.cmd.data)) {
                free(cursor);
                break;
            }

            cursor_channel_process_cmd(worker->cursor_channel, cursor, ext_cmd.group_id);
            break;
        }
        default:
            spice_warning("bad command type");
        }
        n++;
    }
    return n;
}

static RedDrawable *red_drawable_new(RedWorker *worker)
{
    RedDrawable * red = spice_new0(RedDrawable, 1);

    red->refs = 1;
    worker->red_drawable_count++;

    return red;
}

static int red_process_commands(RedWorker *worker, uint32_t max_pipe_size, int *ring_is_empty)
{
    QXLCommandExt ext_cmd;
    int n = 0;
    uint64_t start = red_get_monotonic_time();

    if (!worker->running) {
        *ring_is_empty = TRUE;
        return n;
    }

    worker->process_commands_generation++;
    *ring_is_empty = FALSE;
    while (!display_is_connected(worker) ||
           // TODO: change to average pipe size?
           red_channel_min_pipe_size(RED_CHANNEL(worker->display_channel)) <= max_pipe_size) {
        if (!worker->qxl->st->qif->get_command(worker->qxl, &ext_cmd)) {
            *ring_is_empty = TRUE;;
            if (worker->display_poll_tries < CMD_RING_POLL_RETRIES) {
                worker->display_poll_tries++;
                worker->event_timeout = MIN(worker->event_timeout, CMD_RING_POLL_TIMEOUT);
                return n;
            }
            if (worker->display_poll_tries > CMD_RING_POLL_RETRIES ||
                         worker->qxl->st->qif->req_cmd_notification(worker->qxl)) {
                worker->display_poll_tries++;
                return n;
            }
            continue;
        }

        if (worker->record_fd)
            red_record_qxl_command(worker->record_fd, &worker->mem_slots, ext_cmd,
                                   stat_now(worker->clockid));

        stat_inc_counter(worker->command_counter, 1);
        worker->display_poll_tries = 0;
        switch (ext_cmd.cmd.type) {
        case QXL_CMD_DRAW: {
            RedDrawable *red_drawable = red_drawable_new(worker); // returns with 1 ref

            if (!red_get_drawable(&worker->mem_slots, ext_cmd.group_id,
                                 red_drawable, ext_cmd.cmd.data, ext_cmd.flags)) {
                red_process_draw(worker, red_drawable, ext_cmd.group_id);
            }
            // release the red_drawable
            red_drawable_unref(worker, red_drawable, ext_cmd.group_id);
            break;
        }
        case QXL_CMD_UPDATE: {
            RedUpdateCmd update;
            QXLReleaseInfoExt release_info_ext;

            if (red_get_update_cmd(&worker->mem_slots, ext_cmd.group_id,
                                   &update, ext_cmd.cmd.data)) {
                break;
            }
            if (!validate_surface(worker->display_channel, update.surface_id)) {
                rendering_incorrect("QXL_CMD_UPDATE");
                break;
            }
            spice_info("process update area: [ %d, %d, %d, %d]", update.area.left, update.area.top, update.area.right, update.area.bottom);
            red_update_area(worker->display_channel, &update.area, update.surface_id);
            worker->qxl->st->qif->notify_update(worker->qxl, update.update_id);
            release_info_ext.group_id = ext_cmd.group_id;
            release_info_ext.info = update.release_info;
            worker->qxl->st->qif->release_resource(worker->qxl, release_info_ext);
            red_put_update_cmd(&update);
            break;
        }
        case QXL_CMD_MESSAGE: {
            RedMessage message;
            QXLReleaseInfoExt release_info_ext;

            if (red_get_message(&worker->mem_slots, ext_cmd.group_id,
                                &message, ext_cmd.cmd.data)) {
                break;
            }
#ifdef DEBUG
            /* alert: accessing message.data is insecure */
            spice_warning("MESSAGE: %s", message.data);
#endif
            release_info_ext.group_id = ext_cmd.group_id;
            release_info_ext.info = message.release_info;
            worker->qxl->st->qif->release_resource(worker->qxl, release_info_ext);
            red_put_message(&message);
            break;
        }
        case QXL_CMD_SURFACE: {
            RedSurfaceCmd *surface = spice_new0(RedSurfaceCmd, 1);

            if (red_get_surface_cmd(&worker->mem_slots, ext_cmd.group_id,
                                    surface, ext_cmd.cmd.data)) {
                free(surface);
                break;
            }
            red_process_surface(worker, surface, ext_cmd.group_id, FALSE);
            break;
        }
        default:
            spice_error("bad command type");
        }
        n++;
        if ((worker->display_channel &&
             red_channel_all_blocked(&worker->display_channel->common.base))
            || red_get_monotonic_time() - start > 10 * 1000 * 1000) {
            worker->event_timeout = 0;
            return n;
        }
    }
    return n;
}

#define RED_RELEASE_BUNCH_SIZE 64

static void red_free_some(RedWorker *worker)
{
    DisplayChannel *display = worker->display_channel;
    int n = 0;
    DisplayChannelClient *dcc;
    RingItem *item, *next;

    spice_debug("#draw=%d, #red_draw=%d, #glz_draw=%d", display->drawable_count,
                worker->red_drawable_count, worker->glz_drawable_count);
    FOREACH_DCC(worker->display_channel, item, next, dcc) {
        GlzSharedDictionary *glz_dict = dcc ? dcc->glz_dict : NULL;

        if (glz_dict) {
            // encoding using the dictionary is prevented since the following operations might
            // change the dictionary
            pthread_rwlock_wrlock(&glz_dict->encode_lock);
            n = red_display_free_some_independent_glz_drawables(dcc);
        }
    }

    while (!ring_is_empty(&display->current_list) && n++ < RED_RELEASE_BUNCH_SIZE) {
        free_one_drawable(display, TRUE);
    }

    FOREACH_DCC(worker->display_channel, item, next, dcc) {
        GlzSharedDictionary *glz_dict = dcc ? dcc->glz_dict : NULL;

        if (glz_dict) {
            pthread_rwlock_unlock(&glz_dict->encode_lock);
        }
    }
}

static void red_current_flush(DisplayChannel *display, int surface_id)
{
    while (!ring_is_empty(&display->surfaces[surface_id].current_list)) {
        free_one_drawable(display, FALSE);
    }
    current_remove_all(display, surface_id);
}

// adding the pipe item after pos. If pos == NULL, adding to head.
static ImageItem *red_add_surface_area_image(DisplayChannelClient *dcc, int surface_id,
                                             SpiceRect *area, PipeItem *pos, int can_lossy)
{
    DisplayChannel *display = DCC_TO_DC(dcc);
    RedChannel *channel = RED_CHANNEL(display);
    RedSurface *surface = &display->surfaces[surface_id];
    SpiceCanvas *canvas = surface->context.canvas;
    ImageItem *item;
    int stride;
    int width;
    int height;
    int bpp;
    int all_set;

    spice_assert(area);

    width = area->right - area->left;
    height = area->bottom - area->top;
    bpp = SPICE_SURFACE_FMT_DEPTH(surface->context.format) / 8;
    stride = width * bpp;

    item = (ImageItem *)spice_malloc_n_m(height, stride, sizeof(ImageItem));

    red_channel_pipe_item_init(channel, &item->link, PIPE_ITEM_TYPE_IMAGE);

    item->refs = 1;
    item->surface_id = surface_id;
    item->image_format =
        spice_bitmap_from_surface_type(surface->context.format);
    item->image_flags = 0;
    item->pos.x = area->left;
    item->pos.y = area->top;
    item->width = width;
    item->height = height;
    item->stride = stride;
    item->top_down = surface->context.top_down;
    item->can_lossy = can_lossy;

    canvas->ops->read_bits(canvas, item->data, stride, area);

    /* For 32bit non-primary surfaces we need to keep any non-zero
       high bytes as the surface may be used as source to an alpha_blend */
    if (!is_primary_surface(display, surface_id) &&
        item->image_format == SPICE_BITMAP_FMT_32BIT &&
        rgb32_data_has_alpha(item->width, item->height, item->stride, item->data, &all_set)) {
        if (all_set) {
            item->image_flags |= SPICE_IMAGE_FLAGS_HIGH_BITS_SET;
        } else {
            item->image_format = SPICE_BITMAP_FMT_RGBA;
        }
    }

    if (!pos) {
        red_pipe_add_image_item(dcc, item);
    } else {
        red_pipe_add_image_item_after(dcc, item, pos);
    }

    release_image_item(item);

    return item;
}

static void red_push_surface_image(DisplayChannelClient *dcc, int surface_id)
{
    DisplayChannel *display;
    SpiceRect area;
    RedSurface *surface;

    if (!dcc) {
        return;
    }

    display = DCC_TO_DC(dcc);
    surface = &display->surfaces[surface_id];
    if (!surface->context.canvas) {
        return;
    }
    area.top = area.left = 0;
    area.right = surface->context.width;
    area.bottom = surface->context.height;

    /* not allowing lossy compression because probably, especially if it is a primary surface,
       it combines both "picture-like" areas with areas that are more "artificial"*/
    red_add_surface_area_image(dcc, surface_id, &area, NULL, FALSE);
    red_channel_client_push(RED_CHANNEL_CLIENT(dcc));
}

static RedCompressBuf *compress_buf_new(void)
{
    return g_slice_new(RedCompressBuf);
}

static inline void compress_buf_free(RedCompressBuf *buf)
{
    g_slice_free(RedCompressBuf, buf);
}

static void marshaller_compress_buf_free(uint8_t *data, void *opaque)
{
    compress_buf_free((RedCompressBuf *) opaque);
}

static void marshaller_add_compressed(SpiceMarshaller *m,
                                      RedCompressBuf *comp_buf, size_t size)
{
    size_t max = size;
    size_t now;
    do {
        spice_return_if_fail(comp_buf);
        now = MIN(sizeof(comp_buf->buf), max);
        max -= now;
        spice_marshaller_add_ref_full(m, comp_buf->buf.bytes, now,
                                      marshaller_compress_buf_free, comp_buf);
        comp_buf = comp_buf->send_next;
    } while (max);
}


static void fill_base(SpiceMarshaller *base_marshaller, Drawable *drawable)
{
    SpiceMsgDisplayBase base;

    base.surface_id = drawable->surface_id;
    base.box = drawable->red_drawable->bbox;
    base.clip = drawable->red_drawable->clip;

    spice_marshall_DisplayBase(base_marshaller, &base);
}

static inline void fill_palette(DisplayChannelClient *dcc,
                                SpicePalette *palette,
                                uint8_t *flags)
{
    if (palette == NULL) {
        return;
    }
    if (palette->unique) {
        if (red_palette_cache_find(dcc, palette->unique)) {
            *flags |= SPICE_BITMAP_FLAGS_PAL_FROM_CACHE;
            return;
        }
        if (red_palette_cache_add(dcc, palette->unique, 1)) {
            *flags |= SPICE_BITMAP_FLAGS_PAL_CACHE_ME;
        }
    }
}

/******************************************************
 *      Global lz red drawables routines
*******************************************************/

/* if already exists, returns it. Otherwise allocates and adds it (1) to the ring tail
   in the channel (2) to the Drawable*/
static RedGlzDrawable *red_display_get_glz_drawable(DisplayChannelClient *dcc, Drawable *drawable)
{
    RedGlzDrawable *ret;
    RingItem *item, *next;

    // TODO - I don't really understand what's going on here, so doing the technical equivalent
    // now that we have multiple glz_dicts, so the only way to go from dcc to drawable glz is to go
    // over the glz_ring (unless adding some better data structure then a ring)
    DRAWABLE_FOREACH_GLZ_SAFE(drawable, item, next, ret) {
        if (ret->dcc == dcc) {
            return ret;
        }
    }

    ret = spice_new(RedGlzDrawable, 1);

    ret->dcc = dcc;
    ret->red_drawable = red_drawable_ref(drawable->red_drawable);
    ret->drawable = drawable;
    ret->group_id = drawable->group_id;
    ret->instances_count = 0;
    ring_init(&ret->instances);

    ring_item_init(&ret->link);
    ring_item_init(&ret->drawable_link);
    ring_add_before(&ret->link, &dcc->glz_drawables);
    ring_add(&drawable->glz_ring, &ret->drawable_link);
    DCC_TO_WORKER(dcc)->glz_drawable_count++;
    return ret;
}

/* allocates new instance and adds it to instances in the given drawable.
   NOTE - the caller should set the glz_instance returned by the encoder by itself.*/
static GlzDrawableInstanceItem *red_display_add_glz_drawable_instance(RedGlzDrawable *glz_drawable)
{
    spice_assert(glz_drawable->instances_count < MAX_GLZ_DRAWABLE_INSTANCES);
    // NOTE: We assume the additions are performed consecutively, without removals in the middle
    GlzDrawableInstanceItem *ret = glz_drawable->instances_pool + glz_drawable->instances_count;
    glz_drawable->instances_count++;

    ring_item_init(&ret->free_link);
    ring_item_init(&ret->glz_link);
    ring_add(&glz_drawable->instances, &ret->glz_link);
    ret->glz_instance = NULL;
    ret->red_glz_drawable = glz_drawable;

    return ret;
}

/* Remove from the to_free list and the instances_list.
   When no instance is left - the RedGlzDrawable is released too. (and the qxl drawable too, if
   it is not used by Drawable).
   NOTE - 1) can be called only by the display channel that created the drawable
          2) it is assumed that the instance was already removed from the dictionary*/
void dcc_free_glz_drawable_instance(DisplayChannelClient *dcc,
                                    GlzDrawableInstanceItem *glz_drawable_instance)
{
    DisplayChannel *display_channel = DCC_TO_DC(dcc);
    RedWorker *worker = display_channel->common.worker;
    RedGlzDrawable *glz_drawable;

    spice_assert(glz_drawable_instance);
    spice_assert(glz_drawable_instance->red_glz_drawable);

    glz_drawable = glz_drawable_instance->red_glz_drawable;

    spice_assert(glz_drawable->dcc == dcc);
    spice_assert(glz_drawable->instances_count);

    ring_remove(&glz_drawable_instance->glz_link);
    glz_drawable->instances_count--;
    // when the remove callback is performed from the channel that the
    // drawable belongs to, the instance is not added to the 'to_free' list
    if (ring_item_is_linked(&glz_drawable_instance->free_link)) {
        ring_remove(&glz_drawable_instance->free_link);
    }

    if (ring_is_empty(&glz_drawable->instances)) {
        spice_assert(!glz_drawable->instances_count);

        Drawable *drawable = glz_drawable->drawable;

        if (drawable) {
            ring_remove(&glz_drawable->drawable_link);
        }
        red_drawable_unref(worker, glz_drawable->red_drawable,
                           glz_drawable->group_id);
        worker->glz_drawable_count--;
        if (ring_item_is_linked(&glz_drawable->link)) {
            ring_remove(&glz_drawable->link);
        }
        free(glz_drawable);
    }
}

static void red_display_handle_glz_drawables_to_free(DisplayChannelClient* dcc)
{
    RingItem *ring_link;

    if (!dcc->glz_dict) {
        return;
    }
    pthread_mutex_lock(&dcc->glz_drawables_inst_to_free_lock);
    while ((ring_link = ring_get_head(&dcc->glz_drawables_inst_to_free))) {
        GlzDrawableInstanceItem *drawable_instance = SPICE_CONTAINEROF(ring_link,
                                                                 GlzDrawableInstanceItem,
                                                                 free_link);
        dcc_free_glz_drawable_instance(dcc, drawable_instance);
    }
    pthread_mutex_unlock(&dcc->glz_drawables_inst_to_free_lock);
}

/*
 * Releases all the instances of the drawable from the dictionary and the display channel client.
 * The release of the last instance will also release the drawable itself and the qxl drawable
 * if possible.
 * NOTE - the caller should prevent encoding using the dictionary during this operation
 */
static void dcc_free_glz_drawable(DisplayChannelClient *dcc, RedGlzDrawable *drawable)
{
    RingItem *head_instance = ring_get_head(&drawable->instances);
    int cont = (head_instance != NULL);

    while (cont) {
        if (drawable->instances_count == 1) {
            /* Last instance: dcc_free_glz_drawable_instance will free the drawable */
            cont = FALSE;
        }
        GlzDrawableInstanceItem *instance = SPICE_CONTAINEROF(head_instance,
                                                        GlzDrawableInstanceItem,
                                                        glz_link);
        if (!ring_item_is_linked(&instance->free_link)) {
            // the instance didn't get out from window yet
            glz_enc_dictionary_remove_image(dcc->glz_dict->dict,
                                            instance->glz_instance,
                                            &dcc->glz_data.usr);
        }
        dcc_free_glz_drawable_instance(dcc, instance);

        if (cont) {
            head_instance = ring_get_head(&drawable->instances);
        }
    }
}

/* Clear all lz drawables - enforce their removal from the global dictionary.
   NOTE - prevents encoding using the dictionary during the operation*/
static void red_display_client_clear_glz_drawables(DisplayChannelClient *dcc)
{
    RingItem *ring_link;
    GlzSharedDictionary *glz_dict = dcc ? dcc->glz_dict : NULL;

    if (!glz_dict) {
        return;
    }

    // assure no display channel is during global lz encoding
    pthread_rwlock_wrlock(&glz_dict->encode_lock);
    while ((ring_link = ring_get_head(&dcc->glz_drawables))) {
        RedGlzDrawable *drawable = SPICE_CONTAINEROF(ring_link, RedGlzDrawable, link);
        // no need to lock the to_free list, since we assured no other thread is encoding and
        // thus not other thread access the to_free list of the channel
        dcc_free_glz_drawable(dcc, drawable);
    }
    pthread_rwlock_unlock(&glz_dict->encode_lock);
}

static void red_display_clear_glz_drawables(DisplayChannel *display_channel)
{
    RingItem *link, *next;
    DisplayChannelClient *dcc;

    if (!display_channel) {
        return;
    }
    DCC_FOREACH_SAFE(link, next, dcc, RED_CHANNEL(display_channel)) {
        red_display_client_clear_glz_drawables(dcc);
    }
}

/*
 * Remove from the global lz dictionary some glz_drawables that have no reference to
 * Drawable (their qxl drawables are released too).
 * NOTE - the caller should prevent encoding using the dictionary during the operation
 */
static int red_display_free_some_independent_glz_drawables(DisplayChannelClient *dcc)
{
    RingItem *ring_link;
    int n = 0;

    if (!dcc) {
        return 0;
    }
    ring_link = ring_get_head(&dcc->glz_drawables);
    while ((n < RED_RELEASE_BUNCH_SIZE) && (ring_link != NULL)) {
        RedGlzDrawable *glz_drawable = SPICE_CONTAINEROF(ring_link, RedGlzDrawable, link);
        ring_link = ring_next(&dcc->glz_drawables, ring_link);
        if (!glz_drawable->drawable) {
            dcc_free_glz_drawable(dcc, glz_drawable);
            n++;
        }
    }
    return n;
}

/******************************************************
 *              Encoders callbacks
*******************************************************/
static SPICE_GNUC_NORETURN SPICE_GNUC_PRINTF(2, 3) void
quic_usr_error(QuicUsrContext *usr, const char *fmt, ...)
{
    EncoderData *usr_data = &(((QuicData *)usr)->data);
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(usr_data->message_buf, sizeof(usr_data->message_buf), fmt, ap);
    va_end(ap);
    spice_critical("%s", usr_data->message_buf);

    longjmp(usr_data->jmp_env, 1);
}

static SPICE_GNUC_NORETURN SPICE_GNUC_PRINTF(2, 3) void
lz_usr_error(LzUsrContext *usr, const char *fmt, ...)
{
    EncoderData *usr_data = &(((LzData *)usr)->data);
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(usr_data->message_buf, sizeof(usr_data->message_buf), fmt, ap);
    va_end(ap);
    spice_critical("%s", usr_data->message_buf);

    longjmp(usr_data->jmp_env, 1);
}

static SPICE_GNUC_PRINTF(2, 3) void glz_usr_error(GlzEncoderUsrContext *usr, const char *fmt, ...)
{
    EncoderData *usr_data = &(((GlzData *)usr)->data);
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(usr_data->message_buf, sizeof(usr_data->message_buf), fmt, ap);
    va_end(ap);

    spice_critical("%s", usr_data->message_buf); // if global lz fails in the middle
                                        // the consequences are not predictable since the window
                                        // can turn to be unsynchronized between the server and
                                        // and the client
}

static SPICE_GNUC_PRINTF(2, 3) void quic_usr_warn(QuicUsrContext *usr, const char *fmt, ...)
{
    EncoderData *usr_data = &(((QuicData *)usr)->data);
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(usr_data->message_buf, sizeof(usr_data->message_buf), fmt, ap);
    va_end(ap);
    spice_warning("%s", usr_data->message_buf);
}

static SPICE_GNUC_PRINTF(2, 3) void lz_usr_warn(LzUsrContext *usr, const char *fmt, ...)
{
    EncoderData *usr_data = &(((LzData *)usr)->data);
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(usr_data->message_buf, sizeof(usr_data->message_buf), fmt, ap);
    va_end(ap);
    spice_warning("%s", usr_data->message_buf);
}

static SPICE_GNUC_PRINTF(2, 3) void glz_usr_warn(GlzEncoderUsrContext *usr, const char *fmt, ...)
{
    EncoderData *usr_data = &(((GlzData *)usr)->data);
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(usr_data->message_buf, sizeof(usr_data->message_buf), fmt, ap);
    va_end(ap);
    spice_warning("%s", usr_data->message_buf);
}

static void *quic_usr_malloc(QuicUsrContext *usr, int size)
{
    return spice_malloc(size);
}

static void *lz_usr_malloc(LzUsrContext *usr, int size)
{
    return spice_malloc(size);
}

static void *glz_usr_malloc(GlzEncoderUsrContext *usr, int size)
{
    return spice_malloc(size);
}

static void quic_usr_free(QuicUsrContext *usr, void *ptr)
{
    free(ptr);
}

static void lz_usr_free(LzUsrContext *usr, void *ptr)
{
    free(ptr);
}

static void glz_usr_free(GlzEncoderUsrContext *usr, void *ptr)
{
    free(ptr);
}

/* Allocate more space for compressed buffer.
 * The pointer returned in io_ptr is garanteed to be aligned to 4 bytes.
 */
static int encoder_usr_more_space(EncoderData *enc_data, uint8_t **io_ptr)
{
    RedCompressBuf *buf;

    buf = compress_buf_new();
    enc_data->bufs_tail->send_next = buf;
    enc_data->bufs_tail = buf;
    buf->send_next = NULL;
    *io_ptr = buf->buf.bytes;
    return sizeof(buf->buf);
}

static int quic_usr_more_space(QuicUsrContext *usr, uint32_t **io_ptr, int rows_completed)
{
    EncoderData *usr_data = &(((QuicData *)usr)->data);
    return encoder_usr_more_space(usr_data, (uint8_t **)io_ptr) / sizeof(uint32_t);
}

static int lz_usr_more_space(LzUsrContext *usr, uint8_t **io_ptr)
{
    EncoderData *usr_data = &(((LzData *)usr)->data);
    return encoder_usr_more_space(usr_data, io_ptr);
}

static int glz_usr_more_space(GlzEncoderUsrContext *usr, uint8_t **io_ptr)
{
    EncoderData *usr_data = &(((GlzData *)usr)->data);
    return encoder_usr_more_space(usr_data, io_ptr);
}

static int jpeg_usr_more_space(JpegEncoderUsrContext *usr, uint8_t **io_ptr)
{
    EncoderData *usr_data = &(((JpegData *)usr)->data);
    return encoder_usr_more_space(usr_data, io_ptr);
}

#ifdef USE_LZ4
static int lz4_usr_more_space(Lz4EncoderUsrContext *usr, uint8_t **io_ptr)
{
    EncoderData *usr_data = &(((Lz4Data *)usr)->data);
    return encoder_usr_more_space(usr_data, io_ptr);
}
#endif

static int zlib_usr_more_space(ZlibEncoderUsrContext *usr, uint8_t **io_ptr)
{
    EncoderData *usr_data = &(((ZlibData *)usr)->data);
    return encoder_usr_more_space(usr_data, io_ptr);
}

static inline int encoder_usr_more_lines(EncoderData *enc_data, uint8_t **lines)
{
    struct SpiceChunk *chunk;

    if (enc_data->u.lines_data.reverse) {
        if (!(enc_data->u.lines_data.next >= 0)) {
            return 0;
        }
    } else {
        if (!(enc_data->u.lines_data.next < enc_data->u.lines_data.chunks->num_chunks)) {
            return 0;
        }
    }

    chunk = &enc_data->u.lines_data.chunks->chunk[enc_data->u.lines_data.next];
    if (chunk->len % enc_data->u.lines_data.stride) {
        return 0;
    }

    if (enc_data->u.lines_data.reverse) {
        enc_data->u.lines_data.next--;
        *lines = chunk->data + chunk->len - enc_data->u.lines_data.stride;
    } else {
        enc_data->u.lines_data.next++;
        *lines = chunk->data;
    }

    return chunk->len / enc_data->u.lines_data.stride;
}

static int quic_usr_more_lines(QuicUsrContext *usr, uint8_t **lines)
{
    EncoderData *usr_data = &(((QuicData *)usr)->data);
    return encoder_usr_more_lines(usr_data, lines);
}

static int lz_usr_more_lines(LzUsrContext *usr, uint8_t **lines)
{
    EncoderData *usr_data = &(((LzData *)usr)->data);
    return encoder_usr_more_lines(usr_data, lines);
}

static int glz_usr_more_lines(GlzEncoderUsrContext *usr, uint8_t **lines)
{
    EncoderData *usr_data = &(((GlzData *)usr)->data);
    return encoder_usr_more_lines(usr_data, lines);
}

static int jpeg_usr_more_lines(JpegEncoderUsrContext *usr, uint8_t **lines)
{
    EncoderData *usr_data = &(((JpegData *)usr)->data);
    return encoder_usr_more_lines(usr_data, lines);
}

#ifdef USE_LZ4
static int lz4_usr_more_lines(Lz4EncoderUsrContext *usr, uint8_t **lines)
{
    EncoderData *usr_data = &(((Lz4Data *)usr)->data);
    return encoder_usr_more_lines(usr_data, lines);
}
#endif

static int zlib_usr_more_input(ZlibEncoderUsrContext *usr, uint8_t** input)
{
    EncoderData *usr_data = &(((ZlibData *)usr)->data);
    int buf_size;

    if (!usr_data->u.compressed_data.next) {
        spice_assert(usr_data->u.compressed_data.size_left == 0);
        return 0;
    }

    *input = usr_data->u.compressed_data.next->buf.bytes;
    buf_size = MIN(sizeof(usr_data->u.compressed_data.next->buf),
                   usr_data->u.compressed_data.size_left);

    usr_data->u.compressed_data.next = usr_data->u.compressed_data.next->send_next;
    usr_data->u.compressed_data.size_left -= buf_size;
    return buf_size;
}

static void glz_usr_free_image(GlzEncoderUsrContext *usr, GlzUsrImageContext *image)
{
    GlzData *lz_data = (GlzData *)usr;
    GlzDrawableInstanceItem *glz_drawable_instance = (GlzDrawableInstanceItem *)image;
    DisplayChannelClient *drawable_cc = glz_drawable_instance->red_glz_drawable->dcc;
    DisplayChannelClient *this_cc = SPICE_CONTAINEROF(lz_data, DisplayChannelClient, glz_data);
    if (this_cc == drawable_cc) {
        dcc_free_glz_drawable_instance(drawable_cc, glz_drawable_instance);
    } else {
        /* The glz dictionary is shared between all DisplayChannelClient
         * instances that belong to the same client, and glz_usr_free_image
         * can be called by the dictionary code
         * (glz_dictionary_window_remove_head). Thus this function can be
         * called from any DisplayChannelClient thread, hence the need for
         * this check.
         */
        pthread_mutex_lock(&drawable_cc->glz_drawables_inst_to_free_lock);
        ring_add_before(&glz_drawable_instance->free_link,
                        &drawable_cc->glz_drawables_inst_to_free);
        pthread_mutex_unlock(&drawable_cc->glz_drawables_inst_to_free_lock);
    }
}

static inline void red_init_quic(RedWorker *worker)
{
    worker->quic_data.usr.error = quic_usr_error;
    worker->quic_data.usr.warn = quic_usr_warn;
    worker->quic_data.usr.info = quic_usr_warn;
    worker->quic_data.usr.malloc = quic_usr_malloc;
    worker->quic_data.usr.free = quic_usr_free;
    worker->quic_data.usr.more_space = quic_usr_more_space;
    worker->quic_data.usr.more_lines = quic_usr_more_lines;

    worker->quic = quic_create(&worker->quic_data.usr);

    if (!worker->quic) {
        spice_critical("create quic failed");
    }
}

static inline void red_init_lz(RedWorker *worker)
{
    worker->lz_data.usr.error = lz_usr_error;
    worker->lz_data.usr.warn = lz_usr_warn;
    worker->lz_data.usr.info = lz_usr_warn;
    worker->lz_data.usr.malloc = lz_usr_malloc;
    worker->lz_data.usr.free = lz_usr_free;
    worker->lz_data.usr.more_space = lz_usr_more_space;
    worker->lz_data.usr.more_lines = lz_usr_more_lines;

    worker->lz = lz_create(&worker->lz_data.usr);

    if (!worker->lz) {
        spice_critical("create lz failed");
    }
}

/* TODO: split off to DisplayChannel? avoid just copying those cb pointers */
static inline void red_display_init_glz_data(DisplayChannelClient *dcc)
{
    dcc->glz_data.usr.error = glz_usr_error;
    dcc->glz_data.usr.warn = glz_usr_warn;
    dcc->glz_data.usr.info = glz_usr_warn;
    dcc->glz_data.usr.malloc = glz_usr_malloc;
    dcc->glz_data.usr.free = glz_usr_free;
    dcc->glz_data.usr.more_space = glz_usr_more_space;
    dcc->glz_data.usr.more_lines = glz_usr_more_lines;
    dcc->glz_data.usr.free_image = glz_usr_free_image;
}

static inline void red_init_jpeg(RedWorker *worker)
{
    worker->jpeg_data.usr.more_space = jpeg_usr_more_space;
    worker->jpeg_data.usr.more_lines = jpeg_usr_more_lines;

    worker->jpeg = jpeg_encoder_create(&worker->jpeg_data.usr);

    if (!worker->jpeg) {
        spice_critical("create jpeg encoder failed");
    }
}

#ifdef USE_LZ4
static inline void red_init_lz4(RedWorker *worker)
{
    worker->lz4_data.usr.more_space = lz4_usr_more_space;
    worker->lz4_data.usr.more_lines = lz4_usr_more_lines;

    worker->lz4 = lz4_encoder_create(&worker->lz4_data.usr);

    if (!worker->lz4) {
        spice_critical("create lz4 encoder failed");
    }
}
#endif

static inline void red_init_zlib(RedWorker *worker)
{
    worker->zlib_data.usr.more_space = zlib_usr_more_space;
    worker->zlib_data.usr.more_input = zlib_usr_more_input;

    worker->zlib = zlib_encoder_create(&worker->zlib_data.usr, ZLIB_DEFAULT_COMPRESSION_LEVEL);

    if (!worker->zlib) {
        spice_critical("create zlib encoder failed");
    }
}

typedef struct compress_send_data_t {
    void*    comp_buf;
    uint32_t comp_buf_size;
    SpicePalette *lzplt_palette;
    int is_lossy;
} compress_send_data_t;

static inline int red_glz_compress_image(DisplayChannelClient *dcc,
                                         SpiceImage *dest, SpiceBitmap *src, Drawable *drawable,
                                         compress_send_data_t* o_comp_data)
{
    DisplayChannel *display_channel = DCC_TO_DC(dcc);
    RedWorker *worker = display_channel->common.worker;
#ifdef COMPRESS_STAT
    stat_time_t start_time = stat_now(display_channel->zlib_glz_stat.clock);
#endif
    spice_assert(bitmap_fmt_is_rgb(src->format));
    GlzData *glz_data = &dcc->glz_data;
    ZlibData *zlib_data;
    LzImageType type = MAP_BITMAP_FMT_TO_LZ_IMAGE_TYPE[src->format];
    RedGlzDrawable *glz_drawable;
    GlzDrawableInstanceItem *glz_drawable_instance;
    int glz_size;
    int zlib_size;

    glz_data->data.bufs_tail = compress_buf_new();
    glz_data->data.bufs_head = glz_data->data.bufs_tail;

    if (!glz_data->data.bufs_head) {
        return FALSE;
    }

    glz_data->data.bufs_head->send_next = NULL;
    glz_data->data.dcc = dcc;

    glz_drawable = red_display_get_glz_drawable(dcc, drawable);
    glz_drawable_instance = red_display_add_glz_drawable_instance(glz_drawable);

    glz_data->data.u.lines_data.chunks = src->data;
    glz_data->data.u.lines_data.stride = src->stride;
    glz_data->data.u.lines_data.next = 0;
    glz_data->data.u.lines_data.reverse = 0;
    glz_data->usr.more_lines = glz_usr_more_lines;

    glz_size = glz_encode(dcc->glz, type, src->x, src->y,
                          (src->flags & SPICE_BITMAP_FLAGS_TOP_DOWN), NULL, 0,
                          src->stride, glz_data->data.bufs_head->buf.bytes,
                          sizeof(glz_data->data.bufs_head->buf),
                          glz_drawable_instance,
                          &glz_drawable_instance->glz_instance);

    stat_compress_add(&display_channel->glz_stat, start_time, src->stride * src->y, glz_size);

    if (!display_channel->enable_zlib_glz_wrap || (glz_size < MIN_GLZ_SIZE_FOR_ZLIB)) {
        goto glz;
    }
#ifdef COMPRESS_STAT
    start_time = stat_now(display_channel->zlib_glz_stat.clock);
#endif
    zlib_data = &worker->zlib_data;

    zlib_data->data.bufs_tail = compress_buf_new();
    zlib_data->data.bufs_head = zlib_data->data.bufs_tail;

    if (!zlib_data->data.bufs_head) {
        spice_warning("failed to allocate zlib compress buffer");
        goto glz;
    }

    zlib_data->data.bufs_head->send_next = NULL;
    zlib_data->data.dcc = dcc;

    zlib_data->data.u.compressed_data.next = glz_data->data.bufs_head;
    zlib_data->data.u.compressed_data.size_left = glz_size;

    zlib_size = zlib_encode(worker->zlib, display_channel->zlib_level,
                            glz_size, zlib_data->data.bufs_head->buf.bytes,
                            sizeof(zlib_data->data.bufs_head->buf));

    // the compressed buffer is bigger than the original data
    if (zlib_size >= glz_size) {
        while (zlib_data->data.bufs_head) {
            RedCompressBuf *buf = zlib_data->data.bufs_head;
            zlib_data->data.bufs_head = buf->send_next;
            compress_buf_free(buf);
        }
        goto glz;
    }

    dest->descriptor.type = SPICE_IMAGE_TYPE_ZLIB_GLZ_RGB;
    dest->u.zlib_glz.glz_data_size = glz_size;
    dest->u.zlib_glz.data_size = zlib_size;

    o_comp_data->comp_buf = zlib_data->data.bufs_head;
    o_comp_data->comp_buf_size = zlib_size;

    stat_compress_add(&display_channel->zlib_glz_stat, start_time, glz_size, zlib_size);
    return TRUE;
glz:
    dest->descriptor.type = SPICE_IMAGE_TYPE_GLZ_RGB;
    dest->u.lz_rgb.data_size = glz_size;

    o_comp_data->comp_buf = glz_data->data.bufs_head;
    o_comp_data->comp_buf_size = glz_size;

    return TRUE;
}

static inline int red_lz_compress_image(DisplayChannelClient *dcc,
                                        SpiceImage *dest, SpiceBitmap *src,
                                        compress_send_data_t* o_comp_data, uint32_t group_id)
{
    DisplayChannel *display_channel = DCC_TO_DC(dcc);
    RedWorker *worker = display_channel->common.worker;
    LzData *lz_data = &worker->lz_data;
    LzContext *lz = worker->lz;
    LzImageType type = MAP_BITMAP_FMT_TO_LZ_IMAGE_TYPE[src->format];
    int size;            // size of the compressed data

#ifdef COMPRESS_STAT
    stat_time_t start_time = stat_now(DCC_TO_DC(dcc)->lz_stat.clock);
#endif

    lz_data->data.bufs_tail = compress_buf_new();
    lz_data->data.bufs_head = lz_data->data.bufs_tail;

    if (!lz_data->data.bufs_head) {
        return FALSE;
    }

    lz_data->data.bufs_head->send_next = NULL;
    lz_data->data.dcc = dcc;

    if (setjmp(lz_data->data.jmp_env)) {
        while (lz_data->data.bufs_head) {
            RedCompressBuf *buf = lz_data->data.bufs_head;
            lz_data->data.bufs_head = buf->send_next;
            compress_buf_free(buf);
        }
        return FALSE;
    }

    lz_data->data.u.lines_data.chunks = src->data;
    lz_data->data.u.lines_data.stride = src->stride;
    lz_data->data.u.lines_data.next = 0;
    lz_data->data.u.lines_data.reverse = 0;
    lz_data->usr.more_lines = lz_usr_more_lines;

    size = lz_encode(lz, type, src->x, src->y,
                     !!(src->flags & SPICE_BITMAP_FLAGS_TOP_DOWN),
                     NULL, 0, src->stride,
                     lz_data->data.bufs_head->buf.bytes,
                     sizeof(lz_data->data.bufs_head->buf));

    // the compressed buffer is bigger than the original data
    if (size > (src->y * src->stride)) {
        longjmp(lz_data->data.jmp_env, 1);
    }

    if (bitmap_fmt_is_rgb(src->format)) {
        dest->descriptor.type = SPICE_IMAGE_TYPE_LZ_RGB;
        dest->u.lz_rgb.data_size = size;

        o_comp_data->comp_buf = lz_data->data.bufs_head;
        o_comp_data->comp_buf_size = size;
    } else {
        /* masks are 1BIT bitmaps without palettes, but they are not compressed
         * (see fill_mask) */
        spice_assert(src->palette);
        dest->descriptor.type = SPICE_IMAGE_TYPE_LZ_PLT;
        dest->u.lz_plt.data_size = size;
        dest->u.lz_plt.flags = src->flags & SPICE_BITMAP_FLAGS_TOP_DOWN;
        dest->u.lz_plt.palette = src->palette;
        dest->u.lz_plt.palette_id = src->palette->unique;
        o_comp_data->comp_buf = lz_data->data.bufs_head;
        o_comp_data->comp_buf_size = size;

        fill_palette(dcc, dest->u.lz_plt.palette, &(dest->u.lz_plt.flags));
        o_comp_data->lzplt_palette = dest->u.lz_plt.palette;
    }

    stat_compress_add(&DCC_TO_DC(dcc)->lz_stat, start_time, src->stride * src->y,
                      o_comp_data->comp_buf_size);
    return TRUE;
}

static int red_jpeg_compress_image(DisplayChannelClient *dcc, SpiceImage *dest,
                                   SpiceBitmap *src, compress_send_data_t* o_comp_data,
                                   uint32_t group_id)
{
    DisplayChannel *display_channel = DCC_TO_DC(dcc);
    RedWorker *worker = display_channel->common.worker;
    JpegData *jpeg_data = &worker->jpeg_data;
    LzData *lz_data = &worker->lz_data;
    JpegEncoderContext *jpeg = worker->jpeg;
    LzContext *lz = worker->lz;
    volatile JpegEncoderImageType jpeg_in_type;
    int jpeg_size = 0;
    volatile int has_alpha = FALSE;
    int alpha_lz_size = 0;
    int comp_head_filled;
    int comp_head_left;
    int stride;
    uint8_t *lz_out_start_byte;

#ifdef COMPRESS_STAT
    stat_time_t start_time = stat_now(DCC_TO_DC(dcc)->jpeg_alpha_stat.clock);
#endif
    switch (src->format) {
    case SPICE_BITMAP_FMT_16BIT:
        jpeg_in_type = JPEG_IMAGE_TYPE_RGB16;
        break;
    case SPICE_BITMAP_FMT_24BIT:
        jpeg_in_type = JPEG_IMAGE_TYPE_BGR24;
        break;
    case SPICE_BITMAP_FMT_32BIT:
        jpeg_in_type = JPEG_IMAGE_TYPE_BGRX32;
        break;
    case SPICE_BITMAP_FMT_RGBA:
        jpeg_in_type = JPEG_IMAGE_TYPE_BGRX32;
        has_alpha = TRUE;
        break;
    default:
        return FALSE;
    }

    jpeg_data->data.bufs_tail = compress_buf_new();
    jpeg_data->data.bufs_head = jpeg_data->data.bufs_tail;

    if (!jpeg_data->data.bufs_head) {
        spice_warning("failed to allocate compress buffer");
        return FALSE;
    }

    jpeg_data->data.bufs_head->send_next = NULL;
    jpeg_data->data.dcc = dcc;

    if (setjmp(jpeg_data->data.jmp_env)) {
        while (jpeg_data->data.bufs_head) {
            RedCompressBuf *buf = jpeg_data->data.bufs_head;
            jpeg_data->data.bufs_head = buf->send_next;
            compress_buf_free(buf);
        }
        return FALSE;
    }

    if (src->data->flags & SPICE_CHUNKS_FLAGS_UNSTABLE) {
        spice_chunks_linearize(src->data);
    }

    jpeg_data->data.u.lines_data.chunks = src->data;
    jpeg_data->data.u.lines_data.stride = src->stride;
    jpeg_data->usr.more_lines = jpeg_usr_more_lines;
    if ((src->flags & SPICE_BITMAP_FLAGS_TOP_DOWN)) {
        jpeg_data->data.u.lines_data.next = 0;
        jpeg_data->data.u.lines_data.reverse = 0;
        stride = src->stride;
    } else {
        jpeg_data->data.u.lines_data.next = src->data->num_chunks - 1;
        jpeg_data->data.u.lines_data.reverse = 1;
        stride = -src->stride;
    }
    jpeg_size = jpeg_encode(jpeg, display_channel->jpeg_quality, jpeg_in_type,
                            src->x, src->y, NULL,
                            0, stride, jpeg_data->data.bufs_head->buf.bytes,
                            sizeof(jpeg_data->data.bufs_head->buf));

    // the compressed buffer is bigger than the original data
    if (jpeg_size > (src->y * src->stride)) {
        longjmp(jpeg_data->data.jmp_env, 1);
    }

    if (!has_alpha) {
        dest->descriptor.type = SPICE_IMAGE_TYPE_JPEG;
        dest->u.jpeg.data_size = jpeg_size;

        o_comp_data->comp_buf = jpeg_data->data.bufs_head;
        o_comp_data->comp_buf_size = jpeg_size;
        o_comp_data->is_lossy = TRUE;

        stat_compress_add(&DCC_TO_DC(dcc)->jpeg_stat, start_time, src->stride * src->y,
                          o_comp_data->comp_buf_size);
        return TRUE;
    }

    lz_data->data.bufs_head = jpeg_data->data.bufs_tail;
    lz_data->data.bufs_tail = lz_data->data.bufs_head;

    comp_head_filled = jpeg_size % sizeof(lz_data->data.bufs_head->buf);
    comp_head_left = sizeof(lz_data->data.bufs_head->buf) - comp_head_filled;
    lz_out_start_byte = lz_data->data.bufs_head->buf.bytes + comp_head_filled;

    lz_data->data.dcc = dcc;

    lz_data->data.u.lines_data.chunks = src->data;
    lz_data->data.u.lines_data.stride = src->stride;
    lz_data->data.u.lines_data.next = 0;
    lz_data->data.u.lines_data.reverse = 0;
    lz_data->usr.more_lines = lz_usr_more_lines;

    alpha_lz_size = lz_encode(lz, LZ_IMAGE_TYPE_XXXA, src->x, src->y,
                               !!(src->flags & SPICE_BITMAP_FLAGS_TOP_DOWN),
                               NULL, 0, src->stride,
                               lz_out_start_byte,
                               comp_head_left);

    // the compressed buffer is bigger than the original data
    if ((jpeg_size + alpha_lz_size) > (src->y * src->stride)) {
        longjmp(jpeg_data->data.jmp_env, 1);
    }

    dest->descriptor.type = SPICE_IMAGE_TYPE_JPEG_ALPHA;
    dest->u.jpeg_alpha.flags = 0;
    if (src->flags & SPICE_BITMAP_FLAGS_TOP_DOWN) {
        dest->u.jpeg_alpha.flags |= SPICE_JPEG_ALPHA_FLAGS_TOP_DOWN;
    }

    dest->u.jpeg_alpha.jpeg_size = jpeg_size;
    dest->u.jpeg_alpha.data_size = jpeg_size + alpha_lz_size;

    o_comp_data->comp_buf = jpeg_data->data.bufs_head;
    o_comp_data->comp_buf_size = jpeg_size + alpha_lz_size;
    o_comp_data->is_lossy = TRUE;
    stat_compress_add(&DCC_TO_DC(dcc)->jpeg_alpha_stat, start_time, src->stride * src->y,
                      o_comp_data->comp_buf_size);
    return TRUE;
}

#ifdef USE_LZ4
static int red_lz4_compress_image(DisplayChannelClient *dcc, SpiceImage *dest,
                                  SpiceBitmap *src, compress_send_data_t* o_comp_data,
                                  uint32_t group_id)
{
    DisplayChannel *display_channel = DCC_TO_DC(dcc);
    RedWorker *worker = display_channel->common.worker;
    Lz4Data *lz4_data = &worker->lz4_data;
    Lz4EncoderContext *lz4 = worker->lz4;
    int lz4_size = 0;

#ifdef COMPRESS_STAT
    stat_time_t start_time = stat_now(worker->clockid);
#endif

    lz4_data->data.bufs_tail = compress_buf_new();
    lz4_data->data.bufs_head = lz4_data->data.bufs_tail;

    if (!lz4_data->data.bufs_head) {
        spice_warning("failed to allocate compress buffer");
        return FALSE;
    }

    lz4_data->data.bufs_head->send_next = NULL;
    lz4_data->data.dcc = dcc;

    if (setjmp(lz4_data->data.jmp_env)) {
        while (lz4_data->data.bufs_head) {
            RedCompressBuf *buf = lz4_data->data.bufs_head;
            lz4_data->data.bufs_head = buf->send_next;
            compress_buf_free(buf);
        }
        return FALSE;
    }

    if (src->data->flags & SPICE_CHUNKS_FLAGS_UNSTABLE) {
        spice_chunks_linearize(src->data);
    }

    lz4_data->data.u.lines_data.chunks = src->data;
    lz4_data->data.u.lines_data.stride = src->stride;
    lz4_data->data.u.lines_data.next = 0;
    lz4_data->data.u.lines_data.reverse = 0;
    lz4_data->usr.more_lines = lz4_usr_more_lines;

    lz4_size = lz4_encode(lz4, src->y, src->stride, lz4_data->data.bufs_head->buf.bytes,
                          sizeof(lz4_data->data.bufs_head->buf),
                          src->flags & SPICE_BITMAP_FLAGS_TOP_DOWN, src->format);

    // the compressed buffer is bigger than the original data
    if (lz4_size > (src->y * src->stride)) {
        longjmp(lz4_data->data.jmp_env, 1);
    }

    dest->descriptor.type = SPICE_IMAGE_TYPE_LZ4;
    dest->u.lz4.data_size = lz4_size;

    o_comp_data->comp_buf = lz4_data->data.bufs_head;
    o_comp_data->comp_buf_size = lz4_size;

    stat_compress_add(&display_channel->lz4_stat, start_time, src->stride * src->y,
                      o_comp_data->comp_buf_size);
    return TRUE;
}
#endif

static inline int red_quic_compress_image(DisplayChannelClient *dcc, SpiceImage *dest,
                                          SpiceBitmap *src, compress_send_data_t* o_comp_data,
                                          uint32_t group_id)
{
    DisplayChannel *display_channel = DCC_TO_DC(dcc);
    RedWorker *worker = display_channel->common.worker;
    QuicData *quic_data = &worker->quic_data;
    QuicContext *quic = worker->quic;
    volatile QuicImageType type;
    int size, stride;

#ifdef COMPRESS_STAT
    stat_time_t start_time = stat_now(DCC_TO_DC(dcc)->quic_stat.clock);
#endif

    switch (src->format) {
    case SPICE_BITMAP_FMT_32BIT:
        type = QUIC_IMAGE_TYPE_RGB32;
        break;
    case SPICE_BITMAP_FMT_RGBA:
        type = QUIC_IMAGE_TYPE_RGBA;
        break;
    case SPICE_BITMAP_FMT_16BIT:
        type = QUIC_IMAGE_TYPE_RGB16;
        break;
    case SPICE_BITMAP_FMT_24BIT:
        type = QUIC_IMAGE_TYPE_RGB24;
        break;
    default:
        return FALSE;
    }

    quic_data->data.bufs_tail = compress_buf_new();
    quic_data->data.bufs_head = quic_data->data.bufs_tail;
    quic_data->data.bufs_head->send_next = NULL;
    quic_data->data.dcc = dcc;

    if (setjmp(quic_data->data.jmp_env)) {
        while (quic_data->data.bufs_head) {
            RedCompressBuf *buf = quic_data->data.bufs_head;
            quic_data->data.bufs_head = buf->send_next;
            compress_buf_free(buf);
        }
        return FALSE;
    }

    if (src->data->flags & SPICE_CHUNKS_FLAGS_UNSTABLE) {
        spice_chunks_linearize(src->data);
    }

    quic_data->data.u.lines_data.chunks = src->data;
    quic_data->data.u.lines_data.stride = src->stride;
    quic_data->usr.more_lines = quic_usr_more_lines;
    if ((src->flags & SPICE_BITMAP_FLAGS_TOP_DOWN)) {
        quic_data->data.u.lines_data.next = 0;
        quic_data->data.u.lines_data.reverse = 0;
        stride = src->stride;
    } else {
        quic_data->data.u.lines_data.next = src->data->num_chunks - 1;
        quic_data->data.u.lines_data.reverse = 1;
        stride = -src->stride;
    }
    size = quic_encode(quic, type, src->x, src->y, NULL, 0, stride,
                       quic_data->data.bufs_head->buf.words,
                       G_N_ELEMENTS(quic_data->data.bufs_head->buf.words));

    // the compressed buffer is bigger than the original data
    if ((size << 2) > (src->y * src->stride)) {
        longjmp(quic_data->data.jmp_env, 1);
    }

    dest->descriptor.type = SPICE_IMAGE_TYPE_QUIC;
    dest->u.quic.data_size = size << 2;

    o_comp_data->comp_buf = quic_data->data.bufs_head;
    o_comp_data->comp_buf_size = size << 2;

    stat_compress_add(&DCC_TO_DC(dcc)->quic_stat, start_time, src->stride * src->y,
                      o_comp_data->comp_buf_size);
    return TRUE;
}

#define MIN_SIZE_TO_COMPRESS 54
#define MIN_DIMENSION_TO_QUIC 3
static inline int red_compress_image(DisplayChannelClient *dcc,
                                     SpiceImage *dest, SpiceBitmap *src, Drawable *drawable,
                                     int can_lossy,
                                     compress_send_data_t* o_comp_data)
{
    DisplayChannel *display_channel = DCC_TO_DC(dcc);
    SpiceImageCompression image_compression = dcc->image_compression;
    int quic_compress = FALSE;

    if ((image_compression == SPICE_IMAGE_COMPRESSION_OFF) ||
        ((src->y * src->stride) < MIN_SIZE_TO_COMPRESS)) { // TODO: change the size cond
        return FALSE;
    } else if (image_compression == SPICE_IMAGE_COMPRESSION_QUIC) {
        if (bitmap_fmt_is_plt(src->format)) {
            return FALSE;
        } else {
            quic_compress = TRUE;
        }
    } else {
        /*
            lz doesn't handle (1) bitmaps with strides that are larger than the width
            of the image in bytes (2) unstable bitmaps
        */
        if (bitmap_has_extra_stride(src) || (src->data->flags & SPICE_CHUNKS_FLAGS_UNSTABLE)) {
            if ((image_compression == SPICE_IMAGE_COMPRESSION_LZ) ||
                (image_compression == SPICE_IMAGE_COMPRESSION_GLZ) ||
                (image_compression == SPICE_IMAGE_COMPRESSION_LZ4) ||
                bitmap_fmt_is_plt(src->format)) {
                return FALSE;
            } else {
                quic_compress = TRUE;
            }
        } else {
            if ((image_compression == SPICE_IMAGE_COMPRESSION_AUTO_LZ) ||
                (image_compression == SPICE_IMAGE_COMPRESSION_AUTO_GLZ)) {
                if ((src->x < MIN_DIMENSION_TO_QUIC) || (src->y < MIN_DIMENSION_TO_QUIC)) {
                    quic_compress = FALSE;
                } else {
                    if (drawable->copy_bitmap_graduality == BITMAP_GRADUAL_INVALID) {
                        quic_compress = bitmap_fmt_has_graduality(src->format) &&
                            bitmap_get_graduality_level(src) == BITMAP_GRADUAL_HIGH;
                    } else {
                        quic_compress = (drawable->copy_bitmap_graduality == BITMAP_GRADUAL_HIGH);
                    }
                }
            } else {
                quic_compress = FALSE;
            }
        }
    }

    if (quic_compress) {
#ifdef COMPRESS_DEBUG
        spice_info("QUIC compress");
#endif
        // if bitmaps is picture-like, compress it using jpeg
        if (can_lossy && display_channel->enable_jpeg &&
            ((image_compression == SPICE_IMAGE_COMPRESSION_AUTO_LZ) ||
            (image_compression == SPICE_IMAGE_COMPRESSION_AUTO_GLZ))) {
            // if we use lz for alpha, the stride can't be extra
            if (src->format != SPICE_BITMAP_FMT_RGBA || !bitmap_has_extra_stride(src)) {
                return red_jpeg_compress_image(dcc, dest,
                                               src, o_comp_data, drawable->group_id);
            }
        }
        return red_quic_compress_image(dcc, dest,
                                       src, o_comp_data, drawable->group_id);
    } else {
        int glz;
        int ret;
        if ((image_compression == SPICE_IMAGE_COMPRESSION_AUTO_GLZ) ||
            (image_compression == SPICE_IMAGE_COMPRESSION_GLZ)) {
            glz = bitmap_fmt_has_graduality(src->format) && (
                    (src->x * src->y) < glz_enc_dictionary_get_size(
                        dcc->glz_dict->dict));
        } else if ((image_compression == SPICE_IMAGE_COMPRESSION_AUTO_LZ) ||
                   (image_compression == SPICE_IMAGE_COMPRESSION_LZ) ||
                   (image_compression == SPICE_IMAGE_COMPRESSION_LZ4)) {
            glz = FALSE;
        } else {
            spice_error("invalid image compression type %u", image_compression);
            return FALSE;
        }

        if (glz) {
            /* using the global dictionary only if it is not frozen */
            pthread_rwlock_rdlock(&dcc->glz_dict->encode_lock);
            if (!dcc->glz_dict->migrate_freeze) {
                ret = red_glz_compress_image(dcc,
                                             dest, src,
                                             drawable, o_comp_data);
            } else {
                glz = FALSE;
            }
            pthread_rwlock_unlock(&dcc->glz_dict->encode_lock);
        }

        if (!glz) {
#ifdef USE_LZ4
            if (image_compression == SPICE_IMAGE_COMPRESSION_LZ4 &&
                bitmap_fmt_is_rgb(src->format) &&
                red_channel_client_test_remote_cap(&dcc->common.base,
                        SPICE_DISPLAY_CAP_LZ4_COMPRESSION)) {
                ret = red_lz4_compress_image(dcc, dest, src, o_comp_data,
                                             drawable->group_id);
            } else
#endif
                ret = red_lz_compress_image(dcc, dest, src, o_comp_data,
                                            drawable->group_id);
#ifdef COMPRESS_DEBUG
            spice_info("LZ LOCAL compress");
#endif
        }
#ifdef COMPRESS_DEBUG
        else {
            spice_info("LZ global compress fmt=%d", src->format);
        }
#endif
        return ret;
    }
}

int dcc_pixmap_cache_unlocked_add(DisplayChannelClient *dcc, uint64_t id, uint32_t size, int lossy)
{
    PixmapCache *cache = dcc->pixmap_cache;
    NewCacheItem *item;
    uint64_t serial;
    int key;

    spice_assert(size > 0);

    item = spice_new(NewCacheItem, 1);
    serial = red_channel_client_get_message_serial(RED_CHANNEL_CLIENT(dcc));

    if (cache->generation != dcc->pixmap_cache_generation) {
        if (!dcc->pending_pixmaps_sync) {
            red_channel_client_pipe_add_type(
                                             RED_CHANNEL_CLIENT(dcc), PIPE_ITEM_TYPE_PIXMAP_SYNC);
            dcc->pending_pixmaps_sync = TRUE;
        }
        free(item);
        return FALSE;
    }

    cache->available -= size;
    while (cache->available < 0) {
        NewCacheItem *tail;
        NewCacheItem **now;

        if (!(tail = (NewCacheItem *)ring_get_tail(&cache->lru)) ||
                                                   tail->sync[dcc->common.id] == serial) {
            cache->available += size;
            free(item);
            return FALSE;
        }

        now = &cache->hash_table[BITS_CACHE_HASH_KEY(tail->id)];
        for (;;) {
            spice_assert(*now);
            if (*now == tail) {
                *now = tail->next;
                break;
            }
            now = &(*now)->next;
        }
        ring_remove(&tail->lru_link);
        cache->items--;
        cache->available += tail->size;
        cache->sync[dcc->common.id] = serial;
        display_channel_push_release(dcc, SPICE_RES_TYPE_PIXMAP, tail->id, tail->sync);
        free(tail);
    }
    ++cache->items;
    item->next = cache->hash_table[(key = BITS_CACHE_HASH_KEY(id))];
    cache->hash_table[key] = item;
    ring_item_init(&item->lru_link);
    ring_add(&cache->lru, &item->lru_link);
    item->id = id;
    item->size = size;
    item->lossy = lossy;
    memset(item->sync, 0, sizeof(item->sync));
    item->sync[dcc->common.id] = serial;
    cache->sync[dcc->common.id] = serial;
    return TRUE;
}

static inline void red_display_add_image_to_pixmap_cache(RedChannelClient *rcc,
                                                         SpiceImage *image, SpiceImage *io_image,
                                                         int is_lossy)
{
    DisplayChannel *display_channel = SPICE_CONTAINEROF(rcc->channel, DisplayChannel, common.base);
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);

    if ((image->descriptor.flags & SPICE_IMAGE_FLAGS_CACHE_ME)) {
        spice_assert(image->descriptor.width * image->descriptor.height > 0);
        if (!(io_image->descriptor.flags & SPICE_IMAGE_FLAGS_CACHE_REPLACE_ME)) {
            if (dcc_pixmap_cache_unlocked_add(dcc, image->descriptor.id,
                                              image->descriptor.width * image->descriptor.height,
                                              is_lossy)) {
                io_image->descriptor.flags |= SPICE_IMAGE_FLAGS_CACHE_ME;
                dcc->send_data.pixmap_cache_items[dcc->send_data.num_pixmap_cache_items++] =
                                                                               image->descriptor.id;
                stat_inc_counter(display_channel->add_to_cache_counter, 1);
            }
        }
    }

    if (!(io_image->descriptor.flags & SPICE_IMAGE_FLAGS_CACHE_ME)) {
        stat_inc_counter(display_channel->non_cache_counter, 1);
    }
}

static int dcc_pixmap_cache_unlocked_hit(DisplayChannelClient *dcc, uint64_t id, int *lossy)
{
    PixmapCache *cache = dcc->pixmap_cache;
    NewCacheItem *item;
    uint64_t serial;

    serial = red_channel_client_get_message_serial(RED_CHANNEL_CLIENT(dcc));
    item = cache->hash_table[BITS_CACHE_HASH_KEY(id)];

    while (item) {
        if (item->id == id) {
            ring_remove(&item->lru_link);
            ring_add(&cache->lru, &item->lru_link);
            spice_assert(dcc->common.id < MAX_CACHE_CLIENTS);
            item->sync[dcc->common.id] = serial;
            cache->sync[dcc->common.id] = serial;
            *lossy = item->lossy;
            break;
        }
        item = item->next;
    }

    return !!item;
}

static int dcc_pixmap_cache_hit(DisplayChannelClient *dcc, uint64_t id, int *lossy)
{
    int hit;
    PixmapCache *cache = dcc->pixmap_cache;

    pthread_mutex_lock(&cache->lock);
    hit = dcc_pixmap_cache_unlocked_hit(dcc, id, lossy);
    pthread_mutex_unlock(&cache->lock);
    return hit;
}


typedef enum {
    FILL_BITS_TYPE_INVALID,
    FILL_BITS_TYPE_CACHE,
    FILL_BITS_TYPE_SURFACE,
    FILL_BITS_TYPE_COMPRESS_LOSSLESS,
    FILL_BITS_TYPE_COMPRESS_LOSSY,
    FILL_BITS_TYPE_BITMAP,
} FillBitsType;

/* if the number of times fill_bits can be called per one qxl_drawable increases -
   MAX_LZ_DRAWABLE_INSTANCES must be increased as well */
static FillBitsType fill_bits(DisplayChannelClient *dcc, SpiceMarshaller *m,
                              SpiceImage *simage, Drawable *drawable, int can_lossy)
{
    RedChannelClient *rcc = RED_CHANNEL_CLIENT(dcc);
    DisplayChannel *display = DCC_TO_DC(dcc);
    SpiceImage image;
    compress_send_data_t comp_send_data = {0};
    SpiceMarshaller *bitmap_palette_out, *lzplt_palette_out;

    if (simage == NULL) {
        spice_assert(drawable->red_drawable->self_bitmap_image);
        simage = drawable->red_drawable->self_bitmap_image;
    }

    image.descriptor = simage->descriptor;
    image.descriptor.flags = 0;
    if (simage->descriptor.flags & SPICE_IMAGE_FLAGS_HIGH_BITS_SET) {
        image.descriptor.flags = SPICE_IMAGE_FLAGS_HIGH_BITS_SET;
    }
    pthread_mutex_lock(&dcc->pixmap_cache->lock);

    if ((simage->descriptor.flags & SPICE_IMAGE_FLAGS_CACHE_ME)) {
        int lossy_cache_item;
        if (dcc_pixmap_cache_unlocked_hit(dcc, image.descriptor.id, &lossy_cache_item)) {
            dcc->send_data.pixmap_cache_items[dcc->send_data.num_pixmap_cache_items++] =
                                                                               image.descriptor.id;
            if (can_lossy || !lossy_cache_item) {
                if (!display->enable_jpeg || lossy_cache_item) {
                    image.descriptor.type = SPICE_IMAGE_TYPE_FROM_CACHE;
                } else {
                    // making sure, in multiple monitor scenario, that lossy items that
                    // should have been replaced with lossless data by one display channel,
                    // will be retrieved as lossless by another display channel.
                    image.descriptor.type = SPICE_IMAGE_TYPE_FROM_CACHE_LOSSLESS;
                }
                spice_marshall_Image(m, &image,
                                     &bitmap_palette_out, &lzplt_palette_out);
                spice_assert(bitmap_palette_out == NULL);
                spice_assert(lzplt_palette_out == NULL);
                stat_inc_counter(display->cache_hits_counter, 1);
                pthread_mutex_unlock(&dcc->pixmap_cache->lock);
                return FILL_BITS_TYPE_CACHE;
            } else {
                pixmap_cache_unlocked_set_lossy(dcc->pixmap_cache, simage->descriptor.id,
                                                FALSE);
                image.descriptor.flags |= SPICE_IMAGE_FLAGS_CACHE_REPLACE_ME;
            }
        }
    }

    switch (simage->descriptor.type) {
    case SPICE_IMAGE_TYPE_SURFACE: {
        int surface_id;
        RedSurface *surface;

        surface_id = simage->u.surface.surface_id;
        if (!validate_surface(display, surface_id)) {
            rendering_incorrect("SPICE_IMAGE_TYPE_SURFACE");
            pthread_mutex_unlock(&dcc->pixmap_cache->lock);
            return FILL_BITS_TYPE_SURFACE;
        }

        surface = &display->surfaces[surface_id];
        image.descriptor.type = SPICE_IMAGE_TYPE_SURFACE;
        image.descriptor.flags = 0;
        image.descriptor.width = surface->context.width;
        image.descriptor.height = surface->context.height;

        image.u.surface.surface_id = surface_id;
        spice_marshall_Image(m, &image,
                             &bitmap_palette_out, &lzplt_palette_out);
        spice_assert(bitmap_palette_out == NULL);
        spice_assert(lzplt_palette_out == NULL);
        pthread_mutex_unlock(&dcc->pixmap_cache->lock);
        return FILL_BITS_TYPE_SURFACE;
    }
    case SPICE_IMAGE_TYPE_BITMAP: {
        SpiceBitmap *bitmap = &image.u.bitmap;
#ifdef DUMP_BITMAP
        dump_bitmap(&simage->u.bitmap);
#endif
        /* Images must be added to the cache only after they are compressed
           in order to prevent starvation in the client between pixmap_cache and
           global dictionary (in cases of multiple monitors) */
        if (reds_stream_get_family(rcc->stream) == AF_UNIX ||
            !red_compress_image(dcc, &image, &simage->u.bitmap,
                                drawable, can_lossy, &comp_send_data)) {
            SpicePalette *palette;

            red_display_add_image_to_pixmap_cache(rcc, simage, &image, FALSE);

            *bitmap = simage->u.bitmap;
            bitmap->flags = bitmap->flags & SPICE_BITMAP_FLAGS_TOP_DOWN;

            palette = bitmap->palette;
            fill_palette(dcc, palette, &bitmap->flags);
            spice_marshall_Image(m, &image,
                                 &bitmap_palette_out, &lzplt_palette_out);
            spice_assert(lzplt_palette_out == NULL);

            if (bitmap_palette_out && palette) {
                spice_marshall_Palette(bitmap_palette_out, palette);
            }

            spice_marshaller_add_ref_chunks(m, bitmap->data);
            pthread_mutex_unlock(&dcc->pixmap_cache->lock);
            return FILL_BITS_TYPE_BITMAP;
        } else {
            red_display_add_image_to_pixmap_cache(rcc, simage, &image,
                                                  comp_send_data.is_lossy);

            spice_marshall_Image(m, &image,
                                 &bitmap_palette_out, &lzplt_palette_out);
            spice_assert(bitmap_palette_out == NULL);

            marshaller_add_compressed(m, comp_send_data.comp_buf,
                                      comp_send_data.comp_buf_size);

            if (lzplt_palette_out && comp_send_data.lzplt_palette) {
                spice_marshall_Palette(lzplt_palette_out, comp_send_data.lzplt_palette);
            }

            spice_assert(!comp_send_data.is_lossy || can_lossy);
            pthread_mutex_unlock(&dcc->pixmap_cache->lock);
            return (comp_send_data.is_lossy ? FILL_BITS_TYPE_COMPRESS_LOSSY :
                                              FILL_BITS_TYPE_COMPRESS_LOSSLESS);
        }
        break;
    }
    case SPICE_IMAGE_TYPE_QUIC:
        red_display_add_image_to_pixmap_cache(rcc, simage, &image, FALSE);
        image.u.quic = simage->u.quic;
        spice_marshall_Image(m, &image,
                             &bitmap_palette_out, &lzplt_palette_out);
        spice_assert(bitmap_palette_out == NULL);
        spice_assert(lzplt_palette_out == NULL);
        spice_marshaller_add_ref_chunks(m, image.u.quic.data);
        pthread_mutex_unlock(&dcc->pixmap_cache->lock);
        return FILL_BITS_TYPE_COMPRESS_LOSSLESS;
    default:
        spice_error("invalid image type %u", image.descriptor.type);
    }
    pthread_mutex_unlock(&dcc->pixmap_cache->lock);
    return 0;
}

static void fill_mask(RedChannelClient *rcc, SpiceMarshaller *m,
                      SpiceImage *mask_bitmap, Drawable *drawable)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);

    if (mask_bitmap && m) {
        if (dcc->image_compression != SPICE_IMAGE_COMPRESSION_OFF) {
            /* todo: pass compression argument */
            SpiceImageCompression save_img_comp = dcc->image_compression;
            dcc->image_compression = SPICE_IMAGE_COMPRESSION_OFF;
            fill_bits(dcc, m, mask_bitmap, drawable, FALSE);
            dcc->image_compression = save_img_comp;
        } else {
            fill_bits(dcc, m, mask_bitmap, drawable, FALSE);
        }
    }
}

static void fill_attr(SpiceMarshaller *m, SpiceLineAttr *attr, uint32_t group_id)
{
    int i;

    if (m && attr->style_nseg) {
        for (i = 0 ; i < attr->style_nseg; i++) {
            spice_marshaller_add_uint32(m, attr->style[i]);
        }
    }
}

static inline void red_display_reset_send_data(DisplayChannelClient *dcc)
{
    dcc->send_data.free_list.res->count = 0;
    dcc->send_data.num_pixmap_cache_items = 0;
    memset(dcc->send_data.free_list.sync, 0, sizeof(dcc->send_data.free_list.sync));
}

/* set area=NULL for testing the whole surface */
static int is_surface_area_lossy(DisplayChannelClient *dcc, uint32_t surface_id,
                                 const SpiceRect *area, SpiceRect *out_lossy_area)
{
    RedSurface *surface;
    QRegion *surface_lossy_region;
    QRegion lossy_region;
    DisplayChannel *display = DCC_TO_DC(dcc);

    VALIDATE_SURFACE_RETVAL(display, surface_id, FALSE);
    surface = &display->surfaces[surface_id];
    surface_lossy_region = &dcc->surface_client_lossy_region[surface_id];

    if (!area) {
        if (region_is_empty(surface_lossy_region)) {
            return FALSE;
        } else {
            out_lossy_area->top = 0;
            out_lossy_area->left = 0;
            out_lossy_area->bottom = surface->context.height;
            out_lossy_area->right = surface->context.width;
            return TRUE;
        }
    }

    region_init(&lossy_region);
    region_add(&lossy_region, area);
    region_and(&lossy_region, surface_lossy_region);
    if (!region_is_empty(&lossy_region)) {
        out_lossy_area->left = lossy_region.extents.x1;
        out_lossy_area->top = lossy_region.extents.y1;
        out_lossy_area->right = lossy_region.extents.x2;
        out_lossy_area->bottom = lossy_region.extents.y2;
        region_destroy(&lossy_region);
        return TRUE;
    } else {
        return FALSE;
    }
}
/* returns if the bitmap was already sent lossy to the client. If the bitmap hasn't been sent yet
   to the client, returns false. "area" is for surfaces. If area = NULL,
   all the surface is considered. out_lossy_data will hold info about the bitmap, and its lossy
   area in case it is lossy and part of a surface. */
static int is_bitmap_lossy(RedChannelClient *rcc, SpiceImage *image, SpiceRect *area,
                           Drawable *drawable, BitmapData *out_data)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);

    if (image == NULL) {
        // self bitmap
        out_data->type = BITMAP_DATA_TYPE_BITMAP;
        return FALSE;
    }

    if ((image->descriptor.flags & SPICE_IMAGE_FLAGS_CACHE_ME)) {
        int is_hit_lossy;

        out_data->id = image->descriptor.id;
        if (dcc_pixmap_cache_hit(dcc, image->descriptor.id, &is_hit_lossy)) {
            out_data->type = BITMAP_DATA_TYPE_CACHE;
            if (is_hit_lossy) {
                return TRUE;
            } else {
                return FALSE;
            }
        } else {
            out_data->type = BITMAP_DATA_TYPE_BITMAP_TO_CACHE;
        }
    } else {
         out_data->type = BITMAP_DATA_TYPE_BITMAP;
    }

    if (image->descriptor.type != SPICE_IMAGE_TYPE_SURFACE) {
        return FALSE;
    }

    out_data->type = BITMAP_DATA_TYPE_SURFACE;
    out_data->id = image->u.surface.surface_id;

    if (is_surface_area_lossy(dcc, out_data->id,
                              area, &out_data->lossy_rect))
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

static int is_brush_lossy(RedChannelClient *rcc, SpiceBrush *brush,
                          Drawable *drawable, BitmapData *out_data)
{
    if (brush->type == SPICE_BRUSH_TYPE_PATTERN) {
        return is_bitmap_lossy(rcc, brush->u.pattern.pat, NULL,
                               drawable, out_data);
    } else {
        out_data->type = BITMAP_DATA_TYPE_INVALID;
        return FALSE;
    }
}

static void surface_lossy_region_update(DisplayChannelClient *dcc,
                                        Drawable *item, int has_mask, int lossy)
{
    QRegion *surface_lossy_region;
    RedDrawable *drawable;

    if (has_mask && !lossy) {
        return;
    }

    surface_lossy_region = &dcc->surface_client_lossy_region[item->surface_id];
    drawable = item->red_drawable;

    if (drawable->clip.type == SPICE_CLIP_TYPE_RECTS ) {
        QRegion clip_rgn;
        QRegion draw_region;
        region_init(&clip_rgn);
        region_init(&draw_region);
        region_add(&draw_region, &drawable->bbox);
        region_add_clip_rects(&clip_rgn, drawable->clip.rects);
        region_and(&draw_region, &clip_rgn);
        if (lossy) {
            region_or(surface_lossy_region, &draw_region);
        } else {
            region_exclude(surface_lossy_region, &draw_region);
        }

        region_destroy(&clip_rgn);
        region_destroy(&draw_region);
    } else { /* no clip */
        if (!lossy) {
            region_remove(surface_lossy_region, &drawable->bbox);
        } else {
            region_add(surface_lossy_region, &drawable->bbox);
        }
    }
}

static inline int drawable_intersects_with_areas(Drawable *drawable, int surface_ids[],
                                                 SpiceRect *surface_areas[],
                                                 int num_surfaces)
{
    int i;
    for (i = 0; i < num_surfaces; i++) {
        if (surface_ids[i] == drawable->red_drawable->surface_id) {
            if (rect_intersects(surface_areas[i], &drawable->red_drawable->bbox)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static inline int drawable_depends_on_areas(Drawable *drawable,
                                            int surface_ids[],
                                            SpiceRect surface_areas[],
                                            int num_surfaces)
{
    int i;
    RedDrawable *red_drawable;
    int drawable_has_shadow;
    SpiceRect shadow_rect = {0, 0, 0, 0};

    red_drawable = drawable->red_drawable;
    drawable_has_shadow = has_shadow(red_drawable);

    if (drawable_has_shadow) {
       int delta_x = red_drawable->u.copy_bits.src_pos.x - red_drawable->bbox.left;
       int delta_y = red_drawable->u.copy_bits.src_pos.y - red_drawable->bbox.top;

       shadow_rect.left = red_drawable->u.copy_bits.src_pos.x;
       shadow_rect.top = red_drawable->u.copy_bits.src_pos.y;
       shadow_rect.right = red_drawable->bbox.right + delta_x;
       shadow_rect.bottom = red_drawable->bbox.bottom + delta_y;
    }

    for (i = 0; i < num_surfaces; i++) {
        int x;
        int dep_surface_id;

         for (x = 0; x < 3; ++x) {
            dep_surface_id = drawable->surface_deps[x];
            if (dep_surface_id == surface_ids[i]) {
                if (rect_intersects(&surface_areas[i], &red_drawable->surfaces_rects[x])) {
                    return TRUE;
                }
            }
        }

        if (surface_ids[i] == red_drawable->surface_id) {
            if (drawable_has_shadow) {
                if (rect_intersects(&surface_areas[i], &shadow_rect)) {
                    return TRUE;
                }
            }

            // not dependent on dest
            if (red_drawable->effect == QXL_EFFECT_OPAQUE) {
                continue;
            }

            if (rect_intersects(&surface_areas[i], &red_drawable->bbox)) {
                return TRUE;
            }
        }

    }
    return FALSE;
}


static int pipe_rendered_drawables_intersect_with_areas(DisplayChannelClient *dcc,
                                                        int surface_ids[],
                                                        SpiceRect *surface_areas[],
                                                        int num_surfaces)
{
    PipeItem *pipe_item;
    Ring *pipe;

    spice_assert(num_surfaces);
    pipe = &RED_CHANNEL_CLIENT(dcc)->pipe;

    for (pipe_item = (PipeItem *)ring_get_head(pipe);
         pipe_item;
         pipe_item = (PipeItem *)ring_next(pipe, &pipe_item->link))
    {
        Drawable *drawable;

        if (pipe_item->type != PIPE_ITEM_TYPE_DRAW)
            continue;
        drawable = SPICE_CONTAINEROF(pipe_item, DrawablePipeItem, dpi_pipe_item)->drawable;

        if (ring_item_is_linked(&drawable->list_link))
            continue; // item hasn't been rendered

        if (drawable_intersects_with_areas(drawable, surface_ids, surface_areas, num_surfaces)) {
            return TRUE;
        }
    }

    return FALSE;
}

static void red_pipe_replace_rendered_drawables_with_images(DisplayChannelClient *dcc,
                                                            int first_surface_id,
                                                            SpiceRect *first_area)
{
    /* TODO: can't have those statics with multiple clients */
    static int resent_surface_ids[MAX_PIPE_SIZE];
    static SpiceRect resent_areas[MAX_PIPE_SIZE]; // not pointers since drawbales may be released
    int num_resent;
    PipeItem *pipe_item;
    Ring *pipe;

    resent_surface_ids[0] = first_surface_id;
    resent_areas[0] = *first_area;
    num_resent = 1;

    pipe = &RED_CHANNEL_CLIENT(dcc)->pipe;

    // going from the oldest to the newest
    for (pipe_item = (PipeItem *)ring_get_tail(pipe);
         pipe_item;
         pipe_item = (PipeItem *)ring_prev(pipe, &pipe_item->link)) {
        Drawable *drawable;
        DrawablePipeItem *dpi;
        ImageItem *image;

        if (pipe_item->type != PIPE_ITEM_TYPE_DRAW)
            continue;
        dpi = SPICE_CONTAINEROF(pipe_item, DrawablePipeItem, dpi_pipe_item);
        drawable = dpi->drawable;
        if (ring_item_is_linked(&drawable->list_link))
            continue; // item hasn't been rendered

        // When a drawable command, X, depends on bitmaps that were resent,
        // these bitmaps state at the client might not be synchronized with X
        // (i.e., the bitmaps can be more futuristic w.r.t X). Thus, X shouldn't
        // be rendered at the client, and we replace it with an image as well.
        if (!drawable_depends_on_areas(drawable,
                                       resent_surface_ids,
                                       resent_areas,
                                       num_resent)) {
            continue;
        }

        image = red_add_surface_area_image(dcc, drawable->red_drawable->surface_id,
                                           &drawable->red_drawable->bbox, pipe_item, TRUE);
        resent_surface_ids[num_resent] = drawable->red_drawable->surface_id;
        resent_areas[num_resent] = drawable->red_drawable->bbox;
        num_resent++;

        spice_assert(image);
        red_channel_client_pipe_remove_and_release(RED_CHANNEL_CLIENT(dcc), &dpi->dpi_pipe_item);
        pipe_item = &image->link;
    }
}

static void red_add_lossless_drawable_dependencies(RedChannelClient *rcc,
                                                   Drawable *item,
                                                   int deps_surfaces_ids[],
                                                   SpiceRect *deps_areas[],
                                                   int num_deps)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    DisplayChannel *display = DCC_TO_DC(dcc);
    RedDrawable *drawable = item->red_drawable;
    int sync_rendered = FALSE;
    int i;

    if (!ring_item_is_linked(&item->list_link)) {
        /* drawable was already rendered, we may not be able to retrieve the lossless data
           for the lossy areas */
        sync_rendered = TRUE;

        // checking if the drawable itself or one of the other commands
        // that were rendered, affected the areas that need to be resent
        if (!drawable_intersects_with_areas(item, deps_surfaces_ids,
                                            deps_areas, num_deps)) {
            if (pipe_rendered_drawables_intersect_with_areas(dcc,
                                                             deps_surfaces_ids,
                                                             deps_areas,
                                                             num_deps)) {
                sync_rendered = TRUE;
            }
        } else {
            sync_rendered = TRUE;
        }
    } else {
        sync_rendered = FALSE;
        for (i = 0; i < num_deps; i++) {
            red_update_area_till(display, deps_areas[i],
                                 deps_surfaces_ids[i], item);
        }
    }

    if (!sync_rendered) {
        // pushing the pipe item back to the pipe
        dcc_add_drawable_to_tail(dcc, item);
        // the surfaces areas will be sent as DRAW_COPY commands, that
        // will be executed before the current drawable
        for (i = 0; i < num_deps; i++) {
            red_add_surface_area_image(dcc, deps_surfaces_ids[i], deps_areas[i],
                                       red_pipe_get_tail(dcc), FALSE);

        }
    } else {
        int drawable_surface_id[1];
        SpiceRect *drawable_bbox[1];

        drawable_surface_id[0] = drawable->surface_id;
        drawable_bbox[0] = &drawable->bbox;

        // check if the other rendered images in the pipe have updated the drawable bbox
        if (pipe_rendered_drawables_intersect_with_areas(dcc,
                                                         drawable_surface_id,
                                                         drawable_bbox,
                                                         1)) {
            red_pipe_replace_rendered_drawables_with_images(dcc,
                                                            drawable->surface_id,
                                                            &drawable->bbox);
        }

        red_add_surface_area_image(dcc, drawable->surface_id, &drawable->bbox,
                                   red_pipe_get_tail(dcc), TRUE);
    }
}

static void red_marshall_qxl_draw_fill(RedChannelClient *rcc,
                                       SpiceMarshaller *base_marshaller,
                                       DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    SpiceMarshaller *brush_pat_out;
    SpiceMarshaller *mask_bitmap_out;
    SpiceFill fill;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_FILL, &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    fill = drawable->u.fill;
    spice_marshall_Fill(base_marshaller,
                        &fill,
                        &brush_pat_out,
                        &mask_bitmap_out);

    if (brush_pat_out) {
        fill_bits(dcc, brush_pat_out, fill.brush.u.pattern.pat, item, FALSE);
    }

    fill_mask(rcc, mask_bitmap_out, fill.mask.bitmap, item);
}


static void red_lossy_marshall_qxl_draw_fill(RedChannelClient *rcc,
                                             SpiceMarshaller *m,
                                             DrawablePipeItem *dpi)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;

    int dest_allowed_lossy = FALSE;
    int dest_is_lossy = FALSE;
    SpiceRect dest_lossy_area;
    int brush_is_lossy;
    BitmapData brush_bitmap_data;
    uint16_t rop;

    rop = drawable->u.fill.rop_descriptor;

    dest_allowed_lossy = !((rop & SPICE_ROPD_OP_OR) ||
                           (rop & SPICE_ROPD_OP_AND) ||
                           (rop & SPICE_ROPD_OP_XOR));

    brush_is_lossy = is_brush_lossy(rcc, &drawable->u.fill.brush, item,
                                    &brush_bitmap_data);
    if (!dest_allowed_lossy) {
        dest_is_lossy = is_surface_area_lossy(dcc, item->surface_id, &drawable->bbox,
                                              &dest_lossy_area);
    }

    if (!dest_is_lossy &&
        !(brush_is_lossy && (brush_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE))) {
        int has_mask = !!drawable->u.fill.mask.bitmap;

        red_marshall_qxl_draw_fill(rcc, m, dpi);
        // either the brush operation is opaque, or the dest is not lossy
        surface_lossy_region_update(dcc, item, has_mask, FALSE);
    } else {
        int resend_surface_ids[2];
        SpiceRect *resend_areas[2];
        int num_resend = 0;

        if (dest_is_lossy) {
            resend_surface_ids[num_resend] = item->surface_id;
            resend_areas[num_resend] = &dest_lossy_area;
            num_resend++;
        }

        if (brush_is_lossy && (brush_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) {
            resend_surface_ids[num_resend] = brush_bitmap_data.id;
            resend_areas[num_resend] = &brush_bitmap_data.lossy_rect;
            num_resend++;
        }

        red_add_lossless_drawable_dependencies(rcc, item,
                                               resend_surface_ids, resend_areas, num_resend);
    }
}

static FillBitsType red_marshall_qxl_draw_opaque(RedChannelClient *rcc,
                                                 SpiceMarshaller *base_marshaller,
                                                 DrawablePipeItem *dpi, int src_allowed_lossy)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;
    SpiceMarshaller *brush_pat_out;
    SpiceMarshaller *src_bitmap_out;
    SpiceMarshaller *mask_bitmap_out;
    SpiceOpaque opaque;
    FillBitsType src_send_type;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_OPAQUE, &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    opaque = drawable->u.opaque;
    spice_marshall_Opaque(base_marshaller,
                          &opaque,
                          &src_bitmap_out,
                          &brush_pat_out,
                          &mask_bitmap_out);

    src_send_type = fill_bits(dcc, src_bitmap_out, opaque.src_bitmap, item,
                              src_allowed_lossy);

    if (brush_pat_out) {
        fill_bits(dcc, brush_pat_out, opaque.brush.u.pattern.pat, item, FALSE);
    }
    fill_mask(rcc, mask_bitmap_out, opaque.mask.bitmap, item);

    return src_send_type;
}

static void red_lossy_marshall_qxl_draw_opaque(RedChannelClient *rcc,
                                               SpiceMarshaller *m,
                                               DrawablePipeItem *dpi)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;

    int src_allowed_lossy;
    int rop;
    int src_is_lossy = FALSE;
    int brush_is_lossy = FALSE;
    BitmapData src_bitmap_data;
    BitmapData brush_bitmap_data;

    rop = drawable->u.opaque.rop_descriptor;
    src_allowed_lossy = !((rop & SPICE_ROPD_OP_OR) ||
                          (rop & SPICE_ROPD_OP_AND) ||
                          (rop & SPICE_ROPD_OP_XOR));

    brush_is_lossy = is_brush_lossy(rcc, &drawable->u.opaque.brush, item,
                                    &brush_bitmap_data);

    if (!src_allowed_lossy) {
        src_is_lossy = is_bitmap_lossy(rcc, drawable->u.opaque.src_bitmap,
                                       &drawable->u.opaque.src_area,
                                       item,
                                       &src_bitmap_data);
    }

    if (!(brush_is_lossy && (brush_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) &&
        !(src_is_lossy && (src_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE))) {
        FillBitsType src_send_type;
        int has_mask = !!drawable->u.opaque.mask.bitmap;

        src_send_type = red_marshall_qxl_draw_opaque(rcc, m, dpi, src_allowed_lossy);
        if (src_send_type == FILL_BITS_TYPE_COMPRESS_LOSSY) {
            src_is_lossy = TRUE;
        } else if (src_send_type == FILL_BITS_TYPE_COMPRESS_LOSSLESS) {
            src_is_lossy = FALSE;
        }

        surface_lossy_region_update(dcc, item, has_mask, src_is_lossy);
    } else {
        int resend_surface_ids[2];
        SpiceRect *resend_areas[2];
        int num_resend = 0;

        if (src_is_lossy && (src_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) {
            resend_surface_ids[num_resend] = src_bitmap_data.id;
            resend_areas[num_resend] = &src_bitmap_data.lossy_rect;
            num_resend++;
        }

        if (brush_is_lossy && (brush_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) {
            resend_surface_ids[num_resend] = brush_bitmap_data.id;
            resend_areas[num_resend] = &brush_bitmap_data.lossy_rect;
            num_resend++;
        }

        red_add_lossless_drawable_dependencies(rcc, item,
                                               resend_surface_ids, resend_areas, num_resend);
    }
}

static FillBitsType red_marshall_qxl_draw_copy(RedChannelClient *rcc,
                                               SpiceMarshaller *base_marshaller,
                                               DrawablePipeItem *dpi, int src_allowed_lossy)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;
    SpiceMarshaller *src_bitmap_out;
    SpiceMarshaller *mask_bitmap_out;
    SpiceCopy copy;
    FillBitsType src_send_type;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_COPY, &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    copy = drawable->u.copy;
    spice_marshall_Copy(base_marshaller,
                        &copy,
                        &src_bitmap_out,
                        &mask_bitmap_out);

    src_send_type = fill_bits(dcc, src_bitmap_out, copy.src_bitmap, item, src_allowed_lossy);
    fill_mask(rcc, mask_bitmap_out, copy.mask.bitmap, item);

    return src_send_type;
}

static void red_lossy_marshall_qxl_draw_copy(RedChannelClient *rcc,
                                             SpiceMarshaller *base_marshaller,
                                             DrawablePipeItem *dpi)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;
    int has_mask = !!drawable->u.copy.mask.bitmap;
    int src_is_lossy;
    BitmapData src_bitmap_data;
    FillBitsType src_send_type;

    src_is_lossy = is_bitmap_lossy(rcc, drawable->u.copy.src_bitmap,
                                   &drawable->u.copy.src_area, item, &src_bitmap_data);

    src_send_type = red_marshall_qxl_draw_copy(rcc, base_marshaller, dpi, TRUE);
    if (src_send_type == FILL_BITS_TYPE_COMPRESS_LOSSY) {
        src_is_lossy = TRUE;
    } else if (src_send_type == FILL_BITS_TYPE_COMPRESS_LOSSLESS) {
        src_is_lossy = FALSE;
    }
    surface_lossy_region_update(dcc, item, has_mask,
                                src_is_lossy);
}

static void red_marshall_qxl_draw_transparent(RedChannelClient *rcc,
                                              SpiceMarshaller *base_marshaller,
                                              DrawablePipeItem *dpi)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;
    SpiceMarshaller *src_bitmap_out;
    SpiceTransparent transparent;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_TRANSPARENT,
                                      &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    transparent = drawable->u.transparent;
    spice_marshall_Transparent(base_marshaller,
                               &transparent,
                               &src_bitmap_out);
    fill_bits(dcc, src_bitmap_out, transparent.src_bitmap, item, FALSE);
}

static void red_lossy_marshall_qxl_draw_transparent(RedChannelClient *rcc,
                                                    SpiceMarshaller *base_marshaller,
                                                    DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;
    int src_is_lossy;
    BitmapData src_bitmap_data;

    src_is_lossy = is_bitmap_lossy(rcc, drawable->u.transparent.src_bitmap,
                                   &drawable->u.transparent.src_area, item, &src_bitmap_data);

    if (!src_is_lossy || (src_bitmap_data.type != BITMAP_DATA_TYPE_SURFACE)) {
        red_marshall_qxl_draw_transparent(rcc, base_marshaller, dpi);
        // don't update surface lossy region since transperent areas might be lossy
    } else {
        int resend_surface_ids[1];
        SpiceRect *resend_areas[1];

        resend_surface_ids[0] = src_bitmap_data.id;
        resend_areas[0] = &src_bitmap_data.lossy_rect;

        red_add_lossless_drawable_dependencies(rcc, item,
                                               resend_surface_ids, resend_areas, 1);
    }
}

static FillBitsType red_marshall_qxl_draw_alpha_blend(RedChannelClient *rcc,
                                                      SpiceMarshaller *base_marshaller,
                                                      DrawablePipeItem *dpi,
                                                      int src_allowed_lossy)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    SpiceMarshaller *src_bitmap_out;
    SpiceAlphaBlend alpha_blend;
    FillBitsType src_send_type;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_ALPHA_BLEND,
                                      &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    alpha_blend = drawable->u.alpha_blend;
    spice_marshall_AlphaBlend(base_marshaller,
                              &alpha_blend,
                              &src_bitmap_out);
    src_send_type = fill_bits(dcc, src_bitmap_out, alpha_blend.src_bitmap, item,
                              src_allowed_lossy);

    return src_send_type;
}

static void red_lossy_marshall_qxl_draw_alpha_blend(RedChannelClient *rcc,
                                                    SpiceMarshaller *base_marshaller,
                                                    DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    int src_is_lossy;
    BitmapData src_bitmap_data;
    FillBitsType src_send_type;

    src_is_lossy = is_bitmap_lossy(rcc, drawable->u.alpha_blend.src_bitmap,
                                   &drawable->u.alpha_blend.src_area, item, &src_bitmap_data);

    src_send_type = red_marshall_qxl_draw_alpha_blend(rcc, base_marshaller, dpi, TRUE);

    if (src_send_type == FILL_BITS_TYPE_COMPRESS_LOSSY) {
        src_is_lossy = TRUE;
    } else if (src_send_type == FILL_BITS_TYPE_COMPRESS_LOSSLESS) {
        src_is_lossy = FALSE;
    }

    if (src_is_lossy) {
        surface_lossy_region_update(dcc, item, FALSE, src_is_lossy);
    } // else, the area stays lossy/lossless as the destination
}

static void red_marshall_qxl_copy_bits(RedChannelClient *rcc,
                                       SpiceMarshaller *base_marshaller,
                                       DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;
    SpicePoint copy_bits;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_COPY_BITS, &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    copy_bits = drawable->u.copy_bits.src_pos;
    spice_marshall_Point(base_marshaller,
                         &copy_bits);
}

static void red_lossy_marshall_qxl_copy_bits(RedChannelClient *rcc,
                                             SpiceMarshaller *base_marshaller,
                                             DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    SpiceRect src_rect;
    int horz_offset;
    int vert_offset;
    int src_is_lossy;
    SpiceRect src_lossy_area;

    red_marshall_qxl_copy_bits(rcc, base_marshaller, dpi);

    horz_offset = drawable->u.copy_bits.src_pos.x - drawable->bbox.left;
    vert_offset = drawable->u.copy_bits.src_pos.y - drawable->bbox.top;

    src_rect.left = drawable->u.copy_bits.src_pos.x;
    src_rect.top = drawable->u.copy_bits.src_pos.y;
    src_rect.right = drawable->bbox.right + horz_offset;
    src_rect.bottom = drawable->bbox.bottom + vert_offset;

    src_is_lossy = is_surface_area_lossy(dcc, item->surface_id,
                                         &src_rect, &src_lossy_area);

    surface_lossy_region_update(dcc, item, FALSE, src_is_lossy);
}

static void red_marshall_qxl_draw_blend(RedChannelClient *rcc,
                                        SpiceMarshaller *base_marshaller,
                                        DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    SpiceMarshaller *src_bitmap_out;
    SpiceMarshaller *mask_bitmap_out;
    SpiceBlend blend;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_BLEND, &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    blend = drawable->u.blend;
    spice_marshall_Blend(base_marshaller,
                         &blend,
                         &src_bitmap_out,
                         &mask_bitmap_out);

    fill_bits(dcc, src_bitmap_out, blend.src_bitmap, item, FALSE);

    fill_mask(rcc, mask_bitmap_out, blend.mask.bitmap, item);
}

static void red_lossy_marshall_qxl_draw_blend(RedChannelClient *rcc,
                                              SpiceMarshaller *base_marshaller,
                                              DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    int src_is_lossy;
    BitmapData src_bitmap_data;
    int dest_is_lossy;
    SpiceRect dest_lossy_area;

    src_is_lossy = is_bitmap_lossy(rcc, drawable->u.blend.src_bitmap,
                                   &drawable->u.blend.src_area, item, &src_bitmap_data);
    dest_is_lossy = is_surface_area_lossy(dcc, drawable->surface_id,
                                          &drawable->bbox, &dest_lossy_area);

    if (!dest_is_lossy &&
        (!src_is_lossy || (src_bitmap_data.type != BITMAP_DATA_TYPE_SURFACE))) {
        red_marshall_qxl_draw_blend(rcc, base_marshaller, dpi);
    } else {
        int resend_surface_ids[2];
        SpiceRect *resend_areas[2];
        int num_resend = 0;

        if (src_is_lossy && (src_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) {
            resend_surface_ids[num_resend] = src_bitmap_data.id;
            resend_areas[num_resend] = &src_bitmap_data.lossy_rect;
            num_resend++;
        }

        if (dest_is_lossy) {
            resend_surface_ids[num_resend] = item->surface_id;
            resend_areas[num_resend] = &dest_lossy_area;
            num_resend++;
        }

        red_add_lossless_drawable_dependencies(rcc, item,
                                               resend_surface_ids, resend_areas, num_resend);
    }
}

static void red_marshall_qxl_draw_blackness(RedChannelClient *rcc,
                                            SpiceMarshaller *base_marshaller,
                                            DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;
    SpiceMarshaller *mask_bitmap_out;
    SpiceBlackness blackness;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_BLACKNESS, &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    blackness = drawable->u.blackness;

    spice_marshall_Blackness(base_marshaller,
                             &blackness,
                             &mask_bitmap_out);

    fill_mask(rcc, mask_bitmap_out, blackness.mask.bitmap, item);
}

static void red_lossy_marshall_qxl_draw_blackness(RedChannelClient *rcc,
                                                  SpiceMarshaller *base_marshaller,
                                                  DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    int has_mask = !!drawable->u.blackness.mask.bitmap;

    red_marshall_qxl_draw_blackness(rcc, base_marshaller, dpi);

    surface_lossy_region_update(dcc, item, has_mask, FALSE);
}

static void red_marshall_qxl_draw_whiteness(RedChannelClient *rcc,
                                            SpiceMarshaller *base_marshaller,
                                            DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;
    SpiceMarshaller *mask_bitmap_out;
    SpiceWhiteness whiteness;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_WHITENESS, &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    whiteness = drawable->u.whiteness;

    spice_marshall_Whiteness(base_marshaller,
                             &whiteness,
                             &mask_bitmap_out);

    fill_mask(rcc, mask_bitmap_out, whiteness.mask.bitmap, item);
}

static void red_lossy_marshall_qxl_draw_whiteness(RedChannelClient *rcc,
                                                  SpiceMarshaller *base_marshaller,
                                                  DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    int has_mask = !!drawable->u.whiteness.mask.bitmap;

    red_marshall_qxl_draw_whiteness(rcc, base_marshaller, dpi);

    surface_lossy_region_update(dcc, item, has_mask, FALSE);
}

static void red_marshall_qxl_draw_inverse(RedChannelClient *rcc,
                                          SpiceMarshaller *base_marshaller,
                                          Drawable *item)
{
    RedDrawable *drawable = item->red_drawable;
    SpiceMarshaller *mask_bitmap_out;
    SpiceInvers inverse;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_INVERS, NULL);
    fill_base(base_marshaller, item);
    inverse = drawable->u.invers;

    spice_marshall_Invers(base_marshaller,
                          &inverse,
                          &mask_bitmap_out);

    fill_mask(rcc, mask_bitmap_out, inverse.mask.bitmap, item);
}

static void red_lossy_marshall_qxl_draw_inverse(RedChannelClient *rcc,
                                                SpiceMarshaller *base_marshaller,
                                                Drawable *item)
{
    red_marshall_qxl_draw_inverse(rcc, base_marshaller, item);
}

static void red_marshall_qxl_draw_rop3(RedChannelClient *rcc,
                                       SpiceMarshaller *base_marshaller,
                                       DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    SpiceRop3 rop3;
    SpiceMarshaller *src_bitmap_out;
    SpiceMarshaller *brush_pat_out;
    SpiceMarshaller *mask_bitmap_out;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_ROP3, &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    rop3 = drawable->u.rop3;
    spice_marshall_Rop3(base_marshaller,
                        &rop3,
                        &src_bitmap_out,
                        &brush_pat_out,
                        &mask_bitmap_out);

    fill_bits(dcc, src_bitmap_out, rop3.src_bitmap, item, FALSE);

    if (brush_pat_out) {
        fill_bits(dcc, brush_pat_out, rop3.brush.u.pattern.pat, item, FALSE);
    }
    fill_mask(rcc, mask_bitmap_out, rop3.mask.bitmap, item);
}

static void red_lossy_marshall_qxl_draw_rop3(RedChannelClient *rcc,
                                             SpiceMarshaller *base_marshaller,
                                             DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    int src_is_lossy;
    BitmapData src_bitmap_data;
    int brush_is_lossy;
    BitmapData brush_bitmap_data;
    int dest_is_lossy;
    SpiceRect dest_lossy_area;

    src_is_lossy = is_bitmap_lossy(rcc, drawable->u.rop3.src_bitmap,
                                   &drawable->u.rop3.src_area, item, &src_bitmap_data);
    brush_is_lossy = is_brush_lossy(rcc, &drawable->u.rop3.brush, item,
                                    &brush_bitmap_data);
    dest_is_lossy = is_surface_area_lossy(dcc, drawable->surface_id,
                                          &drawable->bbox, &dest_lossy_area);

    if ((!src_is_lossy || (src_bitmap_data.type != BITMAP_DATA_TYPE_SURFACE)) &&
        (!brush_is_lossy || (brush_bitmap_data.type != BITMAP_DATA_TYPE_SURFACE)) &&
        !dest_is_lossy) {
        int has_mask = !!drawable->u.rop3.mask.bitmap;
        red_marshall_qxl_draw_rop3(rcc, base_marshaller, dpi);
        surface_lossy_region_update(dcc, item, has_mask, FALSE);
    } else {
        int resend_surface_ids[3];
        SpiceRect *resend_areas[3];
        int num_resend = 0;

        if (src_is_lossy && (src_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) {
            resend_surface_ids[num_resend] = src_bitmap_data.id;
            resend_areas[num_resend] = &src_bitmap_data.lossy_rect;
            num_resend++;
        }

        if (brush_is_lossy && (brush_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) {
            resend_surface_ids[num_resend] = brush_bitmap_data.id;
            resend_areas[num_resend] = &brush_bitmap_data.lossy_rect;
            num_resend++;
        }

        if (dest_is_lossy) {
            resend_surface_ids[num_resend] = item->surface_id;
            resend_areas[num_resend] = &dest_lossy_area;
            num_resend++;
        }

        red_add_lossless_drawable_dependencies(rcc, item,
                                               resend_surface_ids, resend_areas, num_resend);
    }
}

static void red_marshall_qxl_draw_composite(RedChannelClient *rcc,
                                            SpiceMarshaller *base_marshaller,
                                            DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    SpiceMarshaller *src_bitmap_out;
    SpiceMarshaller *mask_bitmap_out;
    SpiceComposite composite;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_COMPOSITE, &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    composite = drawable->u.composite;
    spice_marshall_Composite(base_marshaller,
                             &composite,
                             &src_bitmap_out,
                             &mask_bitmap_out);

    fill_bits(dcc, src_bitmap_out, composite.src_bitmap, item, FALSE);
    if (mask_bitmap_out) {
        fill_bits(dcc, mask_bitmap_out, composite.mask_bitmap, item, FALSE);
    }
}

static void red_lossy_marshall_qxl_draw_composite(RedChannelClient *rcc,
                                                  SpiceMarshaller *base_marshaller,
                                                  DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    int src_is_lossy;
    BitmapData src_bitmap_data;
    int mask_is_lossy;
    BitmapData mask_bitmap_data;
    int dest_is_lossy;
    SpiceRect dest_lossy_area;

    src_is_lossy = is_bitmap_lossy(rcc, drawable->u.composite.src_bitmap,
                                   NULL, item, &src_bitmap_data);
    mask_is_lossy = drawable->u.composite.mask_bitmap &&
        is_bitmap_lossy(rcc, drawable->u.composite.mask_bitmap, NULL, item, &mask_bitmap_data);

    dest_is_lossy = is_surface_area_lossy(dcc, drawable->surface_id,
                                          &drawable->bbox, &dest_lossy_area);

    if ((!src_is_lossy || (src_bitmap_data.type != BITMAP_DATA_TYPE_SURFACE))   &&
        (!mask_is_lossy || (mask_bitmap_data.type != BITMAP_DATA_TYPE_SURFACE)) &&
        !dest_is_lossy) {
        red_marshall_qxl_draw_composite(rcc, base_marshaller, dpi);
        surface_lossy_region_update(dcc, item, FALSE, FALSE);
    }
    else {
        int resend_surface_ids[3];
        SpiceRect *resend_areas[3];
        int num_resend = 0;

        if (src_is_lossy && (src_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) {
            resend_surface_ids[num_resend] = src_bitmap_data.id;
            resend_areas[num_resend] = &src_bitmap_data.lossy_rect;
            num_resend++;
        }

        if (mask_is_lossy && (mask_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) {
            resend_surface_ids[num_resend] = mask_bitmap_data.id;
            resend_areas[num_resend] = &mask_bitmap_data.lossy_rect;
            num_resend++;
        }

        if (dest_is_lossy) {
            resend_surface_ids[num_resend] = item->surface_id;
            resend_areas[num_resend] = &dest_lossy_area;
            num_resend++;
        }

        red_add_lossless_drawable_dependencies(rcc, item,
                                               resend_surface_ids, resend_areas, num_resend);
    }
}

static void red_marshall_qxl_draw_stroke(RedChannelClient *rcc,
                                         SpiceMarshaller *base_marshaller,
                                         DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    SpiceStroke stroke;
    SpiceMarshaller *brush_pat_out;
    SpiceMarshaller *style_out;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_STROKE, &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    stroke = drawable->u.stroke;
    spice_marshall_Stroke(base_marshaller,
                          &stroke,
                          &style_out,
                          &brush_pat_out);

    fill_attr(style_out, &stroke.attr, item->group_id);
    if (brush_pat_out) {
        fill_bits(dcc, brush_pat_out, stroke.brush.u.pattern.pat, item, FALSE);
    }
}

static void red_lossy_marshall_qxl_draw_stroke(RedChannelClient *rcc,
                                               SpiceMarshaller *base_marshaller,
                                               DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    int brush_is_lossy;
    BitmapData brush_bitmap_data;
    int dest_is_lossy = FALSE;
    SpiceRect dest_lossy_area;
    int rop;

    brush_is_lossy = is_brush_lossy(rcc, &drawable->u.stroke.brush, item,
                                    &brush_bitmap_data);

    // back_mode is not used at the client. Ignoring.
    rop = drawable->u.stroke.fore_mode;

    // assuming that if the brush type is solid, the destination can
    // be lossy, no matter what the rop is.
    if (drawable->u.stroke.brush.type != SPICE_BRUSH_TYPE_SOLID &&
        ((rop & SPICE_ROPD_OP_OR) || (rop & SPICE_ROPD_OP_AND) ||
        (rop & SPICE_ROPD_OP_XOR))) {
        dest_is_lossy = is_surface_area_lossy(dcc, drawable->surface_id,
                                              &drawable->bbox, &dest_lossy_area);
    }

    if (!dest_is_lossy &&
        (!brush_is_lossy || (brush_bitmap_data.type != BITMAP_DATA_TYPE_SURFACE)))
    {
        red_marshall_qxl_draw_stroke(rcc, base_marshaller, dpi);
    } else {
        int resend_surface_ids[2];
        SpiceRect *resend_areas[2];
        int num_resend = 0;

        if (brush_is_lossy && (brush_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) {
            resend_surface_ids[num_resend] = brush_bitmap_data.id;
            resend_areas[num_resend] = &brush_bitmap_data.lossy_rect;
            num_resend++;
        }

        // TODO: use the path in order to resend smaller areas
        if (dest_is_lossy) {
            resend_surface_ids[num_resend] = drawable->surface_id;
            resend_areas[num_resend] = &dest_lossy_area;
            num_resend++;
        }

        red_add_lossless_drawable_dependencies(rcc, item,
                                               resend_surface_ids, resend_areas, num_resend);
    }
}

static void red_marshall_qxl_draw_text(RedChannelClient *rcc,
                                       SpiceMarshaller *base_marshaller,
                                       DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    SpiceText text;
    SpiceMarshaller *brush_pat_out;
    SpiceMarshaller *back_brush_pat_out;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_TEXT, &dpi->dpi_pipe_item);
    fill_base(base_marshaller, item);
    text = drawable->u.text;
    spice_marshall_Text(base_marshaller,
                        &text,
                        &brush_pat_out,
                        &back_brush_pat_out);

    if (brush_pat_out) {
        fill_bits(dcc, brush_pat_out, text.fore_brush.u.pattern.pat, item, FALSE);
    }
    if (back_brush_pat_out) {
        fill_bits(dcc, back_brush_pat_out, text.back_brush.u.pattern.pat, item, FALSE);
    }
}

static void red_lossy_marshall_qxl_draw_text(RedChannelClient *rcc,
                                             SpiceMarshaller *base_marshaller,
                                             DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *drawable = item->red_drawable;
    int fg_is_lossy;
    BitmapData fg_bitmap_data;
    int bg_is_lossy;
    BitmapData bg_bitmap_data;
    int dest_is_lossy = FALSE;
    SpiceRect dest_lossy_area;
    int rop = 0;

    fg_is_lossy = is_brush_lossy(rcc, &drawable->u.text.fore_brush, item,
                                 &fg_bitmap_data);
    bg_is_lossy = is_brush_lossy(rcc, &drawable->u.text.back_brush, item,
                                 &bg_bitmap_data);

    // assuming that if the brush type is solid, the destination can
    // be lossy, no matter what the rop is.
    if (drawable->u.text.fore_brush.type != SPICE_BRUSH_TYPE_SOLID) {
        rop = drawable->u.text.fore_mode;
    }

    if (drawable->u.text.back_brush.type != SPICE_BRUSH_TYPE_SOLID) {
        rop |= drawable->u.text.back_mode;
    }

    if ((rop & SPICE_ROPD_OP_OR) || (rop & SPICE_ROPD_OP_AND) ||
        (rop & SPICE_ROPD_OP_XOR)) {
        dest_is_lossy = is_surface_area_lossy(dcc, drawable->surface_id,
                                              &drawable->bbox, &dest_lossy_area);
    }

    if (!dest_is_lossy &&
        (!fg_is_lossy || (fg_bitmap_data.type != BITMAP_DATA_TYPE_SURFACE)) &&
        (!bg_is_lossy || (bg_bitmap_data.type != BITMAP_DATA_TYPE_SURFACE))) {
        red_marshall_qxl_draw_text(rcc, base_marshaller, dpi);
    } else {
        int resend_surface_ids[3];
        SpiceRect *resend_areas[3];
        int num_resend = 0;

        if (fg_is_lossy && (fg_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) {
            resend_surface_ids[num_resend] = fg_bitmap_data.id;
            resend_areas[num_resend] = &fg_bitmap_data.lossy_rect;
            num_resend++;
        }

        if (bg_is_lossy && (bg_bitmap_data.type == BITMAP_DATA_TYPE_SURFACE)) {
            resend_surface_ids[num_resend] = bg_bitmap_data.id;
            resend_areas[num_resend] = &bg_bitmap_data.lossy_rect;
            num_resend++;
        }

        if (dest_is_lossy) {
            resend_surface_ids[num_resend] = drawable->surface_id;
            resend_areas[num_resend] = &dest_lossy_area;
            num_resend++;
        }
        red_add_lossless_drawable_dependencies(rcc, item,
                                               resend_surface_ids, resend_areas, num_resend);
    }
}

static void red_lossy_marshall_qxl_drawable(RedChannelClient *rcc,
                                            SpiceMarshaller *base_marshaller,
                                            DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    switch (item->red_drawable->type) {
    case QXL_DRAW_FILL:
        red_lossy_marshall_qxl_draw_fill(rcc, base_marshaller, dpi);
        break;
    case QXL_DRAW_OPAQUE:
        red_lossy_marshall_qxl_draw_opaque(rcc, base_marshaller, dpi);
        break;
    case QXL_DRAW_COPY:
        red_lossy_marshall_qxl_draw_copy(rcc, base_marshaller, dpi);
        break;
    case QXL_DRAW_TRANSPARENT:
        red_lossy_marshall_qxl_draw_transparent(rcc, base_marshaller, dpi);
        break;
    case QXL_DRAW_ALPHA_BLEND:
        red_lossy_marshall_qxl_draw_alpha_blend(rcc, base_marshaller, dpi);
        break;
    case QXL_COPY_BITS:
        red_lossy_marshall_qxl_copy_bits(rcc, base_marshaller, dpi);
        break;
    case QXL_DRAW_BLEND:
        red_lossy_marshall_qxl_draw_blend(rcc, base_marshaller, dpi);
        break;
    case QXL_DRAW_BLACKNESS:
        red_lossy_marshall_qxl_draw_blackness(rcc, base_marshaller, dpi);
        break;
    case QXL_DRAW_WHITENESS:
        red_lossy_marshall_qxl_draw_whiteness(rcc, base_marshaller, dpi);
        break;
    case QXL_DRAW_INVERS:
        red_lossy_marshall_qxl_draw_inverse(rcc, base_marshaller, item);
        break;
    case QXL_DRAW_ROP3:
        red_lossy_marshall_qxl_draw_rop3(rcc, base_marshaller, dpi);
        break;
    case QXL_DRAW_COMPOSITE:
        red_lossy_marshall_qxl_draw_composite(rcc, base_marshaller, dpi);
        break;
    case QXL_DRAW_STROKE:
        red_lossy_marshall_qxl_draw_stroke(rcc, base_marshaller, dpi);
        break;
    case QXL_DRAW_TEXT:
        red_lossy_marshall_qxl_draw_text(rcc, base_marshaller, dpi);
        break;
    default:
        spice_error("invalid type");
    }
}

static inline void red_marshall_qxl_drawable(RedChannelClient *rcc,
                                             SpiceMarshaller *m, DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    RedDrawable *drawable = item->red_drawable;

    switch (drawable->type) {
    case QXL_DRAW_FILL:
        red_marshall_qxl_draw_fill(rcc, m, dpi);
        break;
    case QXL_DRAW_OPAQUE:
        red_marshall_qxl_draw_opaque(rcc, m, dpi, FALSE);
        break;
    case QXL_DRAW_COPY:
        red_marshall_qxl_draw_copy(rcc, m, dpi, FALSE);
        break;
    case QXL_DRAW_TRANSPARENT:
        red_marshall_qxl_draw_transparent(rcc, m, dpi);
        break;
    case QXL_DRAW_ALPHA_BLEND:
        red_marshall_qxl_draw_alpha_blend(rcc, m, dpi, FALSE);
        break;
    case QXL_COPY_BITS:
        red_marshall_qxl_copy_bits(rcc, m, dpi);
        break;
    case QXL_DRAW_BLEND:
        red_marshall_qxl_draw_blend(rcc, m, dpi);
        break;
    case QXL_DRAW_BLACKNESS:
        red_marshall_qxl_draw_blackness(rcc, m, dpi);
        break;
    case QXL_DRAW_WHITENESS:
        red_marshall_qxl_draw_whiteness(rcc, m, dpi);
        break;
    case QXL_DRAW_INVERS:
        red_marshall_qxl_draw_inverse(rcc, m, item);
        break;
    case QXL_DRAW_ROP3:
        red_marshall_qxl_draw_rop3(rcc, m, dpi);
        break;
    case QXL_DRAW_STROKE:
        red_marshall_qxl_draw_stroke(rcc, m, dpi);
        break;
    case QXL_DRAW_COMPOSITE:
        red_marshall_qxl_draw_composite(rcc, m, dpi);
        break;
    case QXL_DRAW_TEXT:
        red_marshall_qxl_draw_text(rcc, m, dpi);
        break;
    default:
        spice_error("invalid type");
    }
}

static void display_channel_push_release(DisplayChannelClient *dcc, uint8_t type, uint64_t id,
                                         uint64_t* sync_data)
{
    FreeList *free_list = &dcc->send_data.free_list;
    int i;

    for (i = 0; i < MAX_CACHE_CLIENTS; i++) {
        free_list->sync[i] = MAX(free_list->sync[i], sync_data[i]);
    }

    if (free_list->res->count == free_list->res_size) {
        SpiceResourceList *new_list;
        new_list = spice_malloc(sizeof(*new_list) +
                                free_list->res_size * sizeof(SpiceResourceID) * 2);
        new_list->count = free_list->res->count;
        memcpy(new_list->resources, free_list->res->resources,
               new_list->count * sizeof(SpiceResourceID));
        free(free_list->res);
        free_list->res = new_list;
        free_list->res_size *= 2;
    }
    free_list->res->resources[free_list->res->count].type = type;
    free_list->res->resources[free_list->res->count++].id = id;
}

static inline void display_marshal_sub_msg_inval_list(SpiceMarshaller *m,
                                                       FreeList *free_list)
{
    /* type + size + submessage */
    spice_marshaller_add_uint16(m, SPICE_MSG_DISPLAY_INVAL_LIST);
    spice_marshaller_add_uint32(m, sizeof(*free_list->res) +
                                free_list->res->count * sizeof(free_list->res->resources[0]));
    spice_marshall_msg_display_inval_list(m, free_list->res);
}

static inline void display_marshal_sub_msg_inval_list_wait(SpiceMarshaller *m,
                                                            FreeList *free_list)

{
    /* type + size + submessage */
    spice_marshaller_add_uint16(m, SPICE_MSG_WAIT_FOR_CHANNELS);
    spice_marshaller_add_uint32(m, sizeof(free_list->wait.header) +
                                free_list->wait.header.wait_count * sizeof(free_list->wait.buf[0]));
    spice_marshall_msg_wait_for_channels(m, &free_list->wait.header);
}

/* use legacy SpiceDataHeader (with sub_list) */
static inline void display_channel_send_free_list_legacy(RedChannelClient *rcc)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    FreeList *free_list = &dcc->send_data.free_list;
    SpiceMarshaller *marshaller;
    int sub_list_len = 1;
    SpiceMarshaller *wait_m = NULL;
    SpiceMarshaller *inval_m;
    SpiceMarshaller *sub_list_m;

    marshaller = red_channel_client_get_marshaller(rcc);
    inval_m = spice_marshaller_get_submarshaller(marshaller);

    display_marshal_sub_msg_inval_list(inval_m, free_list);

    if (free_list->wait.header.wait_count) {
        wait_m = spice_marshaller_get_submarshaller(marshaller);
        display_marshal_sub_msg_inval_list_wait(wait_m, free_list);
        sub_list_len++;
    }

    sub_list_m = spice_marshaller_get_submarshaller(marshaller);
    spice_marshaller_add_uint16(sub_list_m, sub_list_len);
    if (wait_m) {
        spice_marshaller_add_uint32(sub_list_m, spice_marshaller_get_offset(wait_m));
    }
    spice_marshaller_add_uint32(sub_list_m, spice_marshaller_get_offset(inval_m));
    red_channel_client_set_header_sub_list(rcc, spice_marshaller_get_offset(sub_list_m));
}

/* use mini header and SPICE_MSG_LIST */
static inline void display_channel_send_free_list(RedChannelClient *rcc)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    FreeList *free_list = &dcc->send_data.free_list;
    int sub_list_len = 1;
    SpiceMarshaller *urgent_marshaller;
    SpiceMarshaller *wait_m = NULL;
    SpiceMarshaller *inval_m;
    uint32_t sub_arr_offset;
    uint32_t wait_offset = 0;
    uint32_t inval_offset = 0;
    int i;

    urgent_marshaller = red_channel_client_switch_to_urgent_sender(rcc);
    for (i = 0; i < dcc->send_data.num_pixmap_cache_items; i++) {
        int dummy;
        /* When using the urgent marshaller, the serial number of the message that is
         * going to be sent right after the SPICE_MSG_LIST, is increased by one.
         * But all this message pixmaps cache references used its old serial.
         * we use pixmap_cache_items to collect these pixmaps, and we update their serial
         * by calling pixmap_cache_hit. */
        dcc_pixmap_cache_hit(dcc, dcc->send_data.pixmap_cache_items[i], &dummy);
    }

    if (free_list->wait.header.wait_count) {
        red_channel_client_init_send_data(rcc, SPICE_MSG_LIST, NULL);
    } else { /* only one message, no need for a list */
        red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_INVAL_LIST, NULL);
        spice_marshall_msg_display_inval_list(urgent_marshaller, free_list->res);
        return;
    }

    inval_m = spice_marshaller_get_submarshaller(urgent_marshaller);
    display_marshal_sub_msg_inval_list(inval_m, free_list);

    if (free_list->wait.header.wait_count) {
        wait_m = spice_marshaller_get_submarshaller(urgent_marshaller);
        display_marshal_sub_msg_inval_list_wait(wait_m, free_list);
        sub_list_len++;
    }

    sub_arr_offset = sub_list_len * sizeof(uint32_t);

    spice_marshaller_add_uint16(urgent_marshaller, sub_list_len);
    inval_offset = spice_marshaller_get_offset(inval_m); // calc the offset before
                                                         // adding the sub list
                                                         // offsets array to the marshaller
    /* adding the array of offsets */
    if (wait_m) {
        wait_offset = spice_marshaller_get_offset(wait_m);
        spice_marshaller_add_uint32(urgent_marshaller, wait_offset + sub_arr_offset);
    }
    spice_marshaller_add_uint32(urgent_marshaller, inval_offset + sub_arr_offset);
}

static inline void display_begin_send_message(RedChannelClient *rcc)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    FreeList *free_list = &dcc->send_data.free_list;

    if (free_list->res->count) {
        int sync_count = 0;
        int i;

        for (i = 0; i < MAX_CACHE_CLIENTS; i++) {
            if (i != dcc->common.id && free_list->sync[i] != 0) {
                free_list->wait.header.wait_list[sync_count].channel_type = SPICE_CHANNEL_DISPLAY;
                free_list->wait.header.wait_list[sync_count].channel_id = i;
                free_list->wait.header.wait_list[sync_count++].message_serial = free_list->sync[i];
            }
        }
        free_list->wait.header.wait_count = sync_count;

        if (rcc->is_mini_header) {
            display_channel_send_free_list(rcc);
        } else {
            display_channel_send_free_list_legacy(rcc);
        }
    }
    red_channel_client_begin_send_message(rcc);
}

static void red_release_video_encoder_buffer(uint8_t *data, void *opaque)
{
    VideoBuffer *buffer = (VideoBuffer*)opaque;
    buffer->free(buffer);
}

static inline int red_marshall_stream_data(RedChannelClient *rcc,
                  SpiceMarshaller *base_marshaller, Drawable *drawable)
{
//    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
//    DisplayChannel *display = DCC_TO_DC(dcc);
//    Stream *stream = drawable->stream;
//    SpiceImage *image;
//    uint32_t frame_mm_time;
//    VideoBuffer *outbuf;
//    int width, height;
//    int ret;
//
//    if (!stream) {
//        spice_assert(drawable->sized_stream);
//        stream = drawable->sized_stream;
//    }
//    spice_assert(drawable->red_drawable->type == QXL_DRAW_COPY);
//
//    image = drawable->red_drawable->u.copy.src_bitmap;
//
//    if (image->descriptor.type != SPICE_IMAGE_TYPE_BITMAP) {
//    		spice_info("ignore image type %d for stream", image->descriptor.type);
//        return FALSE;
//    }
//
//    if (drawable->sized_stream) {
//        if (red_channel_client_test_remote_cap(rcc, SPICE_DISPLAY_CAP_SIZED_STREAM)) {
//            SpiceRect *src_rect = &drawable->red_drawable->u.copy.src_area;
//
//            width = src_rect->right - src_rect->left;
//            height = src_rect->bottom - src_rect->top;
//        } else {
//            return FALSE;
//        }
//    } else {
//        width = stream->width;
//        height = stream->height;
//    }
    
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    DisplayChannel *display = DCC_TO_DC(dcc);
    Stream* stream = &display->streams_buf[display->fullstream_id];
    uint32_t frame_mm_time = reds_get_mm_time();
    VideoBuffer *outbuf;    

    StreamAgent *agent = &dcc->stream_agents[get_stream_id(display, stream)];
    uint64_t time_now = red_get_monotonic_time();

    if (!dcc->use_video_encoder_rate_control) {
        if (time_now - agent->last_send_time < (1000 * 1000 * 1000) / agent->fps) {
            agent->frames--;
#ifdef STREAM_STATS
            agent->stats.num_drops_fps++;
#endif
            return TRUE;
        }
    }

    //render from client canvas
//    dcc->render_canvas->ops->draw_copy(dcc->render_canvas, &drawable->red_drawable->bbox,
//                               				&drawable->red_drawable->clip, &drawable->red_drawable->u.copy);
                        
//		struct timeval begin_time, end_time;
//		gettimeofday(&begin_time,NULL); 
    //fetch canvas
    SpiceCanvas* canvas = dcc->render_canvas;
    //SpiceCanvas* canvas = display->surfaces[0].context.canvas;
		canvas->ops->read_bits(canvas, dcc->render_bitmap.data->chunk[0].data, 
																			dcc->render_bitmap.stride, &dcc->render_area);
//		gettimeofday(&end_time,NULL); 
//		
//		long read_elapse = 1000*(end_time.tv_sec - begin_time.tv_sec) + (end_time.tv_usec - begin_time.tv_usec)/1000;
		
//		gettimeofday(&begin_time,NULL); 
		//encode
   	int ret = agent->video_encoder->encode_frame(agent->video_encoder,
	                                           &dcc->render_bitmap, dcc->render_width, dcc->render_height,
                                             &dcc->render_area,
                                             stream->top_down, frame_mm_time,
                                             &outbuf);
//    gettimeofday(&end_time,NULL); 		
//		long encode_elapse = 1000*(end_time.tv_sec - begin_time.tv_sec) + (end_time.tv_usec - begin_time.tv_usec)/1000;
//		spice_info("read %ld ms, encode frame %ld ms", read_elapse, encode_elapse);
		
    switch (ret) {
    case VIDEO_ENCODER_FRAME_DROP:
        spice_assert(dcc->use_video_encoder_rate_control);
#ifdef STREAM_STATS
        agent->stats.num_drops_fps++;
#endif
        return TRUE;
    case VIDEO_ENCODER_FRAME_UNSUPPORTED:
        return FALSE;
    case VIDEO_ENCODER_FRAME_ENCODE_DONE:
        break;
    default:
        spice_error("bad return value (%d) from VideoEncoder::encode_frame", ret);
        return FALSE;
    }
		
		//dump stream
		{
			FILE* p = fopen("/tmp/stream.264", "a");
			if(NULL != p){
				int written = fwrite( outbuf->data, sizeof(uint8_t), outbuf->size, p);
				if(written != outbuf->size){
					spice_warning("dump stream exception, %d / %d written", written, outbuf->size);
				}
				fclose(p);
			}
		}
		
		
//    if (!drawable->sized_stream) {
//		SpiceMsgDisplayStreamData stream_data;
//		
//		red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_STREAM_DATA, NULL);
//		
//		stream_data.base.id = get_stream_id(display, stream);
//		stream_data.base.multi_media_time = frame_mm_time;
//		stream_data.data_size = outbuf->size;
//		
//		spice_marshall_msg_display_stream_data(base_marshaller, &stream_data);
		    
//		    spice_info("send %d bytes to stream %d",(uint32_t)outbuf->size, stream_data.base.id);
//    } else {
//    		
        SpiceMsgDisplayStreamDataSized stream_data;

        red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_STREAM_DATA_SIZED, NULL);

        stream_data.base.id = get_stream_id(display, stream);
        stream_data.base.multi_media_time = frame_mm_time;
        stream_data.data_size = outbuf->size;
        stream_data.width = dcc->render_width;
        stream_data.height = dcc->render_height;
        stream_data.dest = dcc->render_area;
        
				//spice_info("send %d bytes to sized stream %d",(uint32_t)outbuf->size, stream_data.base.id);
        spice_marshall_msg_display_stream_data_sized(base_marshaller, &stream_data);
//    }
    
    spice_marshaller_add_ref_full(base_marshaller, outbuf->data, outbuf->size,
                                  &red_release_video_encoder_buffer, outbuf);
    agent->last_send_time = time_now;
#ifdef STREAM_STATS
    agent->stats.num_frames_sent++;
    agent->stats.size_sent += outbuf->size;
    agent->stats.end = frame_mm_time;
#endif

    return TRUE;
}

//if render canvas updated
static inline bool render_qxl_drawable(DisplayChannelClient* dcc, SpiceMarshaller *m, Drawable *drawable){
	RedDrawable* red_drawable = drawable->red_drawable;
	SpiceCanvas* canvas = dcc->render_canvas;
	switch(red_drawable->type){
    case QXL_DRAW_FILL: {
        canvas->ops->draw_fill(canvas, &red_drawable->bbox,
                               &red_drawable->clip, &red_drawable->u.fill);
        break;
    }
    case QXL_DRAW_OPAQUE: {
        canvas->ops->draw_opaque(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.opaque);
        //spice_info("draw opaque in surface %d", drawable->surface_id);
        break;
    }
        		
		case QXL_DRAW_COPY: {    
//				bool render = false;
//				if(drawable->stream || drawable->sized_stream){
//					render = true;
//				}
////				else if(SPICE_IMAGE_TYPE_BITMAP == red_drawable->u.copy.src_bitmap->descriptor.type){
////					render = true;
////				}
//				if(render)
//				{
//        	canvas->ops->draw_copy(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.copy);        
//        }
//        else{
////        	spice_info("rendering ignore draw copy %p, shadow %d, streamable %d, image type %d, rod %d, surface %d, region (%d, %d) ~ (%d, %d)",
////										drawable, has_shadow(red_drawable), 
////										drawable->streamable, red_drawable->u.copy.src_bitmap->descriptor.type, red_drawable->u.copy.rop_descriptor, drawable->surface_id,
////										red_drawable->bbox.left, red_drawable->bbox.top,
////										red_drawable->bbox.right, red_drawable->bbox.bottom);
//					return false;
//        }
        canvas->ops->draw_copy(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.copy); 
        break;
    }
		case QXL_DRAW_TRANSPARENT: {
        canvas->ops->draw_transparent(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.transparent);
        break;
    }
    case QXL_DRAW_ALPHA_BLEND: {
        canvas->ops->draw_alpha_blend(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.alpha_blend);
        //spice_info("draw alpha blend in surface %d", drawable->surface_id);
        break;
    }
    case QXL_COPY_BITS: {
        canvas->ops->copy_bits(canvas, &red_drawable->bbox,
                               &red_drawable->clip, &red_drawable->u.copy_bits.src_pos);
        break;
    }
    case QXL_DRAW_BLEND: {
        canvas->ops->draw_blend(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.blend);
        break;
    }
    case QXL_DRAW_BLACKNESS: {
        canvas->ops->draw_blackness(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.blackness);
        break;
    }
    case QXL_DRAW_WHITENESS: {
        canvas->ops->draw_whiteness(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.whiteness);
        //spice_info("draw whiteness in surface %d", drawable->surface_id);
        break;
    }
    case QXL_DRAW_INVERS: {
        canvas->ops->draw_invers(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.invers);
        break;
    }
    case QXL_DRAW_ROP3: {
        canvas->ops->draw_rop3(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.rop3);
        break;
    }
    
    case QXL_DRAW_COMPOSITE: {
        canvas->ops->draw_composite(canvas, &red_drawable->bbox,
                                    &red_drawable->clip, &red_drawable->u.composite);
        break;
    }    
    
    case QXL_DRAW_STROKE: {
        canvas->ops->draw_stroke(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.stroke);
        break;
    }
    case QXL_DRAW_TEXT: {
        canvas->ops->draw_text(canvas, &red_drawable->bbox, &red_drawable->clip, &red_drawable->u.text);
        break;
    }       

		default:
			spice_info("rendering ignore draw type %d:", red_drawable->type);
			return false;
	}
	return true;
}

static inline void marshall_qxl_drawable(RedChannelClient *rcc,
    SpiceMarshaller *m, DrawablePipeItem *dpi)
{
    Drawable *item = dpi->drawable;
    DisplayChannel *display_channel = SPICE_CONTAINEROF(rcc->channel, DisplayChannel, common.base);

    spice_assert(display_channel && rcc);
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    
    //render_qxl_drawable(dcc, m, item);
		//red_marshall_stream_data(rcc, m, item);
		//return;
		
    //render red drawable 
    if(( (item->stream)||( item->sized_stream) )&& (render_qxl_drawable(dcc, m, item) )){
    	//canvas updated
    	if(!red_marshall_stream_data(rcc, m, item)){
    		spice_warning("encode frame ignored");
    	}
    	return;
    }    
    //process unrender draw request
    //spice_info("drawtype %d escaped", item->red_drawable->type);
    if (!display_channel->enable_jpeg)
        red_marshall_qxl_drawable(rcc, m, dpi);
    else
        red_lossy_marshall_qxl_drawable(rcc, m, dpi);
}



static inline void red_marshall_inval_palette(RedChannelClient *rcc,
                                              SpiceMarshaller *base_marshaller,
                                              CacheItem *cache_item)
{
    SpiceMsgDisplayInvalOne inval_one;

    red_channel_client_init_send_data(rcc, cache_item->inval_type, NULL);
    inval_one.id = *(uint64_t *)&cache_item->id;

    spice_marshall_msg_display_inval_palette(base_marshaller, &inval_one);

}

static void display_channel_marshall_migrate_data_surfaces(DisplayChannelClient *dcc,
                                                           SpiceMarshaller *m,
                                                           int lossy)
{
    SpiceMarshaller *m2 = spice_marshaller_get_ptr_submarshaller(m, 0);
    uint32_t *num_surfaces_created;
    uint32_t i;

    num_surfaces_created = (uint32_t *)spice_marshaller_reserve_space(m2, sizeof(uint32_t));
    *num_surfaces_created = 0;
    for (i = 0; i < NUM_SURFACES; i++) {
        SpiceRect lossy_rect;

        if (!dcc->surface_client_created[i]) {
            continue;
        }
        spice_marshaller_add_uint32(m2, i);
        (*num_surfaces_created)++;

        if (!lossy) {
            continue;
        }
        region_extents(&dcc->surface_client_lossy_region[i], &lossy_rect);
        spice_marshaller_add_int32(m2, lossy_rect.left);
        spice_marshaller_add_int32(m2, lossy_rect.top);
        spice_marshaller_add_int32(m2, lossy_rect.right);
        spice_marshaller_add_int32(m2, lossy_rect.bottom);
    }
}

static void display_channel_marshall_migrate_data(RedChannelClient *rcc,
                                                  SpiceMarshaller *base_marshaller)
{
    DisplayChannel *display_channel;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    SpiceMigrateDataDisplay display_data = {0,};

    display_channel = SPICE_CONTAINEROF(rcc->channel, DisplayChannel, common.base);

    red_channel_client_init_send_data(rcc, SPICE_MSG_MIGRATE_DATA, NULL);
    spice_marshaller_add_uint32(base_marshaller, SPICE_MIGRATE_DATA_DISPLAY_MAGIC);
    spice_marshaller_add_uint32(base_marshaller, SPICE_MIGRATE_DATA_DISPLAY_VERSION);

    spice_assert(dcc->pixmap_cache);
    spice_assert(MIGRATE_DATA_DISPLAY_MAX_CACHE_CLIENTS == 4 &&
                 MIGRATE_DATA_DISPLAY_MAX_CACHE_CLIENTS == MAX_CACHE_CLIENTS);

    display_data.message_serial = red_channel_client_get_message_serial(rcc);
    display_data.low_bandwidth_setting = dcc->common.is_low_bandwidth;

    display_data.pixmap_cache_freezer = pixmap_cache_freeze(dcc->pixmap_cache);
    display_data.pixmap_cache_id = dcc->pixmap_cache->id;
    display_data.pixmap_cache_size = dcc->pixmap_cache->size;
    memcpy(display_data.pixmap_cache_clients, dcc->pixmap_cache->sync,
           sizeof(display_data.pixmap_cache_clients));

    spice_assert(dcc->glz_dict);
    red_freeze_glz(dcc);
    display_data.glz_dict_id = dcc->glz_dict->id;
    glz_enc_dictionary_get_restore_data(dcc->glz_dict->dict,
                                        &display_data.glz_dict_data,
                                        &dcc->glz_data.usr);

    /* all data besided the surfaces ref */
    spice_marshaller_add(base_marshaller,
                         (uint8_t *)&display_data, sizeof(display_data) - sizeof(uint32_t));
    display_channel_marshall_migrate_data_surfaces(dcc, base_marshaller,
                                                   display_channel->enable_jpeg);
}

static void display_channel_marshall_pixmap_sync(RedChannelClient *rcc,
                                                 SpiceMarshaller *base_marshaller)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    SpiceMsgWaitForChannels wait;
    PixmapCache *pixmap_cache;

    red_channel_client_init_send_data(rcc, SPICE_MSG_WAIT_FOR_CHANNELS, NULL);
    pixmap_cache = dcc->pixmap_cache;

    pthread_mutex_lock(&pixmap_cache->lock);

    wait.wait_count = 1;
    wait.wait_list[0].channel_type = SPICE_CHANNEL_DISPLAY;
    wait.wait_list[0].channel_id = pixmap_cache->generation_initiator.client;
    wait.wait_list[0].message_serial = pixmap_cache->generation_initiator.message;
    dcc->pixmap_cache_generation = pixmap_cache->generation;
    dcc->pending_pixmaps_sync = FALSE;

    pthread_mutex_unlock(&pixmap_cache->lock);

    spice_marshall_msg_wait_for_channels(base_marshaller, &wait);
}

static void dcc_pixmap_cache_reset(DisplayChannelClient *dcc, SpiceMsgWaitForChannels* sync_data)
{
    PixmapCache *cache = dcc->pixmap_cache;
    uint8_t wait_count;
    uint64_t serial;
    uint32_t i;

    serial = red_channel_client_get_message_serial(RED_CHANNEL_CLIENT(dcc));
    pthread_mutex_lock(&cache->lock);
    pixmap_cache_clear(cache);

    dcc->pixmap_cache_generation = ++cache->generation;
    cache->generation_initiator.client = dcc->common.id;
    cache->generation_initiator.message = serial;
    cache->sync[dcc->common.id] = serial;

    wait_count = 0;
    for (i = 0; i < MAX_CACHE_CLIENTS; i++) {
        if (cache->sync[i] && i != dcc->common.id) {
            sync_data->wait_list[wait_count].channel_type = SPICE_CHANNEL_DISPLAY;
            sync_data->wait_list[wait_count].channel_id = i;
            sync_data->wait_list[wait_count++].message_serial = cache->sync[i];
        }
    }
    sync_data->wait_count = wait_count;
    pthread_mutex_unlock(&cache->lock);
}

static void display_channel_marshall_reset_cache(RedChannelClient *rcc,
                                                 SpiceMarshaller *base_marshaller)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    SpiceMsgWaitForChannels wait;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_INVAL_ALL_PIXMAPS, NULL);
    dcc_pixmap_cache_reset(dcc, &wait);

    spice_marshall_msg_display_inval_all_pixmaps(base_marshaller,
                                                 &wait);
}

static void red_marshall_image(RedChannelClient *rcc, SpiceMarshaller *m, ImageItem *item)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    DisplayChannel *display_channel = DCC_TO_DC(dcc);
    SpiceImage red_image;
    RedWorker *worker;
    SpiceBitmap bitmap;
    SpiceChunks *chunks;
    QRegion *surface_lossy_region;
    int comp_succeeded = FALSE;
    int lossy_comp = FALSE;
    int quic_comp = FALSE;
    SpiceImageCompression comp_mode;
    SpiceMsgDisplayDrawCopy copy;
    SpiceMarshaller *src_bitmap_out, *mask_bitmap_out;
    SpiceMarshaller *bitmap_palette_out, *lzplt_palette_out;

    spice_assert(rcc && display_channel && item);
    worker = display_channel->common.worker;

    QXL_SET_IMAGE_ID(&red_image, QXL_IMAGE_GROUP_RED, ++worker->bits_unique);
    red_image.descriptor.type = SPICE_IMAGE_TYPE_BITMAP;
    red_image.descriptor.flags = item->image_flags;
    red_image.descriptor.width = item->width;
    red_image.descriptor.height = item->height;

    bitmap.format = item->image_format;
    bitmap.flags = 0;
    if (item->top_down) {
        bitmap.flags |= SPICE_BITMAP_FLAGS_TOP_DOWN;
    }
    bitmap.x = item->width;
    bitmap.y = item->height;
    bitmap.stride = item->stride;
    bitmap.palette = 0;
    bitmap.palette_id = 0;

    chunks = spice_chunks_new_linear(item->data, bitmap.stride * bitmap.y);
    bitmap.data = chunks;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_COPY, &item->link);

    copy.base.surface_id = item->surface_id;
    copy.base.box.left = item->pos.x;
    copy.base.box.top = item->pos.y;
    copy.base.box.right = item->pos.x + bitmap.x;
    copy.base.box.bottom = item->pos.y + bitmap.y;
    copy.base.clip.type = SPICE_CLIP_TYPE_NONE;
    copy.data.rop_descriptor = SPICE_ROPD_OP_PUT;
    copy.data.src_area.left = 0;
    copy.data.src_area.top = 0;
    copy.data.src_area.right = bitmap.x;
    copy.data.src_area.bottom = bitmap.y;
    copy.data.scale_mode = 0;
    copy.data.src_bitmap = 0;
    copy.data.mask.flags = 0;
    copy.data.mask.flags = 0;
    copy.data.mask.pos.x = 0;
    copy.data.mask.pos.y = 0;
    copy.data.mask.bitmap = 0;

    spice_marshall_msg_display_draw_copy(m, &copy,
                                         &src_bitmap_out, &mask_bitmap_out);

    compress_send_data_t comp_send_data = {0};

    comp_mode = dcc->image_compression;

    if (((comp_mode == SPICE_IMAGE_COMPRESSION_AUTO_LZ) ||
        (comp_mode == SPICE_IMAGE_COMPRESSION_AUTO_GLZ)) && !bitmap_has_extra_stride(&bitmap)) {

        if (bitmap_fmt_has_graduality(item->image_format)) {
            BitmapGradualType grad_level;

            grad_level = bitmap_get_graduality_level(&bitmap);
            if (grad_level == BITMAP_GRADUAL_HIGH) {
                // if we use lz for alpha, the stride can't be extra
                lossy_comp = display_channel->enable_jpeg && item->can_lossy;
                quic_comp = TRUE;
            }
        }
    } else if (comp_mode == SPICE_IMAGE_COMPRESSION_QUIC) {
        quic_comp = TRUE;
    }

    if (lossy_comp) {
        comp_succeeded = red_jpeg_compress_image(dcc, &red_image,
                                                 &bitmap, &comp_send_data,
                                                 worker->mem_slots.internal_groupslot_id);
    } else if (quic_comp) {
        comp_succeeded = red_quic_compress_image(dcc, &red_image, &bitmap,
                                                 &comp_send_data,
                                                 worker->mem_slots.internal_groupslot_id);
#ifdef USE_LZ4
    } else if (comp_mode == SPICE_IMAGE_COMPRESSION_LZ4 &&
               bitmap_fmt_is_rgb(bitmap.format) &&
               red_channel_client_test_remote_cap(&dcc->common.base,
                                                  SPICE_DISPLAY_CAP_LZ4_COMPRESSION)) {
        comp_succeeded = red_lz4_compress_image(dcc, &red_image, &bitmap,
                                                &comp_send_data,
                                                worker->mem_slots.internal_groupslot_id);
#endif
    } else if (comp_mode != SPICE_IMAGE_COMPRESSION_OFF) {
        comp_succeeded = red_lz_compress_image(dcc, &red_image, &bitmap,
                                               &comp_send_data,
                                               worker->mem_slots.internal_groupslot_id);
    }

    surface_lossy_region = &dcc->surface_client_lossy_region[item->surface_id];
    if (comp_succeeded) {
        spice_marshall_Image(src_bitmap_out, &red_image,
                             &bitmap_palette_out, &lzplt_palette_out);

        marshaller_add_compressed(src_bitmap_out,
                                  comp_send_data.comp_buf, comp_send_data.comp_buf_size);

        if (lzplt_palette_out && comp_send_data.lzplt_palette) {
            spice_marshall_Palette(lzplt_palette_out, comp_send_data.lzplt_palette);
        }

        if (lossy_comp) {
            region_add(surface_lossy_region, &copy.base.box);
        } else {
            region_remove(surface_lossy_region, &copy.base.box);
        }
    } else {
        red_image.descriptor.type = SPICE_IMAGE_TYPE_BITMAP;
        red_image.u.bitmap = bitmap;

        spice_marshall_Image(src_bitmap_out, &red_image,
                             &bitmap_palette_out, &lzplt_palette_out);
        spice_marshaller_add_ref(src_bitmap_out, item->data,
                                 bitmap.y * bitmap.stride);
        region_remove(surface_lossy_region, &copy.base.box);
    }
    spice_chunks_destroy(chunks);
}

static void red_display_marshall_upgrade(RedChannelClient *rcc, SpiceMarshaller *m,
                                         UpgradeItem *item)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    RedDrawable *red_drawable;
    SpiceMsgDisplayDrawCopy copy;
    SpiceMarshaller *src_bitmap_out, *mask_bitmap_out;

    spice_assert(rcc && rcc->channel && item && item->drawable);
    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_DRAW_COPY, &item->base);

    red_drawable = item->drawable->red_drawable;
    spice_assert(red_drawable->type == QXL_DRAW_COPY);
    spice_assert(red_drawable->u.copy.rop_descriptor == SPICE_ROPD_OP_PUT);
    spice_assert(red_drawable->u.copy.mask.bitmap == 0);

    copy.base.surface_id = 0;
    copy.base.box = red_drawable->bbox;
    copy.base.clip.type = SPICE_CLIP_TYPE_RECTS;
    copy.base.clip.rects = item->rects;
    copy.data = red_drawable->u.copy;

    spice_marshall_msg_display_draw_copy(m, &copy,
                                         &src_bitmap_out, &mask_bitmap_out);

    fill_bits(dcc, src_bitmap_out, copy.data.src_bitmap, item->drawable, FALSE);
}

static void red_display_marshall_stream_start(RedChannelClient *rcc,
                     SpiceMarshaller *base_marshaller, StreamAgent *agent)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    Stream *stream = agent->stream;

    agent->last_send_time = 0;
    spice_assert(stream);
    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_STREAM_CREATE, NULL);
    SpiceMsgDisplayStreamCreate stream_create;
    SpiceClipRects clip_rects;

    stream_create.surface_id = 0;
    stream_create.id = get_stream_id(DCC_TO_DC(dcc), stream);
    stream_create.flags = stream->top_down ? SPICE_STREAM_FLAGS_TOP_DOWN : 0;
    stream_create.codec_type = agent->video_encoder->codec_type;

    stream_create.src_width = stream->width;
    stream_create.src_height = stream->height;
    stream_create.stream_width = stream_create.src_width;
    stream_create.stream_height = stream_create.src_height;
    stream_create.dest = stream->dest_area;

    if (stream->current) {
        RedDrawable *red_drawable = stream->current->red_drawable;
        stream_create.clip = red_drawable->clip;
    } else {
        stream_create.clip.type = SPICE_CLIP_TYPE_RECTS;
        clip_rects.num_rects = 0;
        stream_create.clip.rects = &clip_rects;
    }

    stream_create.stamp = 0;

    spice_marshall_msg_display_stream_create(base_marshaller, &stream_create);
}

static void red_display_marshall_stream_clip(RedChannelClient *rcc,
                                         SpiceMarshaller *base_marshaller,
                                         StreamClipItem *item)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    StreamAgent *agent = item->stream_agent;

    spice_assert(agent->stream);

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_STREAM_CLIP, &item->base);
    SpiceMsgDisplayStreamClip stream_clip;

    stream_clip.id = get_stream_id(DCC_TO_DC(dcc), agent->stream);
    stream_clip.clip.type = item->clip_type;
    stream_clip.clip.rects = item->rects;

    spice_marshall_msg_display_stream_clip(base_marshaller, &stream_clip);
}

static void red_display_marshall_stream_end(RedChannelClient *rcc,
                   SpiceMarshaller *base_marshaller, StreamAgent* agent)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    SpiceMsgDisplayStreamDestroy destroy;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_STREAM_DESTROY, NULL);
    destroy.id = get_stream_id(DCC_TO_DC(dcc), agent->stream);
    stream_agent_stop(dcc, agent);
    spice_marshall_msg_display_stream_destroy(base_marshaller, &destroy);
}

static void red_marshall_surface_create(RedChannelClient *rcc,
    SpiceMarshaller *base_marshaller, SpiceMsgSurfaceCreate *surface_create)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);

    region_init(&dcc->surface_client_lossy_region[surface_create->surface_id]);
    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_SURFACE_CREATE, NULL);

    spice_marshall_msg_display_surface_create(base_marshaller, surface_create);
}

static void red_marshall_surface_destroy(RedChannelClient *rcc,
       SpiceMarshaller *base_marshaller, uint32_t surface_id)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    SpiceMsgSurfaceDestroy surface_destroy;

    region_destroy(&dcc->surface_client_lossy_region[surface_id]);
    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_SURFACE_DESTROY, NULL);

    surface_destroy.surface_id = surface_id;

    spice_marshall_msg_display_surface_destroy(base_marshaller, &surface_destroy);
}

static void red_marshall_monitors_config(RedChannelClient *rcc, SpiceMarshaller *base_marshaller,
                                         MonitorsConfig *monitors_config)
{
    int heads_size = sizeof(SpiceHead) * monitors_config->count;
    int i;
    SpiceMsgDisplayMonitorsConfig *msg = spice_malloc0(sizeof(*msg) + heads_size);
    int count = 0; // ignore monitors_config->count, it may contain zero width monitors, remove them now

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_MONITORS_CONFIG, NULL);
    for (i = 0 ; i < monitors_config->count; ++i) {
        if (monitors_config->heads[i].width == 0 || monitors_config->heads[i].height == 0) {
            continue;
        }
        msg->heads[count].id = monitors_config->heads[i].id;
        msg->heads[count].surface_id = monitors_config->heads[i].surface_id;
        msg->heads[count].width = monitors_config->heads[i].width;
        msg->heads[count].height = monitors_config->heads[i].height;
        msg->heads[count].x = monitors_config->heads[i].x;
        msg->heads[count].y = monitors_config->heads[i].y;
        count++;
    }
    msg->count = count;
    msg->max_allowed = monitors_config->max_allowed;
    spice_marshall_msg_display_monitors_config(base_marshaller, msg);
    free(msg);
}

static void red_marshall_stream_activate_report(RedChannelClient *rcc,
                                                SpiceMarshaller *base_marshaller,
                                                uint32_t stream_id)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    StreamAgent *agent = &dcc->stream_agents[stream_id];
    SpiceMsgDisplayStreamActivateReport msg;

    red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_STREAM_ACTIVATE_REPORT, NULL);
    msg.stream_id = stream_id;
    msg.unique_id = agent->report_id;
    msg.max_window_size = RED_STREAM_CLIENT_REPORT_WINDOW;
    msg.timeout_ms = RED_STREAM_CLIENT_REPORT_TIMEOUT;
    spice_marshall_msg_display_stream_activate_report(base_marshaller, &msg);
}

static void display_channel_send_item(RedChannelClient *rcc, PipeItem *pipe_item)
{
    SpiceMarshaller *m = red_channel_client_get_marshaller(rcc);
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);

    red_display_reset_send_data(dcc);
    switch (pipe_item->type) {
    case PIPE_ITEM_TYPE_DRAW: {
        DrawablePipeItem *dpi = SPICE_CONTAINEROF(pipe_item, DrawablePipeItem, dpi_pipe_item);
        marshall_qxl_drawable(rcc, m, dpi);
        //spice_info("send draw");
        break;
    }
    case PIPE_ITEM_TYPE_INVAL_ONE:
        red_marshall_inval_palette(rcc, m, (CacheItem *)pipe_item);
        //spice_info("send invalidate");
        break;
    case PIPE_ITEM_TYPE_STREAM_CREATE: {
        StreamAgent *agent = SPICE_CONTAINEROF(pipe_item, StreamAgent, create_item);
        red_display_marshall_stream_start(rcc, m, agent);
        spice_info("send stream create");
        break;
    }
    case PIPE_ITEM_TYPE_STREAM_CLIP: {
        StreamClipItem* clip_item = (StreamClipItem *)pipe_item;
        red_display_marshall_stream_clip(rcc, m, clip_item);
        spice_info("send stream clip");
        break;
    }
    case PIPE_ITEM_TYPE_STREAM_DESTROY: {
        StreamAgent *agent = SPICE_CONTAINEROF(pipe_item, StreamAgent, destroy_item);
        red_display_marshall_stream_end(rcc, m, agent);
        spice_info("send stream destroy");
        break;
    }
    case PIPE_ITEM_TYPE_UPGRADE:
        red_display_marshall_upgrade(rcc, m, (UpgradeItem *)pipe_item);
        //spice_info("send update");
        break;
    case PIPE_ITEM_TYPE_VERB:
        red_marshall_verb(rcc, (VerbItem*)pipe_item);
        //spice_info("send verb");
        break;
    case PIPE_ITEM_TYPE_MIGRATE_DATA:
        display_channel_marshall_migrate_data(rcc, m);
        //spice_info("send migrage data");
        break;
    case PIPE_ITEM_TYPE_IMAGE:
        red_marshall_image(rcc, m, (ImageItem *)pipe_item);
        //spice_info("send image");
        break;
    case PIPE_ITEM_TYPE_PIXMAP_SYNC:
        display_channel_marshall_pixmap_sync(rcc, m);
        //spice_info("send pixmap sync");
        break;
    case PIPE_ITEM_TYPE_PIXMAP_RESET:
        display_channel_marshall_reset_cache(rcc, m);
        //spice_info("send pixmap reset");
        break;
    case PIPE_ITEM_TYPE_INVAL_PALETTE_CACHE:
        red_reset_palette_cache(dcc);
        red_channel_client_init_send_data(rcc, SPICE_MSG_DISPLAY_INVAL_ALL_PALETTES, NULL);
        //spice_info("send invalidate palette");
        break;
    case PIPE_ITEM_TYPE_CREATE_SURFACE: {
        SurfaceCreateItem *surface_create = SPICE_CONTAINEROF(pipe_item, SurfaceCreateItem,
                                                              pipe_item);
        red_marshall_surface_create(rcc, m, &surface_create->surface_create);
        //("send create surface");
        break;
    }
    case PIPE_ITEM_TYPE_DESTROY_SURFACE: {
        SurfaceDestroyItem *surface_destroy = SPICE_CONTAINEROF(pipe_item, SurfaceDestroyItem,
                                                                pipe_item);
        red_marshall_surface_destroy(rcc, m, surface_destroy->surface_destroy.surface_id);
        //spice_info("send destroy surface %d", surface_destroy->surface_destroy.surface_id);
        break;
    }
    case PIPE_ITEM_TYPE_MONITORS_CONFIG: {
        MonitorsConfigItem *monconf_item = SPICE_CONTAINEROF(pipe_item,
                                                             MonitorsConfigItem, pipe_item);
        red_marshall_monitors_config(rcc, m, monconf_item->monitors_config);
        //spice_info("send monitor config");
        break;
    }
    case PIPE_ITEM_TYPE_STREAM_ACTIVATE_REPORT: {
        StreamActivateReportItem *report_item = SPICE_CONTAINEROF(pipe_item,
                                                                  StreamActivateReportItem,
                                                                  pipe_item);
        red_marshall_stream_activate_report(rcc, m, report_item->stream_id);
        spice_info("send stream report");
        break;
    }
    default:
        spice_error("invalid pipe item type");
    }

    display_channel_client_release_item_before_push(dcc, pipe_item);

    // a message is pending
    if (red_channel_client_send_message_pending(rcc)) {
        display_begin_send_message(rcc);
    }
}

static inline void red_push(RedWorker *worker)
{
    if (worker->cursor_channel) {
        red_channel_push(RED_CHANNEL(worker->cursor_channel));
    }
    if (worker->display_channel) {
        red_channel_push(RED_CHANNEL(worker->display_channel));
    }
}

static void display_channel_client_on_disconnect(RedChannelClient *rcc)
{
    DisplayChannel *display;
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    CommonChannel *common;
    RedWorker *worker;

    if (!rcc) {
        return;
    }
    dcc_deallocate_render(dcc);
    common = SPICE_CONTAINEROF(rcc->channel, CommonChannel, base);
    worker = common->worker;
    display = (DisplayChannel *)rcc->channel;
    spice_assert(display == worker->display_channel);
    display_channel_compress_stats_print(display);
    pixmap_cache_unref(dcc->pixmap_cache);
    dcc->pixmap_cache = NULL;
    red_release_glz(dcc);
    red_reset_palette_cache(dcc);
    free(dcc->send_data.free_list.res);
    if(!display_is_connected(worker)){
    	//stop all stream if no client connected
    	display_channel_stop_streams(display);
    }
    dcc_destroy_stream_agents(dcc);

    // this was the last channel client
    spice_debug("#draw=%d, #red_draw=%d, #glz_draw=%d",
                display->drawable_count, worker->red_drawable_count,
                worker->glz_drawable_count);
}

void red_disconnect_all_display_TODO_remove_me(RedChannel *channel)
{
    // TODO: we need to record the client that actually causes the timeout. So
    // we need to check the locations of the various pipe heads when counting,
    // and disconnect only those/that.
    if (!channel) {
        return;
    }
    red_channel_apply_clients(channel, red_channel_client_disconnect);
}

static void detach_and_stop_streams(DisplayChannel *display)
{
    RingItem *stream_item;

    spice_debug(NULL);
    while ((stream_item = ring_get_head(&display->streams))) {
        Stream *stream = SPICE_CONTAINEROF(stream_item, Stream, link);

        detach_stream_gracefully(display, stream, NULL);
        stream_stop(display, stream);
    }
}

static void red_migrate_display(DisplayChannel *display, RedChannelClient *rcc)
{
    /* We need to stop the streams, and to send upgrade_items to the client.
     * Otherwise, (1) the client might display lossy regions that we don't track
     * (streams are not part of the migration data) (2) streams_timeout may occur
     * after the MIGRATE message has been sent. This can result in messages
     * being sent to the client after MSG_MIGRATE and before MSG_MIGRATE_DATA (e.g.,
     * STREAM_CLIP, STREAM_DESTROY, DRAW_COPY)
     * No message besides MSG_MIGRATE_DATA should be sent after MSG_MIGRATE.
     * Notice that detach_and_stop_streams won't lead to any dev ram changes, since
     * handle_dev_stop already took care of releasing all the dev ram resources.
     */
    detach_and_stop_streams(display);
    if (red_channel_client_is_connected(rcc)) {
        red_channel_client_default_migrate(rcc);
    }
}

#ifdef USE_OPENGL
static SpiceCanvas *create_ogl_context_common(DisplayChannel *display, OGLCtx *ctx,
                                              uint32_t width, uint32_t height,
                                              int32_t stride, uint8_t depth)
{
    SpiceCanvas *canvas;

    oglctx_make_current(ctx);
    if (!(canvas = gl_canvas_create(width, height, depth, &display->image_cache.base,
                                    &display->image_surfaces, NULL, NULL, NULL))) {
        return NULL;
    }

    spice_canvas_set_usr_data(canvas, ctx, (spice_destroy_fn_t)oglctx_destroy);

    canvas->ops->clear(canvas);

    return canvas;
}

static SpiceCanvas *create_ogl_pbuf_context(DisplayChannel *display, uint32_t width,
                                            uint32_t height, int32_t stride, uint8_t depth)
{
    OGLCtx *ctx;
    SpiceCanvas *canvas;

    if (!(ctx = pbuf_create(width, height))) {
        return NULL;
    }

    if (!(canvas = create_ogl_context_common(display, ctx, width, height, stride, depth))) {
        oglctx_destroy(ctx);
        return NULL;
    }

    return canvas;
}

static SpiceCanvas *create_ogl_pixmap_context(DisplayChannel *display, uint32_t width,
                                              uint32_t height, int32_t stride, uint8_t depth)
{
    OGLCtx *ctx;
    SpiceCanvas *canvas;

    if (!(ctx = pixmap_create(width, height))) {
        return NULL;
    }

    if (!(canvas = create_ogl_context_common(display, ctx, width, height, stride, depth))) {
        oglctx_destroy(ctx);
        return NULL;
    }

    return canvas;
}
#endif

static inline void *create_canvas_for_surface(DisplayChannel *display, RedSurface *surface,
                                              uint32_t renderer, uint32_t width, uint32_t height,
                                              int32_t stride, uint32_t format, void *line_0)
{
    SpiceCanvas *canvas;

    switch (renderer) {
    case RED_RENDERER_SW:
        canvas = canvas_create_for_data(width, height, format,
                                        line_0, stride,
                                        &display->image_cache.base,
                                        &display->image_surfaces, NULL, NULL, NULL);
        surface->context.top_down = TRUE;
        surface->context.canvas_draws_on_surface = TRUE;
        return canvas;
#ifdef USE_OPENGL
    case RED_RENDERER_OGL_PBUF:
        canvas = create_ogl_pbuf_context(display, width, height, stride,
                                         SPICE_SURFACE_FMT_DEPTH(format));
        surface->context.top_down = FALSE;
        return canvas;
    case RED_RENDERER_OGL_PIXMAP:
        canvas = create_ogl_pixmap_context(display, width, height, stride,
                                           SPICE_SURFACE_FMT_DEPTH(format));
        surface->context.top_down = FALSE;
        return canvas;
#endif
    default:
        spice_error("invalid renderer type");
    };

    return NULL;
}

static SurfaceCreateItem *get_surface_create_item(
    RedChannel* channel,
    uint32_t surface_id, uint32_t width,
    uint32_t height, uint32_t format, uint32_t flags)
{
    SurfaceCreateItem *create;

    create = spice_malloc(sizeof(SurfaceCreateItem));

    create->surface_create.surface_id = surface_id;
    create->surface_create.width = width;
    create->surface_create.height = height;
    create->surface_create.flags = flags;
    create->surface_create.format = format;

    red_channel_pipe_item_init(channel,
            &create->pipe_item, PIPE_ITEM_TYPE_CREATE_SURFACE);
    return create;
}

static inline void red_create_surface_item(DisplayChannelClient *dcc, int surface_id)
{
    DisplayChannel *display;
    RedSurface *surface;
    SurfaceCreateItem *create;
    uint32_t flags;

    if (!dcc) {
        return;
    }

    display = DCC_TO_DC(dcc);
    flags = is_primary_surface(DCC_TO_DC(dcc), surface_id) ? SPICE_SURFACE_FLAGS_PRIMARY : 0;

    /* don't send redundant create surface commands to client */
    if (display->common.during_target_migrate ||
        dcc->surface_client_created[surface_id]) {
        return;
    }
    surface = &display->surfaces[surface_id];
    create = get_surface_create_item(RED_CHANNEL_CLIENT(dcc)->channel,
            surface_id, surface->context.width, surface->context.height,
                                     surface->context.format, flags);
    dcc->surface_client_created[surface_id] = TRUE;
    red_channel_client_pipe_add(RED_CHANNEL_CLIENT(dcc), &create->pipe_item);
}

static void red_worker_create_surface_item(DisplayChannel *display, int surface_id)
{
    DisplayChannelClient *dcc;
    RingItem *item, *next;

    FOREACH_DCC(display, item, next, dcc) {
        red_create_surface_item(dcc, surface_id);
    }
}


static void red_worker_push_surface_image(DisplayChannel *display, int surface_id)
{
    DisplayChannelClient *dcc;
    RingItem *item, *next;

    FOREACH_DCC(display, item, next, dcc) {
        red_push_surface_image(dcc, surface_id);
    }
}

static void red_create_surface(DisplayChannel *display, uint32_t surface_id, uint32_t width,
                               uint32_t height, int32_t stride, uint32_t format,
                               void *line_0, int data_is_valid, int send_client)
{
    RedSurface *surface = &display->surfaces[surface_id];
    uint32_t i;

    spice_warn_if(surface->context.canvas);

    surface->context.canvas_draws_on_surface = FALSE;
    surface->context.width = width;
    surface->context.height = height;
    surface->context.format = format;
    surface->context.stride = stride;
    surface->context.line_0 = line_0;
    if (!data_is_valid) {
        char *data = line_0;
        if (stride < 0) {
            data -= abs(stride) * (height - 1);
        }
        memset(data, 0, height*abs(stride));
    }
    surface->create.info = NULL;
    surface->destroy.info = NULL;
    ring_init(&surface->current);
    ring_init(&surface->current_list);
    ring_init(&surface->depend_on_me);
    region_init(&surface->draw_dirty_region);
    surface->refs = 1;
    
    if (display->renderer != RED_RENDERER_INVALID) {
        surface->context.canvas = create_canvas_for_surface(display, surface, display->renderer,
                                                            width, height, stride,
                                                            surface->context.format, line_0);
        if (!surface->context.canvas) {
            spice_critical("drawing canvas creating failed - can`t create same type canvas");
        }
        

        if (send_client) {
            red_worker_create_surface_item(display, surface_id);
            if (data_is_valid) {
                red_worker_push_surface_image(display, surface_id);
            }
        }
        return;
    }

    for (i = 0; i < display->num_renderers; i++) {
        surface->context.canvas = create_canvas_for_surface(display, surface, display->renderers[i],
                                                            width, height, stride,
                                                            surface->context.format, line_0);
        if (surface->context.canvas) { //no need canvas check
            display->renderer = display->renderers[i];
            if (send_client) {
                red_worker_create_surface_item(display, surface_id);
                if (data_is_valid) {
                    red_worker_push_surface_image(display, surface_id);
                }
            }
            return;
        }
    }

    spice_critical("unable to create drawing canvas");
}

static inline void flush_display_commands(RedWorker *worker)
{
    RedChannel *display_red_channel = RED_CHANNEL(worker->display_channel);

    for (;;) {
        uint64_t end_time;
        int ring_is_empty;

        red_process_commands(worker, MAX_PIPE_SIZE, &ring_is_empty);
        if (ring_is_empty) {
            break;
        }

        while (red_process_commands(worker, MAX_PIPE_SIZE, &ring_is_empty)) {
            red_channel_push(RED_CHANNEL(worker->display_channel));
        }

        if (ring_is_empty) {
            break;
        }
        end_time = red_get_monotonic_time() + DISPLAY_CLIENT_TIMEOUT;
        int sleep_count = 0;
        for (;;) {
            red_channel_push(RED_CHANNEL(worker->display_channel));
            if (!display_is_connected(worker) ||
                red_channel_max_pipe_size(display_red_channel) <= MAX_PIPE_SIZE) {
                break;
            }
            RedChannel *channel = (RedChannel *)worker->display_channel;
            red_channel_receive(channel);
            red_channel_send(channel);
            // TODO: MC: the whole timeout will break since it takes lowest timeout, should
            // do it client by client.
            if (red_get_monotonic_time() >= end_time) {
                spice_warning("update timeout");
                red_disconnect_all_display_TODO_remove_me(channel);
            } else {
                sleep_count++;
                usleep(DISPLAY_CLIENT_RETRY_INTERVAL);
            }
        }
    }
}

static inline void flush_cursor_commands(RedWorker *worker)
{
    RedChannel *cursor_red_channel = RED_CHANNEL(worker->cursor_channel);

    for (;;) {
        uint64_t end_time;
        int ring_is_empty = FALSE;

        red_process_cursor(worker, MAX_PIPE_SIZE, &ring_is_empty);
        if (ring_is_empty) {
            break;
        }

        while (red_process_cursor(worker, MAX_PIPE_SIZE, &ring_is_empty)) {
            red_channel_push(RED_CHANNEL(worker->cursor_channel));
        }

        if (ring_is_empty) {
            break;
        }
        end_time = red_get_monotonic_time() + DISPLAY_CLIENT_TIMEOUT;
        int sleep_count = 0;
        for (;;) {
            red_channel_push(RED_CHANNEL(worker->cursor_channel));
            if (!cursor_is_connected(worker)
                || red_channel_min_pipe_size(cursor_red_channel) <= MAX_PIPE_SIZE) {
                break;
            }
            RedChannel *channel = (RedChannel *)worker->cursor_channel;
            red_channel_receive(channel);
            red_channel_send(channel);
            if (red_get_monotonic_time() >= end_time) {
                spice_warning("flush cursor timeout");
                cursor_channel_disconnect(worker->cursor_channel);
                worker->cursor_channel = NULL;
            } else {
                sleep_count++;
                usleep(DISPLAY_CLIENT_RETRY_INTERVAL);
            }
        }
    }
}

// TODO: on timeout, don't disconnect all channels immediatly - try to disconnect the slowest ones
// first and maybe turn timeouts to several timeouts in order to disconnect channels gradually.
// Should use disconnect or shutdown?
static inline void flush_all_qxl_commands(RedWorker *worker)
{
    flush_display_commands(worker);
    flush_cursor_commands(worker);
}

static void push_new_primary_surface(DisplayChannelClient *dcc)
{
    RedChannelClient *rcc = RED_CHANNEL_CLIENT(dcc);

    red_channel_client_pipe_add_type(rcc, PIPE_ITEM_TYPE_INVAL_PALETTE_CACHE);
    red_create_surface_item(dcc, 0);
    red_channel_client_push(rcc);
}

/* TODO: this function is evil^Wsynchronous, fix */
static int display_channel_client_wait_for_init(DisplayChannelClient *dcc)
{
    dcc->expect_init = TRUE;
    uint64_t end_time = red_get_monotonic_time() + DISPLAY_CLIENT_TIMEOUT;
    for (;;) {
        red_channel_client_receive(RED_CHANNEL_CLIENT(dcc));
        if (!red_channel_client_is_connected(RED_CHANNEL_CLIENT(dcc))) {
            break;
        }
        if (dcc->pixmap_cache && dcc->glz_dict) {
            dcc->pixmap_cache_generation = dcc->pixmap_cache->generation;
            /* TODO: move common.id? if it's used for a per client structure.. */
            spice_info("creating encoder with id == %d", dcc->common.id);
            dcc->glz = glz_encoder_create(dcc->common.id, dcc->glz_dict->dict, &dcc->glz_data.usr);
            if (!dcc->glz) {
                spice_critical("create global lz failed");
            }
            return TRUE;
        }
        if (red_get_monotonic_time() > end_time) {
            spice_warning("timeout");
            red_channel_client_disconnect(RED_CHANNEL_CLIENT(dcc));
            break;
        }
        usleep(DISPLAY_CLIENT_RETRY_INTERVAL);
    }
    return FALSE;
}

static void on_new_display_channel_client(DisplayChannelClient *dcc)
{
    DisplayChannel *display = DCC_TO_DC(dcc);
    RedChannelClient *rcc = RED_CHANNEL_CLIENT(dcc);

    red_channel_client_push_set_ack(RED_CHANNEL_CLIENT(dcc));

    if (red_channel_client_waits_for_migrate_data(rcc)) {
        return;
    }

    if (!display_channel_client_wait_for_init(dcc)) {
        return;
    }
    red_channel_client_ack_zero_messages_window(RED_CHANNEL_CLIENT(dcc));
    if (display->surfaces[0].context.canvas) {
        red_current_flush(display, 0);
        push_new_primary_surface(dcc);
        red_push_surface_image(dcc, 0);
        dcc_push_monitors_config(dcc);
        red_pipe_add_verb(rcc, SPICE_MSG_DISPLAY_MARK);
        dcc_allocate_render(dcc, display->renderer, &display->surfaces[0].context, &display->image_cache.base, &display->image_surfaces);
        create_full_screen_stream(display);
    }
}

static GlzSharedDictionary *_red_find_glz_dictionary(RedClient *client, uint8_t dict_id)
{
    RingItem *now;
    GlzSharedDictionary *ret = NULL;

    now = &glz_dictionary_list;
    while ((now = ring_next(&glz_dictionary_list, now))) {
        GlzSharedDictionary *dict = (GlzSharedDictionary *)now;
        if ((dict->client == client) && (dict->id == dict_id)) {
            ret = dict;
            break;
        }
    }

    return ret;
}

static GlzSharedDictionary *_red_create_glz_dictionary(RedClient *client, uint8_t id,
                                                       GlzEncDictContext *opaque_dict)
{
    GlzSharedDictionary *shared_dict = spice_new0(GlzSharedDictionary, 1);
    shared_dict->dict = opaque_dict;
    shared_dict->id = id;
    shared_dict->refs = 1;
    shared_dict->migrate_freeze = FALSE;
    shared_dict->client = client;
    ring_item_init(&shared_dict->base);
    pthread_rwlock_init(&shared_dict->encode_lock, NULL);
    return shared_dict;
}

static GlzSharedDictionary *red_create_glz_dictionary(DisplayChannelClient *dcc,
                                                      uint8_t id, int window_size)
{
    GlzEncDictContext *glz_dict = glz_enc_dictionary_create(window_size,
                                                            MAX_LZ_ENCODERS,
                                                            &dcc->glz_data.usr);
#ifdef COMPRESS_DEBUG
    spice_info("Lz Window %d Size=%d", id, window_size);
#endif
    if (!glz_dict) {
        spice_critical("failed creating lz dictionary");
        return NULL;
    }
    return _red_create_glz_dictionary(RED_CHANNEL_CLIENT(dcc)->client, id, glz_dict);
}

static GlzSharedDictionary *red_create_restored_glz_dictionary(DisplayChannelClient *dcc,
                                                               uint8_t id,
                                                               GlzEncDictRestoreData *restore_data)
{
    GlzEncDictContext *glz_dict = glz_enc_dictionary_restore(restore_data,
                                                             &dcc->glz_data.usr);
    if (!glz_dict) {
        spice_critical("failed creating lz dictionary");
        return NULL;
    }
    return _red_create_glz_dictionary(RED_CHANNEL_CLIENT(dcc)->client, id, glz_dict);
}

static GlzSharedDictionary *red_get_glz_dictionary(DisplayChannelClient *dcc,
                                                   uint8_t id, int window_size)
{
    GlzSharedDictionary *shared_dict = NULL;

    pthread_mutex_lock(&glz_dictionary_list_lock);

    shared_dict = _red_find_glz_dictionary(RED_CHANNEL_CLIENT(dcc)->client, id);

    if (!shared_dict) {
        shared_dict = red_create_glz_dictionary(dcc, id, window_size);
        ring_add(&glz_dictionary_list, &shared_dict->base);
    } else {
        shared_dict->refs++;
    }
    pthread_mutex_unlock(&glz_dictionary_list_lock);
    return shared_dict;
}

static GlzSharedDictionary *red_restore_glz_dictionary(DisplayChannelClient *dcc,
                                                       uint8_t id,
                                                       GlzEncDictRestoreData *restore_data)
{
    GlzSharedDictionary *shared_dict = NULL;

    pthread_mutex_lock(&glz_dictionary_list_lock);

    shared_dict = _red_find_glz_dictionary(RED_CHANNEL_CLIENT(dcc)->client, id);

    if (!shared_dict) {
        shared_dict = red_create_restored_glz_dictionary(dcc, id, restore_data);
        ring_add(&glz_dictionary_list, &shared_dict->base);
    } else {
        shared_dict->refs++;
    }
    pthread_mutex_unlock(&glz_dictionary_list_lock);
    return shared_dict;
}

static void red_freeze_glz(DisplayChannelClient *dcc)
{
    pthread_rwlock_wrlock(&dcc->glz_dict->encode_lock);
    if (!dcc->glz_dict->migrate_freeze) {
        dcc->glz_dict->migrate_freeze = TRUE;
    }
    pthread_rwlock_unlock(&dcc->glz_dict->encode_lock);
}

/* destroy encoder, and dictionary if no one uses it*/
static void red_release_glz(DisplayChannelClient *dcc)
{
    GlzSharedDictionary *shared_dict;

    red_display_client_clear_glz_drawables(dcc);

    glz_encoder_destroy(dcc->glz);
    dcc->glz = NULL;

    if (!(shared_dict = dcc->glz_dict)) {
        return;
    }

    dcc->glz_dict = NULL;
    pthread_mutex_lock(&glz_dictionary_list_lock);
    if (--shared_dict->refs) {
        pthread_mutex_unlock(&glz_dictionary_list_lock);
        return;
    }
    ring_remove(&shared_dict->base);
    pthread_mutex_unlock(&glz_dictionary_list_lock);
    glz_enc_dictionary_destroy(shared_dict->dict, &dcc->glz_data.usr);
    free(shared_dict);
}

static int display_channel_init_cache(DisplayChannelClient *dcc, SpiceMsgcDisplayInit *init_info)
{
    spice_assert(!dcc->pixmap_cache);
    return !!(dcc->pixmap_cache = pixmap_cache_get(RED_CHANNEL_CLIENT(dcc)->client,
                                                   init_info->pixmap_cache_id,
                                                   init_info->pixmap_cache_size));
}

static int display_channel_init_glz_dictionary(DisplayChannelClient *dcc,
                                               SpiceMsgcDisplayInit *init_info)
{
    spice_assert(!dcc->glz_dict);
    ring_init(&dcc->glz_drawables);
    ring_init(&dcc->glz_drawables_inst_to_free);
    pthread_mutex_init(&dcc->glz_drawables_inst_to_free_lock, NULL);
    return !!(dcc->glz_dict = red_get_glz_dictionary(dcc,
                                                     init_info->glz_dictionary_id,
                                                     init_info->glz_dictionary_window_size));
}

static int display_channel_init(DisplayChannelClient *dcc, SpiceMsgcDisplayInit *init_info)
{
    return (display_channel_init_cache(dcc, init_info) &&
            display_channel_init_glz_dictionary(dcc, init_info));
}

static int display_channel_handle_migrate_glz_dictionary(DisplayChannelClient *dcc,
                                                         SpiceMigrateDataDisplay *migrate_info)
{
    spice_assert(!dcc->glz_dict);
    ring_init(&dcc->glz_drawables);
    ring_init(&dcc->glz_drawables_inst_to_free);
    pthread_mutex_init(&dcc->glz_drawables_inst_to_free_lock, NULL);
    return !!(dcc->glz_dict = red_restore_glz_dictionary(dcc,
                                                         migrate_info->glz_dict_id,
                                                         &migrate_info->glz_dict_data));
}

static int display_channel_handle_migrate_mark(RedChannelClient *rcc)
{
    DisplayChannel *display_channel = SPICE_CONTAINEROF(rcc->channel, DisplayChannel, common.base);
    RedChannel *channel = RED_CHANNEL(display_channel);

    red_channel_pipes_add_type(channel, PIPE_ITEM_TYPE_MIGRATE_DATA);
    return TRUE;
}

static uint64_t display_channel_handle_migrate_data_get_serial(
                RedChannelClient *rcc, uint32_t size, void *message)
{
    SpiceMigrateDataDisplay *migrate_data;

    migrate_data = (SpiceMigrateDataDisplay *)((uint8_t *)message + sizeof(SpiceMigrateDataHeader));

    return migrate_data->message_serial;
}

static int display_channel_client_restore_surface(DisplayChannelClient *dcc, uint32_t surface_id)
{
    /* we don't process commands till we receive the migration data, thus,
     * we should have not sent any surface to the client. */
    if (dcc->surface_client_created[surface_id]) {
        spice_warning("surface %u is already marked as client_created", surface_id);
        return FALSE;
    }
    dcc->surface_client_created[surface_id] = TRUE;
    return TRUE;
}

static int display_channel_client_restore_surfaces_lossless(DisplayChannelClient *dcc,
                                                            MigrateDisplaySurfacesAtClientLossless *mig_surfaces)
{
    uint32_t i;

    spice_debug(NULL);
    for (i = 0; i < mig_surfaces->num_surfaces; i++) {
        uint32_t surface_id = mig_surfaces->surfaces[i].id;

        if (!display_channel_client_restore_surface(dcc, surface_id)) {
            return FALSE;
        }
    }
    return TRUE;
}

static int display_channel_client_restore_surfaces_lossy(DisplayChannelClient *dcc,
                                                          MigrateDisplaySurfacesAtClientLossy *mig_surfaces)
{
    uint32_t i;

    spice_debug(NULL);
    for (i = 0; i < mig_surfaces->num_surfaces; i++) {
        uint32_t surface_id = mig_surfaces->surfaces[i].id;
        SpiceMigrateDataRect *mig_lossy_rect;
        SpiceRect lossy_rect;

        if (!display_channel_client_restore_surface(dcc, surface_id)) {
            return FALSE;
        }
        spice_assert(dcc->surface_client_created[surface_id]);

        mig_lossy_rect = &mig_surfaces->surfaces[i].lossy_rect;
        lossy_rect.left = mig_lossy_rect->left;
        lossy_rect.top = mig_lossy_rect->top;
        lossy_rect.right = mig_lossy_rect->right;
        lossy_rect.bottom = mig_lossy_rect->bottom;
        region_init(&dcc->surface_client_lossy_region[surface_id]);
        region_add(&dcc->surface_client_lossy_region[surface_id], &lossy_rect);
    }
    return TRUE;
}
static int display_channel_handle_migrate_data(RedChannelClient *rcc, uint32_t size,
                                               void *message)
{
    SpiceMigrateDataHeader *header;
    SpiceMigrateDataDisplay *migrate_data;
    DisplayChannel *display_channel = SPICE_CONTAINEROF(rcc->channel, DisplayChannel, common.base);
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);
    uint8_t *surfaces;
    int surfaces_restored = FALSE;
    int i;

    spice_debug(NULL);
    if (size < sizeof(*migrate_data) + sizeof(SpiceMigrateDataHeader)) {
        spice_error("bad message size");
        return FALSE;
    }
    header = (SpiceMigrateDataHeader *)message;
    migrate_data = (SpiceMigrateDataDisplay *)(header + 1);
    if (!migration_protocol_validate_header(header,
                                            SPICE_MIGRATE_DATA_DISPLAY_MAGIC,
                                            SPICE_MIGRATE_DATA_DISPLAY_VERSION)) {
        spice_error("bad header");
        return FALSE;
    }
    /* size is set to -1 in order to keep the cache frozen until the original
     * channel client that froze the cache on the src size receives the migrate
     * data and unfreezes the cache by setting its size > 0 and by triggering
     * pixmap_cache_reset */
    dcc->pixmap_cache = pixmap_cache_get(RED_CHANNEL_CLIENT(dcc)->client,
                                         migrate_data->pixmap_cache_id, -1);
    if (!dcc->pixmap_cache) {
        return FALSE;
    }
    pthread_mutex_lock(&dcc->pixmap_cache->lock);
    for (i = 0; i < MAX_CACHE_CLIENTS; i++) {
        dcc->pixmap_cache->sync[i] = MAX(dcc->pixmap_cache->sync[i],
                                         migrate_data->pixmap_cache_clients[i]);
    }
    pthread_mutex_unlock(&dcc->pixmap_cache->lock);

    if (migrate_data->pixmap_cache_freezer) {
        /* activating the cache. The cache will start to be active after
         * pixmap_cache_reset is called, when handling PIPE_ITEM_TYPE_PIXMAP_RESET */
        dcc->pixmap_cache->size = migrate_data->pixmap_cache_size;
        red_channel_client_pipe_add_type(rcc,
                                         PIPE_ITEM_TYPE_PIXMAP_RESET);
    }

    if (display_channel_handle_migrate_glz_dictionary(dcc, migrate_data)) {
        dcc->glz = glz_encoder_create(dcc->common.id,
                                      dcc->glz_dict->dict, &dcc->glz_data.usr);
        if (!dcc->glz) {
            spice_critical("create global lz failed");
        }
    } else {
        spice_critical("restoring global lz dictionary failed");
    }

    dcc->common.is_low_bandwidth = migrate_data->low_bandwidth_setting;

    if (migrate_data->low_bandwidth_setting) {
        red_channel_client_ack_set_client_window(rcc, WIDE_CLIENT_ACK_WINDOW);
        if (dcc->jpeg_state == SPICE_WAN_COMPRESSION_AUTO) {
            display_channel->enable_jpeg = TRUE;
        }
        if (dcc->zlib_glz_state == SPICE_WAN_COMPRESSION_AUTO) {
            display_channel->enable_zlib_glz_wrap = TRUE;
        }
    }

    surfaces = (uint8_t *)message + migrate_data->surfaces_at_client_ptr;
    if (display_channel->enable_jpeg) {
        surfaces_restored = display_channel_client_restore_surfaces_lossy(dcc,
                                (MigrateDisplaySurfacesAtClientLossy *)surfaces);
    } else {
        surfaces_restored = display_channel_client_restore_surfaces_lossless(dcc,
                                (MigrateDisplaySurfacesAtClientLossless*)surfaces);
    }

    if (!surfaces_restored) {
        return FALSE;
    }
    red_channel_client_pipe_add_type(rcc, PIPE_ITEM_TYPE_INVAL_PALETTE_CACHE);
    /* enable sending messages */
    red_channel_client_ack_zero_messages_window(rcc);
    return TRUE;
}

static int display_channel_handle_stream_report(DisplayChannelClient *dcc,
                                                SpiceMsgcDisplayStreamReport *stream_report)
{
    StreamAgent *stream_agent;

    if (stream_report->stream_id >= NUM_STREAMS) {
        spice_warning("stream_report: invalid stream id %u", stream_report->stream_id);
        return FALSE;
    }
    stream_agent = &dcc->stream_agents[stream_report->stream_id];
    if (!stream_agent->video_encoder) {
        spice_info("stream_report: no encoder for stream id %u."
                    "Probably the stream has been destroyed", stream_report->stream_id);
        return TRUE;
    }

    if (stream_report->unique_id != stream_agent->report_id) {
        spice_warning("local reoprt-id (%u) != msg report-id (%u)",
                      stream_agent->report_id, stream_report->unique_id);
        return TRUE;
    }
    stream_agent->video_encoder->client_stream_report(stream_agent->video_encoder,
                                                      stream_report->num_frames,
                                                      stream_report->num_drops,
                                                      stream_report->start_frame_mm_time,
                                                      stream_report->end_frame_mm_time,
                                                      stream_report->last_frame_delay,
                                                      stream_report->audio_delay);
    return TRUE;
}

static int display_channel_handle_preferred_compression(DisplayChannelClient *dcc,
        SpiceMsgcDisplayPreferredCompression *pc) {
    DisplayChannel *display_channel = DCC_TO_DC(dcc);
    switch (pc->image_compression) {
    case SPICE_IMAGE_COMPRESSION_AUTO_LZ:
    case SPICE_IMAGE_COMPRESSION_AUTO_GLZ:
    case SPICE_IMAGE_COMPRESSION_QUIC:
#ifdef USE_LZ4
    case SPICE_IMAGE_COMPRESSION_LZ4:
#endif
    case SPICE_IMAGE_COMPRESSION_LZ:
    case SPICE_IMAGE_COMPRESSION_GLZ:
    case SPICE_IMAGE_COMPRESSION_OFF:
        display_channel->common.worker->image_compression = pc->image_compression;
        dcc->image_compression = pc->image_compression;
        return TRUE;
    default:
        spice_warning("preferred-compression: unsupported image compression setting");
        return FALSE;
    }
}

static int display_channel_handle_message(RedChannelClient *rcc, uint32_t size, uint16_t type,
                                          void *message)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);

    switch (type) {
    case SPICE_MSGC_DISPLAY_INIT:
        if (!dcc->expect_init) {
            spice_warning("unexpected SPICE_MSGC_DISPLAY_INIT");
            return FALSE;
        }
        dcc->expect_init = FALSE;
        return display_channel_init(dcc, (SpiceMsgcDisplayInit *)message);
    case SPICE_MSGC_DISPLAY_STREAM_REPORT:
        return display_channel_handle_stream_report(dcc,
                                                    (SpiceMsgcDisplayStreamReport *)message);
    case SPICE_MSGC_DISPLAY_PREFERRED_COMPRESSION:
        return display_channel_handle_preferred_compression(dcc,
            (SpiceMsgcDisplayPreferredCompression *)message);

    default:
        return red_channel_client_handle_message(rcc, size, type, message);
    }
}

static int common_channel_config_socket(RedChannelClient *rcc)
{
    RedClient *client = red_channel_client_get_client(rcc);
    MainChannelClient *mcc = red_client_get_main(client);
    RedsStream *stream = red_channel_client_get_stream(rcc);
    CommonChannelClient *ccc = COMMON_CHANNEL_CLIENT(rcc);
    int flags;
    int delay_val;

    if ((flags = fcntl(stream->socket, F_GETFL)) == -1) {
        spice_warning("accept failed, %s", strerror(errno));
        return FALSE;
    }

    if (fcntl(stream->socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        spice_warning("accept failed, %s", strerror(errno));
        return FALSE;
    }

    // TODO - this should be dynamic, not one time at channel creation
    ccc->is_low_bandwidth = main_channel_client_is_low_bandwidth(mcc);
    delay_val = ccc->is_low_bandwidth ? 0 : 1;
    /* FIXME: Using Nagle's Algorithm can lead to apparent delays, depending
     * on the delayed ack timeout on the other side.
     * Instead of using Nagle's, we need to implement message buffering on
     * the application level.
     * see: http://www.stuartcheshire.org/papers/NagleDelayedAck/
     */
    if (setsockopt(stream->socket, IPPROTO_TCP, TCP_NODELAY, &delay_val,
                   sizeof(delay_val)) == -1) {
        if (errno != ENOTSUP) {
            spice_warning("setsockopt failed, %s", strerror(errno));
        }
    }
    return TRUE;
}

static void worker_watch_update_mask(SpiceWatch *watch, int event_mask)
{
    struct RedWorker *worker;
    int i;

    if (!watch) {
        return;
    }

    worker = watch->worker;
    i = watch - worker->watches;

    worker->poll_fds[i].events = 0;
    if (event_mask & SPICE_WATCH_EVENT_READ) {
        worker->poll_fds[i].events |= POLLIN;
    }
    if (event_mask & SPICE_WATCH_EVENT_WRITE) {
        worker->poll_fds[i].events |= POLLOUT;
    }
}

static SpiceWatch *worker_watch_add(int fd, int event_mask, SpiceWatchFunc func, void *opaque)
{
    /* Since we are a channel core implementation, we always get called from
       red_channel_client_create(), so opaque always is our rcc */
    RedChannelClient *rcc = opaque;
    struct RedWorker *worker;
    int i;

    /* Since we are called from red_channel_client_create()
       CommonChannelClient->worker has not been set yet! */
    worker = SPICE_CONTAINEROF(rcc->channel, CommonChannel, base)->worker;

    /* Search for a free slot in our poll_fds & watches arrays */
    for (i = 0; i < MAX_EVENT_SOURCES; i++) {
        if (worker->poll_fds[i].fd == -1) {
            break;
        }
    }
    if (i == MAX_EVENT_SOURCES) {
        spice_warning("could not add a watch for channel type %u id %u",
                      rcc->channel->type, rcc->channel->id);
        return NULL;
    }

    worker->poll_fds[i].fd = fd;
    worker->watches[i].worker = worker;
    worker->watches[i].watch_func = func;
    worker->watches[i].watch_func_opaque = opaque;
    worker_watch_update_mask(&worker->watches[i], event_mask);

    return &worker->watches[i];
}

static void worker_watch_remove(SpiceWatch *watch)
{
    if (!watch) {
        return;
    }

    /* Note we don't touch the poll_fd here, to avoid the
       poll_fds/watches table entry getting re-used in the same
       red_worker_main loop over the fds as it is removed.

       This is done because re-using it while events were pending on
       the fd previously occupying the slot would lead to incorrectly
       calling the watch_func for the new fd. */
    memset(watch, 0, sizeof(SpiceWatch));
}

SpiceCoreInterface worker_core = {
    .timer_add = spice_timer_queue_add,
    .timer_start = spice_timer_set,
    .timer_cancel = spice_timer_cancel,
    .timer_remove = spice_timer_remove,

    .watch_update_mask = worker_watch_update_mask,
    .watch_add = worker_watch_add,
    .watch_remove = worker_watch_remove,
};

CommonChannelClient *common_channel_new_client(CommonChannel *common,
                                               int size,
                                               RedClient *client,
                                               RedsStream *stream,
                                               int mig_target,
                                               int monitor_latency,
                                               uint32_t *common_caps,
                                               int num_common_caps,
                                               uint32_t *caps,
                                               int num_caps)
{
    RedChannelClient *rcc =
        red_channel_client_create(size, &common->base, client, stream, monitor_latency,
                                  num_common_caps, common_caps, num_caps, caps);
    if (!rcc) {
        return NULL;
    }
    CommonChannelClient *common_cc = (CommonChannelClient*)rcc;
    common_cc->worker = common->worker;
    common_cc->id = common->worker->qxl->id;
    common->during_target_migrate = mig_target;

    // TODO: move wide/narrow ack setting to red_channel.
    red_channel_client_ack_set_client_window(rcc,
        common_cc->is_low_bandwidth ?
        WIDE_CLIENT_ACK_WINDOW : NARROW_CLIENT_ACK_WINDOW);
    return common_cc;
}


RedChannel *red_worker_new_channel(RedWorker *worker, int size,
                                   const char *name,
                                   uint32_t channel_type, int migration_flags,
                                   ChannelCbs *channel_cbs,
                                   channel_handle_parsed_proc handle_parsed)
{
    RedChannel *channel = NULL;
    CommonChannel *common;

    spice_return_val_if_fail(worker, NULL);
    spice_return_val_if_fail(channel_cbs, NULL);
    spice_return_val_if_fail(!channel_cbs->config_socket, NULL);
    spice_return_val_if_fail(!channel_cbs->alloc_recv_buf, NULL);
    spice_return_val_if_fail(!channel_cbs->release_recv_buf, NULL);

    channel_cbs->config_socket = common_channel_config_socket;
    channel_cbs->alloc_recv_buf = common_alloc_recv_buf;
    channel_cbs->release_recv_buf = common_release_recv_buf;

    channel = red_channel_create_parser(size, &worker_core,
                                        channel_type, worker->qxl->id,
                                        TRUE /* handle_acks */,
                                        spice_get_client_channel_parser(channel_type, NULL),
                                        handle_parsed,
                                        channel_cbs,
                                        migration_flags);
    spice_return_val_if_fail(channel, NULL);
    red_channel_set_stat_node(channel, stat_add_node(worker->stat, name, TRUE));

    common = (CommonChannel *)channel;
    common->worker = worker;
    return channel;
}

static void display_channel_hold_pipe_item(RedChannelClient *rcc, PipeItem *item)
{
    spice_assert(item);
    switch (item->type) {
    case PIPE_ITEM_TYPE_DRAW:
        drawable_pipe_item_ref(SPICE_CONTAINEROF(item, DrawablePipeItem, dpi_pipe_item));
        break;
    case PIPE_ITEM_TYPE_STREAM_CLIP:
        ((StreamClipItem *)item)->refs++;
        break;
    case PIPE_ITEM_TYPE_UPGRADE:
        ((UpgradeItem *)item)->refs++;
        break;
    case PIPE_ITEM_TYPE_IMAGE:
        ((ImageItem *)item)->refs++;
        break;
    default:
        spice_critical("invalid item type");
    }
}

static void display_channel_client_release_item_after_push(DisplayChannelClient *dcc,
                                                           PipeItem *item)
{
    DisplayChannel *display = DCC_TO_DC(dcc);

    switch (item->type) {
    case PIPE_ITEM_TYPE_DRAW:
        drawable_pipe_item_unref(SPICE_CONTAINEROF(item, DrawablePipeItem, dpi_pipe_item));
        break;
    case PIPE_ITEM_TYPE_STREAM_CLIP:
        display_stream_clip_unref(display, (StreamClipItem *)item);
        break;
    case PIPE_ITEM_TYPE_UPGRADE:
        upgrade_item_unref(display, (UpgradeItem *)item);
        break;
    case PIPE_ITEM_TYPE_IMAGE:
        release_image_item((ImageItem *)item);
        break;
    case PIPE_ITEM_TYPE_VERB:
        free(item);
        break;
    case PIPE_ITEM_TYPE_MONITORS_CONFIG: {
        MonitorsConfigItem *monconf_item = SPICE_CONTAINEROF(item,
                                                             MonitorsConfigItem, pipe_item);
        monitors_config_unref(monconf_item->monitors_config);
        free(item);
        break;
    }
    default:
        spice_critical("invalid item type");
    }
}

// TODO: share code between before/after_push since most of the items need the same
// release
static void display_channel_client_release_item_before_push(DisplayChannelClient *dcc,
                                                            PipeItem *item)
{
    DisplayChannel *display = DCC_TO_DC(dcc);

    switch (item->type) {
    case PIPE_ITEM_TYPE_DRAW: {
        DrawablePipeItem *dpi = SPICE_CONTAINEROF(item, DrawablePipeItem, dpi_pipe_item);
        ring_remove(&dpi->base);
        drawable_pipe_item_unref(dpi);
        break;
    }
    case PIPE_ITEM_TYPE_STREAM_CREATE: {
        StreamAgent *agent = SPICE_CONTAINEROF(item, StreamAgent, create_item);
        stream_agent_unref(display, agent);
        break;
    }
    case PIPE_ITEM_TYPE_STREAM_CLIP:
        display_stream_clip_unref(display, (StreamClipItem *)item);
        break;
    case PIPE_ITEM_TYPE_STREAM_DESTROY: {
        StreamAgent *agent = SPICE_CONTAINEROF(item, StreamAgent, destroy_item);
        stream_agent_unref(display, agent);
        break;
    }
    case PIPE_ITEM_TYPE_UPGRADE:
        upgrade_item_unref(display, (UpgradeItem *)item);
        break;
    case PIPE_ITEM_TYPE_IMAGE:
        release_image_item((ImageItem *)item);
        break;
    case PIPE_ITEM_TYPE_CREATE_SURFACE: {
        SurfaceCreateItem *surface_create = SPICE_CONTAINEROF(item, SurfaceCreateItem,
                                                              pipe_item);
        free(surface_create);
        break;
    }
    case PIPE_ITEM_TYPE_DESTROY_SURFACE: {
        SurfaceDestroyItem *surface_destroy = SPICE_CONTAINEROF(item, SurfaceDestroyItem,
                                                                pipe_item);
        free(surface_destroy);
        break;
    }
    case PIPE_ITEM_TYPE_MONITORS_CONFIG: {
        MonitorsConfigItem *monconf_item = SPICE_CONTAINEROF(item,
                                                             MonitorsConfigItem, pipe_item);
        monitors_config_unref(monconf_item->monitors_config);
        free(item);
        break;
    }
    case PIPE_ITEM_TYPE_INVAL_ONE:
    case PIPE_ITEM_TYPE_VERB:
    case PIPE_ITEM_TYPE_MIGRATE_DATA:
    case PIPE_ITEM_TYPE_PIXMAP_SYNC:
    case PIPE_ITEM_TYPE_PIXMAP_RESET:
    case PIPE_ITEM_TYPE_INVAL_PALETTE_CACHE:
    case PIPE_ITEM_TYPE_STREAM_ACTIVATE_REPORT:
        free(item);
        break;
    default:
        spice_critical("invalid item type");
    }
}

static void display_channel_release_item(RedChannelClient *rcc, PipeItem *item, int item_pushed)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);

    spice_assert(item);
    if (item_pushed) {
        display_channel_client_release_item_after_push(dcc, item);
    } else {
        spice_debug("not pushed (%d)", item->type);
        display_channel_client_release_item_before_push(dcc, item);
    }
}

static void display_channel_create(RedWorker *worker, int migrate, int stream_video,
                                   uint32_t n_surfaces)
{
    DisplayChannel *display_channel;
    ChannelCbs cbs = {
        .on_disconnect = display_channel_client_on_disconnect,
        .send_item = display_channel_send_item,
        .hold_item = display_channel_hold_pipe_item,
        .release_item = display_channel_release_item,
        .handle_migrate_flush_mark = display_channel_handle_migrate_mark,
        .handle_migrate_data = display_channel_handle_migrate_data,
        .handle_migrate_data_get_serial = display_channel_handle_migrate_data_get_serial
    };

    spice_return_if_fail(num_renderers > 0);

    spice_info("create display channel");
    if (!(display_channel = (DisplayChannel *)red_worker_new_channel(
            worker, sizeof(*display_channel), "display_channel",
            SPICE_CHANNEL_DISPLAY,
            SPICE_MIGRATE_NEED_FLUSH | SPICE_MIGRATE_NEED_DATA_TRANSFER,
            &cbs, display_channel_handle_message))) {
        spice_warning("failed to create display channel");
        return;
    }
    worker->display_channel = display_channel;
    stat_init(&display_channel->add_stat, "add", worker->clockid);
    stat_init(&display_channel->exclude_stat, "exclude", worker->clockid);
    stat_init(&display_channel->__exclude_stat, "__exclude", worker->clockid);
#ifdef RED_STATISTICS
    RedChannel *channel = RED_CHANNEL(display_channel);
    display_channel->cache_hits_counter = stat_add_counter(channel->stat,
                                                           "cache_hits", TRUE);
    display_channel->add_to_cache_counter = stat_add_counter(channel->stat,
                                                             "add_to_cache", TRUE);
    display_channel->non_cache_counter = stat_add_counter(channel->stat,
                                                          "non_cache", TRUE);
#endif
    stat_compress_init(&display_channel->lz_stat, "lz");
    stat_compress_init(&display_channel->glz_stat, "glz");
    stat_compress_init(&display_channel->quic_stat, "quic");
    stat_compress_init(&display_channel->jpeg_stat, "jpeg");
    stat_compress_init(&display_channel->zlib_glz_stat, "zlib");
    stat_compress_init(&display_channel->jpeg_alpha_stat, "jpeg_alpha");
    stat_compress_init(&display_channel->lz4_stat, "lz4");

    display_channel->n_surfaces = n_surfaces;
    display_channel->num_renderers = num_renderers;
    memcpy(display_channel->renderers, renderers, sizeof(display_channel->renderers));
    display_channel->renderer = RED_RENDERER_INVALID;

    ring_init(&display_channel->current_list);
    image_surface_init(display_channel);
    drawables_init(display_channel);
    image_cache_init(&display_channel->image_cache);
    display_channel->stream_video = stream_video;
    display_channel_init_streams(display_channel);
}

static void guest_set_client_capabilities(RedWorker *worker)
{
    int i;
    DisplayChannelClient *dcc;
    RedChannelClient *rcc;
    RingItem *link, *next;
    uint8_t caps[SPICE_CAPABILITIES_SIZE] = { 0 };
    int caps_available[] = {
        SPICE_DISPLAY_CAP_SIZED_STREAM,
        SPICE_DISPLAY_CAP_MONITORS_CONFIG,
        SPICE_DISPLAY_CAP_COMPOSITE,
        SPICE_DISPLAY_CAP_A8_SURFACE,
    };

    if (worker->qxl->st->qif->base.major_version < 3 ||
        (worker->qxl->st->qif->base.major_version == 3 &&
        worker->qxl->st->qif->base.minor_version < 2) ||
        !worker->qxl->st->qif->set_client_capabilities) {
        return;
    }
#define SET_CAP(a,c)                                                    \
        ((a)[(c) / 8] |= (1 << ((c) % 8)))

#define CLEAR_CAP(a,c)                                                  \
        ((a)[(c) / 8] &= ~(1 << ((c) % 8)))

    if (!worker->running) {
        return;
    }
    if ((worker->display_channel == NULL) ||
        (RED_CHANNEL(worker->display_channel)->clients_num == 0)) {
        worker->qxl->st->qif->set_client_capabilities(worker->qxl, FALSE, caps);
    } else {
        // Take least common denominator
        for (i = 0 ; i < sizeof(caps_available) / sizeof(caps_available[0]); ++i) {
            SET_CAP(caps, caps_available[i]);
        }
        DCC_FOREACH_SAFE(link, next, dcc, RED_CHANNEL(worker->display_channel)) {
            rcc = (RedChannelClient *)dcc;
            for (i = 0 ; i < sizeof(caps_available) / sizeof(caps_available[0]); ++i) {
                if (!red_channel_client_test_remote_cap(rcc, caps_available[i]))
                    CLEAR_CAP(caps, caps_available[i]);
            }
        }
        worker->qxl->st->qif->set_client_capabilities(worker->qxl, TRUE, caps);
    }
}

static void handle_new_display_channel(RedWorker *worker, RedClient *client, RedsStream *stream,
                                       int migrate,
                                       uint32_t *common_caps, int num_common_caps,
                                       uint32_t *caps, int num_caps)
{
    DisplayChannel *display_channel;
    DisplayChannelClient *dcc;

    if (!worker->display_channel) {
        spice_warning("Display channel was not created");
        return;
    }
    display_channel = worker->display_channel;
    spice_info("add display channel client");
    dcc = dcc_new(display_channel, client, stream, migrate,
                  common_caps, num_common_caps, caps, num_caps,
                  worker->image_compression, worker->jpeg_state, worker->zlib_glz_state);
    if (!dcc) {
        return;
    }
    spice_info("New display (client %p) dcc %p stream %p", client, dcc, stream);
    red_display_init_glz_data(dcc);

    dcc->send_data.free_list.res =
        spice_malloc(sizeof(SpiceResourceList) +
                     DISPLAY_FREE_LIST_DEFAULT_SIZE * sizeof(SpiceResourceID));
    dcc->send_data.free_list.res_size = DISPLAY_FREE_LIST_DEFAULT_SIZE;

    if (dcc->jpeg_state == SPICE_WAN_COMPRESSION_AUTO) {
        display_channel->enable_jpeg = dcc->common.is_low_bandwidth;
    } else {
        display_channel->enable_jpeg = (dcc->jpeg_state == SPICE_WAN_COMPRESSION_ALWAYS);
    }

    // todo: tune quality according to bandwidth
    display_channel->jpeg_quality = 85;

    if (dcc->zlib_glz_state == SPICE_WAN_COMPRESSION_AUTO) {
        display_channel->enable_zlib_glz_wrap = dcc->common.is_low_bandwidth;
    } else {
        display_channel->enable_zlib_glz_wrap = (dcc->zlib_glz_state ==
                                                 SPICE_WAN_COMPRESSION_ALWAYS);
    }

    spice_info("jpeg %s", display_channel->enable_jpeg ? "enabled" : "disabled");
    spice_info("zlib-over-glz %s", display_channel->enable_zlib_glz_wrap ? "enabled" : "disabled");
    	
    //enable full video
    display_channel_set_stream_video(display_channel, SPICE_STREAM_VIDEO_ALL);
    
    //invoke set video codec
    red_dispatcher_on_vc_change();

    guest_set_client_capabilities(worker);

    // todo: tune level according to bandwidth
    display_channel->zlib_level = ZLIB_DEFAULT_COMPRESSION_LEVEL;
    dcc_init_stream_agents(dcc);
    on_new_display_channel_client(dcc);
}

static void red_connect_cursor(RedWorker *worker, RedClient *client, RedsStream *stream,
                               int migrate,
                               uint32_t *common_caps, int num_common_caps,
                               uint32_t *caps, int num_caps)
{
    CursorChannel *channel;
    CursorChannelClient *ccc;

    spice_return_if_fail(worker->cursor_channel != NULL);

    channel = worker->cursor_channel;
    spice_info("add cursor channel client");
    ccc = cursor_channel_client_new(channel, client, stream,
                                    migrate,
                                    common_caps, num_common_caps,
                                    caps, num_caps);
    spice_return_if_fail(ccc != NULL);

    RedChannelClient *rcc = RED_CHANNEL_CLIENT(ccc);
    red_channel_client_ack_zero_messages_window(rcc);
    red_channel_client_push_set_ack(rcc);

    // TODO: why do we check for context.canvas? defer this to after display cc is connected
    // and test it's canvas? this is just a test to see if there is an active renderer?
    if (display_channel_surface_has_canvas(worker->display_channel, 0))
        cursor_channel_init(channel, ccc);
}

static void surface_dirty_region_to_rects(RedSurface *surface,
                                          QXLRect *qxl_dirty_rects,
                                          uint32_t num_dirty_rects)
{
    QRegion *surface_dirty_region;
    SpiceRect *dirty_rects;
    int i;

    surface_dirty_region = &surface->draw_dirty_region;
    dirty_rects = spice_new0(SpiceRect, num_dirty_rects);
    region_ret_rects(surface_dirty_region, dirty_rects, num_dirty_rects);
    for (i = 0; i < num_dirty_rects; i++) {
        qxl_dirty_rects[i].top    = dirty_rects[i].top;
        qxl_dirty_rects[i].left   = dirty_rects[i].left;
        qxl_dirty_rects[i].bottom = dirty_rects[i].bottom;
        qxl_dirty_rects[i].right  = dirty_rects[i].right;
    }
    free(dirty_rects);
}

void display_channel_update(DisplayChannel *display,
                            uint32_t surface_id, const QXLRect *area, uint32_t clear_dirty,
                            QXLRect **qxl_dirty_rects, uint32_t *num_dirty_rects)
{
    SpiceRect rect;
    RedSurface *surface;

    spice_return_if_fail(validate_surface(display, surface_id));

    red_get_rect_ptr(&rect, area);
    red_update_area(display, &rect, surface_id);

    surface = &display->surfaces[surface_id];
    if (!*qxl_dirty_rects) {
        *num_dirty_rects = pixman_region32_n_rects(&surface->draw_dirty_region);
        *qxl_dirty_rects = spice_new0(QXLRect, *num_dirty_rects);
    }

    surface_dirty_region_to_rects(surface, *qxl_dirty_rects, *num_dirty_rects);
    if (clear_dirty)
        region_clear(&surface->draw_dirty_region);
}

static void handle_dev_update_async(void *opaque, void *payload)
{
    RedWorker *worker = opaque;
    RedWorkerMessageUpdateAsync *msg = payload;
    QXLRect *qxl_dirty_rects = NULL;
    uint32_t num_dirty_rects = 0;

    spice_return_if_fail(worker->running);
    spice_return_if_fail(worker->qxl->st->qif->update_area_complete);

    flush_display_commands(worker);
    display_channel_update(worker->display_channel,
                           msg->surface_id, &msg->qxl_area, msg->clear_dirty_region,
                           &qxl_dirty_rects, &num_dirty_rects);

    worker->qxl->st->qif->update_area_complete(worker->qxl, msg->surface_id,
                                                qxl_dirty_rects, num_dirty_rects);
    free(qxl_dirty_rects);
}

static void handle_dev_update(void *opaque, void *payload)
{
    RedWorker *worker = opaque;
    RedWorkerMessageUpdate *msg = payload;

    spice_return_if_fail(worker->running);

    flush_display_commands(worker);
    display_channel_update(worker->display_channel,
                           msg->surface_id, msg->qxl_area, msg->clear_dirty_region,
                           &msg->qxl_dirty_rects, &msg->num_dirty_rects);
}

static void handle_dev_del_memslot(void *opaque, void *payload)
{
    RedWorker *worker = opaque;
    RedWorkerMessageDelMemslot *msg = payload;
    uint32_t slot_id = msg->slot_id;
    uint32_t slot_group_id = msg->slot_group_id;

    red_memslot_info_del_slot(&worker->mem_slots, slot_group_id, slot_id);
}

void display_channel_destroy_surface_wait(DisplayChannel *display, int surface_id)
{
    VALIDATE_SURFACE_RET(display, surface_id);
    if (!display->surfaces[surface_id].context.canvas)
        return;

    red_handle_depends_on_target_surface(display, surface_id);
    /* note that red_handle_depends_on_target_surface must be called before current_remove_all.
       otherwise "current" will hold items that other drawables may depend on, and then
       current_remove_all will remove them from the pipe. */
    current_remove_all(display, surface_id);
    red_clear_surface_drawables_from_pipes(display, surface_id, TRUE);
}

static void handle_dev_destroy_surface_wait(void *opaque, void *payload)
{
    RedWorkerMessageDestroySurfaceWait *msg = payload;
    RedWorker *worker = opaque;

    spice_return_if_fail(msg->surface_id == 0);

    flush_all_qxl_commands(worker);
    display_channel_destroy_surface_wait(worker->display_channel, msg->surface_id);
}

/* called upon device reset */

/* TODO: split me*/
void display_channel_destroy_surfaces(DisplayChannel *display)
{
    int i;

    spice_debug(NULL);
    //to handle better
    for (i = 0; i < NUM_SURFACES; ++i) {
        if (display->surfaces[i].context.canvas) {
            display_channel_destroy_surface_wait(display, i);
            if (display->surfaces[i].context.canvas) {
                display_channel_surface_unref(display, i);
            }
            spice_assert(!display->surfaces[i].context.canvas);
        }
    }
    spice_warn_if_fail(ring_is_empty(&display->streams));

    if (red_channel_is_connected(RED_CHANNEL(display))) {
        red_channel_pipes_add_type(RED_CHANNEL(display), PIPE_ITEM_TYPE_INVAL_PALETTE_CACHE);
        red_pipes_add_verb(RED_CHANNEL(display), SPICE_MSG_DISPLAY_STREAM_DESTROY_ALL);
    }

    red_display_clear_glz_drawables(display);
}

static void handle_dev_destroy_surfaces(void *opaque, void *payload)
{
    RedWorker *worker = opaque;

    flush_all_qxl_commands(worker);
    display_channel_destroy_surfaces(worker->display_channel);
    cursor_channel_reset(worker->cursor_channel);
}

static void display_update_monitors_config(DisplayChannel *display,
                                           QXLMonitorsConfig *config,
                                           uint16_t count, uint16_t max_allowed)
{

    if (display->monitors_config)
        monitors_config_unref(display->monitors_config);

    display->monitors_config =
        monitors_config_new(config->heads, count, max_allowed);
}

static void red_worker_push_monitors_config(RedWorker *worker)
{
    DisplayChannelClient *dcc;
    RingItem *item, *next;

    FOREACH_DCC(worker->display_channel, item, next, dcc) {
        dcc_push_monitors_config(dcc);
    }
}

static void set_monitors_config_to_primary(DisplayChannel *display)
{
    DrawContext *context = &display->surfaces[0].context;
    QXLHead head = { 0, };

    spice_return_if_fail(display->surfaces[0].context.canvas);

    if (display->monitors_config)
        monitors_config_unref(display->monitors_config);

    head.width = context->width;
    head.height = context->height;
    display->monitors_config = monitors_config_new(&head, 1, 1);
}

static void dev_create_primary_surface(RedWorker *worker, uint32_t surface_id,
                                       QXLDevSurfaceCreate surface)
{
    DisplayChannel *display = worker->display_channel;
    uint8_t *line_0;
    int error;

    spice_debug(NULL);
    spice_warn_if(surface_id != 0);
    spice_warn_if(surface.height == 0);
    spice_warn_if(((uint64_t)abs(surface.stride) * (uint64_t)surface.height) !=
             abs(surface.stride) * surface.height);

    line_0 = (uint8_t*)get_virt(&worker->mem_slots, surface.mem,
                                surface.height * abs(surface.stride),
                                surface.group_id, &error);
    if (error) {
        return;
    }
    if (worker->record_fd) {
        red_record_dev_input_primary_surface_create(worker->record_fd,
                    &surface, line_0);
    }

    if (surface.stride < 0) {
        line_0 -= (int32_t)(surface.stride * (surface.height -1));
    }

    red_create_surface(display, 0, surface.width, surface.height, surface.stride, surface.format,
                       line_0, surface.flags & QXL_SURF_FLAG_KEEP_DATA, TRUE);
    
    spice_info("primary surface created, %d x %d ", surface.width, surface.height);
    
    set_monitors_config_to_primary(display);

    if (display_is_connected(worker) && !worker->display_channel->common.during_target_migrate) {
        /* guest created primary, so it will (hopefully) send a monitors_config
         * now, don't send our own temporary one */
        if (!worker->driver_cap_monitors_config) {
            red_worker_push_monitors_config(worker);
        }
        red_pipes_add_verb(&worker->display_channel->common.base,
                           SPICE_MSG_DISPLAY_MARK);
        red_channel_push(&worker->display_channel->common.base);
        detach_and_stop_streams(worker->display_channel);
        //reallocate for all display client
        resize_all_client_render(display, display->renderer, &display->surfaces[0].context);
        create_full_screen_stream(display);
    }

    cursor_channel_init(worker->cursor_channel, NULL);
}

static void handle_dev_create_primary_surface(void *opaque, void *payload)
{
    RedWorkerMessageCreatePrimarySurface *msg = payload;
    RedWorker *worker = opaque;

    dev_create_primary_surface(worker, msg->surface_id, msg->surface);
}

static void destroy_primary_surface(RedWorker *worker, uint32_t surface_id)
{
    DisplayChannel *display = worker->display_channel;

    VALIDATE_SURFACE_RET(display, surface_id);
    spice_warn_if(surface_id != 0);

    spice_debug(NULL);
    if (!display->surfaces[surface_id].context.canvas) {
        spice_warning("double destroy of primary surface");
        return;
    }

    flush_all_qxl_commands(worker);
    display_channel_destroy_surface_wait(display, 0);
    display_channel_surface_unref(display, 0);

    spice_warn_if_fail(ring_is_empty(&display->streams));
    spice_warn_if_fail(!display->surfaces[surface_id].context.canvas);

    cursor_channel_reset(worker->cursor_channel);
}

static void handle_dev_destroy_primary_surface(void *opaque, void *payload)
{
    RedWorkerMessageDestroyPrimarySurface *msg = payload;
    RedWorker *worker = opaque;
    uint32_t surface_id = msg->surface_id;

    destroy_primary_surface(worker, surface_id);
}

static void handle_dev_destroy_primary_surface_async(void *opaque, void *payload)
{
    RedWorkerMessageDestroyPrimarySurfaceAsync *msg = payload;
    RedWorker *worker = opaque;
    uint32_t surface_id = msg->surface_id;

    destroy_primary_surface(worker, surface_id);
}

static void flush_all_surfaces(DisplayChannel *display)
{
    int x;

    for (x = 0; x < NUM_SURFACES; ++x) {
        if (display->surfaces[x].context.canvas) {
            red_current_flush(display, x);
        }
    }
}

static void dev_flush_surfaces(RedWorker *worker)
{
    flush_all_qxl_commands(worker);
    flush_all_surfaces(worker->display_channel);
}

static void handle_dev_flush_surfaces_async(void *opaque, void *payload)
{
    RedWorker *worker = opaque;

    dev_flush_surfaces(worker);
}

static void handle_dev_stop(void *opaque, void *payload)
{
    RedWorker *worker = opaque;

    spice_info("stop");
    spice_assert(worker->running);
    worker->running = FALSE;
    red_display_clear_glz_drawables(worker->display_channel);
    flush_all_surfaces(worker->display_channel);
    /* todo: when the waiting is expected to take long (slow connection and
     * overloaded pipe), don't wait, and in case of migration,
     * purge the pipe, send destroy_all_surfaces
     * to the client (there is no such message right now), and start
     * from scratch on the destination side */
    if (!red_channel_wait_all_sent(RED_CHANNEL(worker->display_channel),
                                   DISPLAY_CLIENT_TIMEOUT)) {
        red_channel_apply_clients(RED_CHANNEL(worker->display_channel),
                                 red_channel_client_disconnect_if_pending_send);
    }
    if (!red_channel_wait_all_sent(RED_CHANNEL(worker->cursor_channel),
                                   DISPLAY_CLIENT_TIMEOUT)) {
        red_channel_apply_clients(RED_CHANNEL(worker->cursor_channel),
                                 red_channel_client_disconnect_if_pending_send);
    }
}

static int display_channel_wait_for_migrate_data(DisplayChannel *display)
{
    uint64_t end_time = red_get_monotonic_time() + DISPLAY_CLIENT_MIGRATE_DATA_TIMEOUT;
    RedChannel *channel = &display->common.base;
    RedChannelClient *rcc;

    spice_debug(NULL);
    spice_assert(channel->clients_num == 1);

    rcc = SPICE_CONTAINEROF(ring_get_head(&channel->clients), RedChannelClient, channel_link);
    spice_assert(red_channel_client_waits_for_migrate_data(rcc));

    for (;;) {
        red_channel_client_receive(rcc);
        if (!red_channel_client_is_connected(rcc)) {
            break;
        }

        if (!red_channel_client_waits_for_migrate_data(rcc)) {
            return TRUE;
        }
        if (red_get_monotonic_time() > end_time) {
            spice_warning("timeout");
            red_channel_client_disconnect(rcc);
            break;
        }
        usleep(DISPLAY_CLIENT_RETRY_INTERVAL);
    }
    return FALSE;
}

static void handle_dev_start(void *opaque, void *payload)
{
    RedWorker *worker = opaque;

    spice_assert(!worker->running);
    if (worker->cursor_channel) {
        COMMON_CHANNEL(worker->cursor_channel)->during_target_migrate = FALSE;
    }
    if (worker->display_channel) {
        worker->display_channel->common.during_target_migrate = FALSE;
        if (red_channel_waits_for_migrate_data(&worker->display_channel->common.base)) {
            display_channel_wait_for_migrate_data(worker->display_channel);
        }
    }
    worker->running = TRUE;
    guest_set_client_capabilities(worker);
}

static void handle_dev_wakeup(void *opaque, void *payload)
{
    RedWorker *worker = opaque;

    stat_inc_counter(worker->wakeup_counter, 1);
    red_dispatcher_clear_pending(worker->red_dispatcher, RED_DISPATCHER_PENDING_WAKEUP);
}

static void handle_dev_oom(void *opaque, void *payload)
{
    RedWorker *worker = opaque;
    DisplayChannel *display = worker->display_channel;

    RedChannel *display_red_channel = &worker->display_channel->common.base;
    int ring_is_empty;

    spice_assert(worker->running);
    // streams? but without streams also leak
    spice_debug("OOM1 #draw=%u, #red_draw=%u, #glz_draw=%u current %u pipes %u",
                display->drawable_count,
                worker->red_drawable_count,
                worker->glz_drawable_count,
                display->current_size,
                worker->display_channel ?
                red_channel_sum_pipes_size(display_red_channel) : 0);
    while (red_process_commands(worker, MAX_PIPE_SIZE, &ring_is_empty)) {
        red_channel_push(&worker->display_channel->common.base);
    }
    if (worker->qxl->st->qif->flush_resources(worker->qxl) == 0) {
        red_free_some(worker);
        worker->qxl->st->qif->flush_resources(worker->qxl);
    }
    spice_debug("OOM2 #draw=%u, #red_draw=%u, #glz_draw=%u current %u pipes %u",
                display->drawable_count,
                worker->red_drawable_count,
                worker->glz_drawable_count,
                display->current_size,
                worker->display_channel ?
                red_channel_sum_pipes_size(display_red_channel) : 0);
    red_dispatcher_clear_pending(worker->red_dispatcher, RED_DISPATCHER_PENDING_OOM);
}

static void handle_dev_reset_cursor(void *opaque, void *payload)
{
    RedWorker *worker = opaque;

    cursor_channel_reset(worker->cursor_channel);
}

static void handle_dev_reset_image_cache(void *opaque, void *payload)
{
    RedWorker *worker = opaque;
    DisplayChannel *display = worker->display_channel;

    image_cache_reset(&display->image_cache);
}

static void handle_dev_destroy_surface_wait_async(void *opaque, void *payload)
{
    RedWorkerMessageDestroySurfaceWaitAsync *msg = payload;
    RedWorker *worker = opaque;

    display_channel_destroy_surface_wait(worker->display_channel, msg->surface_id);
}

static void handle_dev_destroy_surfaces_async(void *opaque, void *payload)
{
    RedWorker *worker = opaque;

    flush_all_qxl_commands(worker);
    display_channel_destroy_surfaces(worker->display_channel);
    cursor_channel_reset(worker->cursor_channel);
}

static void handle_dev_create_primary_surface_async(void *opaque, void *payload)
{
    RedWorkerMessageCreatePrimarySurfaceAsync *msg = payload;
    RedWorker *worker = opaque;

    dev_create_primary_surface(worker, msg->surface_id, msg->surface);
}

static void handle_dev_display_connect(void *opaque, void *payload)
{
    RedWorkerMessageDisplayConnect *msg = payload;
    RedWorker *worker = opaque;
    RedsStream *stream = msg->stream;
    RedClient *client = msg->client;
    int migration = msg->migration;

    spice_info("connect");
    handle_new_display_channel(worker, client, stream, migration,
                               msg->common_caps, msg->num_common_caps,
                               msg->caps, msg->num_caps);
    free(msg->caps);
    free(msg->common_caps);
}

static void handle_dev_display_disconnect(void *opaque, void *payload)
{
    RedWorkerMessageDisplayDisconnect *msg = payload;
    RedChannelClient *rcc = msg->rcc;
    RedWorker *worker = opaque;

    spice_info("disconnect display client");
    spice_assert(rcc);

    guest_set_client_capabilities(worker);

    red_channel_client_disconnect(rcc);
}

static void handle_dev_display_migrate(void *opaque, void *payload)
{
    RedWorkerMessageDisplayMigrate *msg = payload;
    RedWorker *worker = opaque;

    RedChannelClient *rcc = msg->rcc;
    spice_info("migrate display client");
    spice_assert(rcc);
    red_migrate_display(worker->display_channel, rcc);
}

static inline uint32_t qxl_monitors_config_size(uint32_t heads)
{
    return sizeof(QXLMonitorsConfig) + sizeof(QXLHead) * heads;
}

static void handle_dev_monitors_config_async(void *opaque, void *payload)
{
    RedWorkerMessageMonitorsConfigAsync *msg = payload;
    RedWorker *worker = opaque;
    int error;
    uint16_t count, max_allowed;
    QXLMonitorsConfig *dev_monitors_config =
        (QXLMonitorsConfig*)get_virt(&worker->mem_slots, msg->monitors_config,
                                     qxl_monitors_config_size(1),
                                     msg->group_id, &error);

    if (error) {
        /* TODO: raise guest bug (requires added QXL interface) */
        return;
    }
    worker->driver_cap_monitors_config = 1;
    count = dev_monitors_config->count;
    max_allowed = dev_monitors_config->max_allowed;
    if (count == 0) {
        spice_warning("ignoring an empty monitors config message from driver");
        return;
    }
    if (count > max_allowed) {
        spice_warning("ignoring malformed monitors_config from driver, "
                      "count > max_allowed %d > %d",
                      count,
                      max_allowed);
        return;
    }
    /* get pointer again to check virtual size */
    dev_monitors_config =
        (QXLMonitorsConfig*)get_virt(&worker->mem_slots, msg->monitors_config,
                                     qxl_monitors_config_size(count),
                                     msg->group_id, &error);
    if (error) {
        /* TODO: raise guest bug (requires added QXL interface) */
        return;
    }
    display_update_monitors_config(worker->display_channel, dev_monitors_config,
                                   MIN(count, msg->max_monitors),
                                   MIN(max_allowed, msg->max_monitors));
    red_worker_push_monitors_config(worker);
}

/* TODO: special, perhaps use another dispatcher? */
static void handle_dev_cursor_connect(void *opaque, void *payload)
{
    RedWorkerMessageCursorConnect *msg = payload;
    RedWorker *worker = opaque;
    RedsStream *stream = msg->stream;
    RedClient *client = msg->client;
    int migration = msg->migration;

    spice_info("cursor connect");
    red_connect_cursor(worker, client, stream, migration,
                       msg->common_caps, msg->num_common_caps,
                       msg->caps, msg->num_caps);
    free(msg->caps);
    free(msg->common_caps);
}

static void handle_dev_cursor_disconnect(void *opaque, void *payload)
{
    RedWorkerMessageCursorDisconnect *msg = payload;
    RedChannelClient *rcc = msg->rcc;

    spice_info("disconnect cursor client");
    spice_return_if_fail(rcc);
    red_channel_client_disconnect(rcc);
}

static void handle_dev_cursor_migrate(void *opaque, void *payload)
{
    RedWorkerMessageCursorMigrate *msg = payload;
    RedChannelClient *rcc = msg->rcc;

    spice_info("migrate cursor client");
    cursor_channel_client_migrate(CURSOR_CHANNEL_CLIENT(rcc));
}

static void handle_dev_set_compression(void *opaque, void *payload)
{
    RedWorkerMessageSetCompression *msg = payload;
    RedWorker *worker = opaque;

    worker->image_compression = msg->image_compression;
    switch (worker->image_compression) {
    case SPICE_IMAGE_COMPRESSION_AUTO_LZ:
        spice_info("ic auto_lz");
        break;
    case SPICE_IMAGE_COMPRESSION_AUTO_GLZ:
        spice_info("ic auto_glz");
        break;
    case SPICE_IMAGE_COMPRESSION_QUIC:
        spice_info("ic quic");
        break;
#ifdef USE_LZ4
    case SPICE_IMAGE_COMPRESSION_LZ4:
        spice_info("ic lz4");
        break;
#endif
    case SPICE_IMAGE_COMPRESSION_LZ:
        spice_info("ic lz");
        break;
    case SPICE_IMAGE_COMPRESSION_GLZ:
        spice_info("ic glz");
        break;
    case SPICE_IMAGE_COMPRESSION_OFF:
        spice_info("ic off");
        break;
    default:
        spice_warning("ic invalid");
    }

    display_channel_compress_stats_print(worker->display_channel);
    display_channel_compress_stats_reset(worker->display_channel);
}

static void handle_dev_set_streaming_video(void *opaque, void *payload)
{
    RedWorkerMessageSetStreamingVideo *msg = payload;
    RedWorker *worker = opaque;

    display_channel_set_stream_video(worker->display_channel, msg->streaming_video);
}

void handle_dev_set_video_codecs(void *opaque, void *payload)
{
    RedWorkerMessageSetVideoCodecs *msg = payload;
    RedWorker *worker = opaque;

    display_channel_set_video_codecs(worker->display_channel,
                                     msg->num_video_codecs, msg->video_codecs);
}

static void handle_dev_set_mouse_mode(void *opaque, void *payload)
{
    RedWorkerMessageSetMouseMode *msg = payload;
    RedWorker *worker = opaque;

    spice_info("mouse mode %u", msg->mode);
    cursor_channel_set_mouse_mode(worker->cursor_channel, msg->mode);
}

static void dev_add_memslot(RedWorker *worker, QXLDevMemSlot mem_slot)
{
    red_memslot_info_add_slot(&worker->mem_slots, mem_slot.slot_group_id, mem_slot.slot_id,
                              mem_slot.addr_delta, mem_slot.virt_start, mem_slot.virt_end,
                              mem_slot.generation);
}

static void handle_dev_add_memslot(void *opaque, void *payload)
{
    RedWorker *worker = opaque;
    RedWorkerMessageAddMemslot *msg = payload;
    QXLDevMemSlot mem_slot = msg->mem_slot;

    red_memslot_info_add_slot(&worker->mem_slots, mem_slot.slot_group_id, mem_slot.slot_id,
                              mem_slot.addr_delta, mem_slot.virt_start, mem_slot.virt_end,
                              mem_slot.generation);
}

static void handle_dev_add_memslot_async(void *opaque, void *payload)
{
    RedWorkerMessageAddMemslotAsync *msg = payload;
    RedWorker *worker = opaque;

    dev_add_memslot(worker, msg->mem_slot);
}

static void handle_dev_reset_memslots(void *opaque, void *payload)
{
    RedWorker *worker = opaque;

    red_memslot_info_reset(&worker->mem_slots);
}

static void handle_dev_driver_unload(void *opaque, void *payload)
{
    RedWorker *worker = opaque;

    worker->driver_cap_monitors_config = 0;
}

static int loadvm_command(RedWorker *worker, QXLCommandExt *ext)
{
    RedCursorCmd *cursor_cmd;
    RedSurfaceCmd *surface_cmd;

    switch (ext->cmd.type) {
    case QXL_CMD_CURSOR:
        cursor_cmd = spice_new0(RedCursorCmd, 1);
        if (red_get_cursor_cmd(&worker->mem_slots, ext->group_id, cursor_cmd, ext->cmd.data)) {
            free(cursor_cmd);
            return FALSE;
        }
        cursor_channel_process_cmd(worker->cursor_channel, cursor_cmd, ext->group_id);
        break;
    case QXL_CMD_SURFACE:
        surface_cmd = spice_new0(RedSurfaceCmd, 1);
        if (red_get_surface_cmd(&worker->mem_slots, ext->group_id, surface_cmd, ext->cmd.data)) {
            free(surface_cmd);
            return FALSE;
        }
        red_process_surface(worker, surface_cmd, ext->group_id, TRUE);
        break;
    default:
        spice_warning("unhandled loadvm command type (%d)", ext->cmd.type);
    }

    return TRUE;
}

static void handle_dev_loadvm_commands(void *opaque, void *payload)
{
    RedWorkerMessageLoadvmCommands *msg = payload;
    RedWorker *worker = opaque;
    uint32_t i;
    uint32_t count = msg->count;
    QXLCommandExt *ext = msg->ext;

    spice_info("loadvm_commands");
    for (i = 0 ; i < count ; ++i) {
        if (!loadvm_command(worker, &ext[i])) {
            /* XXX allow failure in loadvm? */
            spice_warning("failed loadvm command type (%d)", ext[i].cmd.type);
        }
    }
}

static void worker_handle_dispatcher_async_done(void *opaque,
                                                uint32_t message_type,
                                                void *payload)
{
    RedWorker *worker = opaque;
    RedWorkerMessageAsync *msg_async = payload;

    spice_debug(NULL);
    red_dispatcher_async_complete(worker->red_dispatcher, msg_async->cmd);
}

static void worker_dispatcher_record(void *opaque, uint32_t message_type, void *payload)
{
    RedWorker *worker = opaque;

    red_record_event(worker->record_fd, 1, message_type, stat_now(worker->clockid));
}

static void register_callbacks(Dispatcher *dispatcher)
{
    dispatcher_register_async_done_callback(
                                    dispatcher,
                                    worker_handle_dispatcher_async_done);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_DISPLAY_CONNECT,
                                handle_dev_display_connect,
                                sizeof(RedWorkerMessageDisplayConnect),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_DISPLAY_DISCONNECT,
                                handle_dev_display_disconnect,
                                sizeof(RedWorkerMessageDisplayDisconnect),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_DISPLAY_MIGRATE,
                                handle_dev_display_migrate,
                                sizeof(RedWorkerMessageDisplayMigrate),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_CURSOR_CONNECT,
                                handle_dev_cursor_connect,
                                sizeof(RedWorkerMessageCursorConnect),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_CURSOR_DISCONNECT,
                                handle_dev_cursor_disconnect,
                                sizeof(RedWorkerMessageCursorDisconnect),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_CURSOR_MIGRATE,
                                handle_dev_cursor_migrate,
                                sizeof(RedWorkerMessageCursorMigrate),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_UPDATE,
                                handle_dev_update,
                                sizeof(RedWorkerMessageUpdate),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_UPDATE_ASYNC,
                                handle_dev_update_async,
                                sizeof(RedWorkerMessageUpdateAsync),
                                DISPATCHER_ASYNC);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_ADD_MEMSLOT,
                                handle_dev_add_memslot,
                                sizeof(RedWorkerMessageAddMemslot),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_ADD_MEMSLOT_ASYNC,
                                handle_dev_add_memslot_async,
                                sizeof(RedWorkerMessageAddMemslotAsync),
                                DISPATCHER_ASYNC);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_DEL_MEMSLOT,
                                handle_dev_del_memslot,
                                sizeof(RedWorkerMessageDelMemslot),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_DESTROY_SURFACES,
                                handle_dev_destroy_surfaces,
                                sizeof(RedWorkerMessageDestroySurfaces),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_DESTROY_SURFACES_ASYNC,
                                handle_dev_destroy_surfaces_async,
                                sizeof(RedWorkerMessageDestroySurfacesAsync),
                                DISPATCHER_ASYNC);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_DESTROY_PRIMARY_SURFACE,
                                handle_dev_destroy_primary_surface,
                                sizeof(RedWorkerMessageDestroyPrimarySurface),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_DESTROY_PRIMARY_SURFACE_ASYNC,
                                handle_dev_destroy_primary_surface_async,
                                sizeof(RedWorkerMessageDestroyPrimarySurfaceAsync),
                                DISPATCHER_ASYNC);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_CREATE_PRIMARY_SURFACE_ASYNC,
                                handle_dev_create_primary_surface_async,
                                sizeof(RedWorkerMessageCreatePrimarySurfaceAsync),
                                DISPATCHER_ASYNC);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_CREATE_PRIMARY_SURFACE,
                                handle_dev_create_primary_surface,
                                sizeof(RedWorkerMessageCreatePrimarySurface),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_RESET_IMAGE_CACHE,
                                handle_dev_reset_image_cache,
                                sizeof(RedWorkerMessageResetImageCache),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_RESET_CURSOR,
                                handle_dev_reset_cursor,
                                sizeof(RedWorkerMessageResetCursor),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_WAKEUP,
                                handle_dev_wakeup,
                                sizeof(RedWorkerMessageWakeup),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_OOM,
                                handle_dev_oom,
                                sizeof(RedWorkerMessageOom),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_START,
                                handle_dev_start,
                                sizeof(RedWorkerMessageStart),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_FLUSH_SURFACES_ASYNC,
                                handle_dev_flush_surfaces_async,
                                sizeof(RedWorkerMessageFlushSurfacesAsync),
                                DISPATCHER_ASYNC);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_STOP,
                                handle_dev_stop,
                                sizeof(RedWorkerMessageStop),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_LOADVM_COMMANDS,
                                handle_dev_loadvm_commands,
                                sizeof(RedWorkerMessageLoadvmCommands),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_SET_COMPRESSION,
                                handle_dev_set_compression,
                                sizeof(RedWorkerMessageSetCompression),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_SET_STREAMING_VIDEO,
                                handle_dev_set_streaming_video,
                                sizeof(RedWorkerMessageSetStreamingVideo),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_SET_VIDEO_CODECS,
                                handle_dev_set_video_codecs,
                                sizeof(RedWorkerMessageSetVideoCodecs),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_SET_MOUSE_MODE,
                                handle_dev_set_mouse_mode,
                                sizeof(RedWorkerMessageSetMouseMode),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_DESTROY_SURFACE_WAIT,
                                handle_dev_destroy_surface_wait,
                                sizeof(RedWorkerMessageDestroySurfaceWait),
                                DISPATCHER_ACK);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_DESTROY_SURFACE_WAIT_ASYNC,
                                handle_dev_destroy_surface_wait_async,
                                sizeof(RedWorkerMessageDestroySurfaceWaitAsync),
                                DISPATCHER_ASYNC);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_RESET_MEMSLOTS,
                                handle_dev_reset_memslots,
                                sizeof(RedWorkerMessageResetMemslots),
                                DISPATCHER_NONE);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_MONITORS_CONFIG_ASYNC,
                                handle_dev_monitors_config_async,
                                sizeof(RedWorkerMessageMonitorsConfigAsync),
                                DISPATCHER_ASYNC);
    dispatcher_register_handler(dispatcher,
                                RED_WORKER_MESSAGE_DRIVER_UNLOAD,
                                handle_dev_driver_unload,
                                sizeof(RedWorkerMessageDriverUnload),
                                DISPATCHER_NONE);
}



static void handle_dev_input(int fd, int event, void *opaque)
{
    RedWorker *worker = opaque;

    dispatcher_handle_recv_read(red_dispatcher_get_dispatcher(worker->red_dispatcher));
}

RedWorker* red_worker_new(QXLInstance *qxl, RedDispatcher *red_dispatcher)
{
    QXLDevInitInfo init_info;
    RedWorker *worker;
    Dispatcher *dispatcher;
    int i;
    const char *record_filename;

    qxl->st->qif->get_init_info(qxl, &init_info);

    worker = spice_new0(RedWorker, 1);

    record_filename = getenv("SPICE_WORKER_RECORD_FILENAME");
    if (record_filename) {
        static const char header[] = "SPICE_REPLAY 1\n";

        worker->record_fd = fopen(record_filename, "w+");
        if (worker->record_fd == NULL) {
            spice_error("failed to open recording file %s\n", record_filename);
        }
        if (fwrite(header, sizeof(header)-1, 1, worker->record_fd) != 1) {
            spice_error("failed to write replay header");
        }
    }
    dispatcher = red_dispatcher_get_dispatcher(red_dispatcher);
    dispatcher_set_opaque(dispatcher, worker);

    worker->red_dispatcher = red_dispatcher;
    worker->qxl = qxl;
    register_callbacks(dispatcher);
    if (worker->record_fd) {
        dispatcher_register_universal_handler(dispatcher, worker_dispatcher_record);
    }
    worker->image_compression = image_compression;
    worker->jpeg_state = jpeg_state;
    worker->zlib_glz_state = zlib_glz_state;
    worker->driver_cap_monitors_config = 0;
#ifdef RED_STATISTICS
    char worker_str[20];
    sprintf(worker_str, "display[%d]", worker->qxl->id);
    worker->stat = stat_add_node(INVALID_STAT_REF, worker_str, TRUE);
    worker->wakeup_counter = stat_add_counter(worker->stat, "wakeups", TRUE);
    worker->command_counter = stat_add_counter(worker->stat, "commands", TRUE);
#endif
    for (i = 0; i < MAX_EVENT_SOURCES; i++) {
        worker->poll_fds[i].fd = -1;
    }

    worker->poll_fds[0].fd = dispatcher_get_recv_fd(dispatcher);
    worker->poll_fds[0].events = POLLIN;
    worker->watches[0].worker = worker;
    worker->watches[0].watch_func = handle_dev_input;
    worker->watches[0].watch_func_opaque = worker;

    red_memslot_info_init(&worker->mem_slots,
                          init_info.num_memslots_groups,
                          init_info.num_memslots,
                          init_info.memslot_gen_bits,
                          init_info.memslot_id_bits,
                          init_info.internal_groupslot_id);

    spice_warn_if(init_info.n_surfaces > NUM_SURFACES);

    red_init_quic(worker);
    red_init_lz(worker);
    red_init_jpeg(worker);
#ifdef USE_LZ4
    red_init_lz4(worker);
#endif
    red_init_zlib(worker);
    worker->event_timeout = INF_EVENT_WAIT;

    worker->cursor_channel = cursor_channel_new(worker);
    // TODO: handle seemless migration. Temp, setting migrate to FALSE
    display_channel_create(worker, FALSE, streaming_video, init_info.n_surfaces);

    return worker;
}

static void red_display_cc_free_glz_drawables(RedChannelClient *rcc)
{
    DisplayChannelClient *dcc = RCC_TO_DCC(rcc);

    red_display_handle_glz_drawables_to_free(dcc);
}

SPICE_GNUC_NORETURN static void *red_worker_main(void *arg)
{
    RedWorker *worker = arg;

    spice_info("begin");
    spice_assert(MAX_PIPE_SIZE > WIDE_CLIENT_ACK_WINDOW &&
           MAX_PIPE_SIZE > NARROW_CLIENT_ACK_WINDOW); //ensure wakeup by ack message

    if (!spice_timer_queue_create()) {
        spice_error("failed to create timer queue");
    }

    if (pthread_getcpuclockid(pthread_self(), &worker->clockid)) {
        spice_warning("getcpuclockid failed");
    }

    RED_CHANNEL(worker->cursor_channel)->thread_id = pthread_self();
    RED_CHANNEL(worker->display_channel)->thread_id = pthread_self();

    for (;;) {
        int i, num_events;
        unsigned int timeout;

        timeout = spice_timer_queue_get_timeout_ms();
        worker->event_timeout = MIN(timeout, worker->event_timeout);
        timeout = display_channel_get_streams_timeout(worker->display_channel);
        worker->event_timeout = MIN(timeout, worker->event_timeout);
        num_events = poll(worker->poll_fds, MAX_EVENT_SOURCES, worker->event_timeout);
        //disable timeout check by akumas 2015.11.27
        //display_channel_streams_timeout(worker->display_channel);
        spice_timer_queue_cb();

        if (worker->display_channel) {
            /* during migration, in the dest, the display channel can be initialized
               while the global lz data not since migrate data msg hasn't been
               received yet */
            red_channel_apply_clients(&worker->display_channel->common.base,
                                      red_display_cc_free_glz_drawables);
        }

        worker->event_timeout = INF_EVENT_WAIT;
        if (num_events == -1) {
            if (errno != EINTR) {
                spice_error("poll failed, %s", strerror(errno));
            }
        }

        for (i = 0; i < MAX_EVENT_SOURCES; i++) {
            /* The watch may have been removed by the watch-func from
               another fd (ie a disconnect through the dispatcher),
               in this case watch_func is NULL. */
            if (worker->poll_fds[i].revents && worker->watches[i].watch_func) {
                int events = 0;
                if (worker->poll_fds[i].revents & POLLIN) {
                    events |= SPICE_WATCH_EVENT_READ;
                }
                if (worker->poll_fds[i].revents & POLLOUT) {
                    events |= SPICE_WATCH_EVENT_WRITE;
                }
                worker->watches[i].watch_func(worker->poll_fds[i].fd, events,
                                        worker->watches[i].watch_func_opaque);
            }
        }

        /* Clear the poll_fd for any removed watches, see the comment in
           watch_remove for why we don't do this there. */
        for (i = 0; i < MAX_EVENT_SOURCES; i++) {
            if (!worker->watches[i].watch_func) {
                worker->poll_fds[i].fd = -1;
            }
        }

        if (worker->running) {
            int ring_is_empty;
            red_process_cursor(worker, MAX_PIPE_SIZE, &ring_is_empty);
            red_process_commands(worker, MAX_PIPE_SIZE, &ring_is_empty);
        }
        red_push(worker);
    }

    spice_warn_if_reached();
}

bool red_worker_run(RedWorker *worker)
{
    sigset_t thread_sig_mask;
    sigset_t curr_sig_mask;
    int r;

    spice_return_val_if_fail(worker, FALSE);
    spice_return_val_if_fail(!worker->thread, FALSE);

    sigfillset(&thread_sig_mask);
    sigdelset(&thread_sig_mask, SIGILL);
    sigdelset(&thread_sig_mask, SIGFPE);
    sigdelset(&thread_sig_mask, SIGSEGV);
    pthread_sigmask(SIG_SETMASK, &thread_sig_mask, &curr_sig_mask);
    if ((r = pthread_create(&worker->thread, NULL, red_worker_main, worker))) {
        spice_error("create thread failed %d", r);
    }
    pthread_sigmask(SIG_SETMASK, &curr_sig_mask, NULL);

    return r == 0;
}

RedChannel* red_worker_get_cursor_channel(RedWorker *worker)
{
    spice_return_val_if_fail(worker, NULL);

    return RED_CHANNEL(worker->cursor_channel);
}

RedChannel* red_worker_get_display_channel(RedWorker *worker)
{
    spice_return_val_if_fail(worker, NULL);

    return RED_CHANNEL(worker->display_channel);
}

clockid_t red_worker_get_clockid(RedWorker *worker)
{
    spice_return_val_if_fail(worker, 0);

    return worker->clockid;
}
