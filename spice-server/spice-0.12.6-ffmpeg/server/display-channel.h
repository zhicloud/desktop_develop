/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
#ifndef DISPLAY_CHANNEL_H_
# define DISPLAY_CHANNEL_H_

#include <setjmp.h>

#include "common/rect.h"
#include "red_worker.h"
#include "reds_stream.h"
#include "cache-item.h"
#include "pixmap-cache.h"
#ifdef USE_OPENGL
#include "common/ogl_ctx.h"
#include "reds_gl_canvas.h"
#endif /* USE_OPENGL */
#include "reds_sw_canvas.h"
#include "glz_encoder_dictionary.h"
#include "glz_encoder.h"
#include "stat.h"
#include "reds.h"
#include "video_encoder.h"
#include "red_memslots.h"
#include "red_parse_qxl.h"
#include "red_record_qxl.h"
#include "jpeg_encoder.h"
#ifdef USE_LZ4
#include "lz4_encoder.h"
#endif
#include "demarshallers.h"
#include "zlib_encoder.h"
#include "red_channel.h"
#include "red_dispatcher.h"
#include "dispatcher.h"
#include "main_channel.h"
#include "migration_protocol.h"
#include "main_dispatcher.h"
#include "spice_server_utils.h"
#include "spice_bitmap_utils.h"
#include "spice_image_cache.h"
#include "utils.h"
#include "tree.h"
#include "stream.h"

#define PALETTE_CACHE_HASH_SHIFT 8
#define PALETTE_CACHE_HASH_SIZE (1 << PALETTE_CACHE_HASH_SHIFT)
#define PALETTE_CACHE_HASH_MASK (PALETTE_CACHE_HASH_SIZE - 1)
#define PALETTE_CACHE_HASH_KEY(id) ((id) & PALETTE_CACHE_HASH_MASK)

#define CLIENT_PALETTE_CACHE_SIZE 128

/* Each drawable can refer to at most 3 images: src, brush and mask */
#define MAX_DRAWABLE_PIXMAP_CACHE_ITEMS 3

#define NUM_STREAMS 50
#define NUM_SURFACES 10000

#define RED_COMPRESS_BUF_SIZE (1024 * 64)
typedef struct RedCompressBuf RedCompressBuf;
struct RedCompressBuf {
    /* This buffer provide space for compression algorithms.
     * Some algorithms access the buffer as an array of 32 bit words
     * so is defined to make sure is always aligned that way.
     */
    union {
        uint8_t  bytes[RED_COMPRESS_BUF_SIZE];
        uint32_t words[RED_COMPRESS_BUF_SIZE / 4];
    } buf;
    RedCompressBuf *send_next;
};

typedef struct WaitForChannels {
    SpiceMsgWaitForChannels header;
    SpiceWaitForChannel buf[MAX_CACHE_CLIENTS];
} WaitForChannels;

typedef struct FreeList {
    int res_size;
    SpiceResourceList *res;
    uint64_t sync[MAX_CACHE_CLIENTS];
    WaitForChannels wait;
} FreeList;

typedef struct GlzSharedDictionary {
    RingItem base;
    GlzEncDictContext *dict;
    uint32_t refs;
    uint8_t id;
    pthread_rwlock_t encode_lock;
    int migrate_freeze;
    RedClient *client; // channel clients of the same client share the dict
} GlzSharedDictionary;

typedef struct  {
    DisplayChannelClient *dcc;
    RedCompressBuf *bufs_head;
    RedCompressBuf *bufs_tail;
    jmp_buf jmp_env;
    union {
        struct {
            SpiceChunks *chunks;
            int next;
            int stride;
            int reverse;
        } lines_data;
        struct {
            RedCompressBuf* next;
            int size_left;
        } compressed_data; // for encoding data that was already compressed by another method
    } u;
    char message_buf[512];
} EncoderData;

typedef struct {
    GlzEncoderUsrContext usr;
    EncoderData data;
} GlzData;

typedef struct DependItem {
    Drawable *drawable;
    RingItem ring_item;
} DependItem;

struct Drawable {
    uint8_t refs;
    RingItem surface_list_link;
    RingItem list_link;
    DrawItem tree_item;
    Ring pipes;
    PipeItem *pipe_item_rest;
    uint32_t size_pipe_item_rest;
    RedDrawable *red_drawable;

    Ring glz_ring;

    red_time_t creation_time;
    red_time_t first_frame_time;
    int frames_count;
    int gradual_frames_count;
    int last_gradual_frame;
    Stream *stream;
    Stream *sized_stream;
    int streamable;
    BitmapGradualType copy_bitmap_graduality;
    uint32_t group_id;
    DependItem depend_items[3];

    int surface_id;
    int surface_deps[3];

    uint32_t process_commands_generation;
};

#define LINK_TO_DPI(ptr) SPICE_CONTAINEROF((ptr), DrawablePipeItem, base)
#define DRAWABLE_FOREACH_DPI_SAFE(drawable, link, next, dpi)            \
    SAFE_FOREACH(link, next, drawable,  &(drawable)->pipes, dpi, LINK_TO_DPI(link))


struct DisplayChannelClient {
    CommonChannelClient common;
    SpiceImageCompression image_compression;
    spice_wan_compression_t jpeg_state;
    spice_wan_compression_t zlib_glz_state;

    int expect_init;

    PixmapCache *pixmap_cache;
    uint32_t pixmap_cache_generation;
    int pending_pixmaps_sync;

    CacheItem *palette_cache[PALETTE_CACHE_HASH_SIZE];
    Ring palette_cache_lru;
    long palette_cache_available;
    uint32_t palette_cache_items;

    struct {
        FreeList free_list;
        uint64_t pixmap_cache_items[MAX_DRAWABLE_PIXMAP_CACHE_ITEMS];
        int num_pixmap_cache_items;
    } send_data;

    /* global lz encoding entities */
    GlzSharedDictionary *glz_dict;
    GlzEncoderContext   *glz;
    GlzData glz_data;

    Ring glz_drawables;               // all the living lz drawable, ordered by encoding time
    Ring glz_drawables_inst_to_free;               // list of instances to be freed
    pthread_mutex_t glz_drawables_inst_to_free_lock;

    uint8_t surface_client_created[NUM_SURFACES];
    QRegion surface_client_lossy_region[NUM_SURFACES];

    StreamAgent stream_agents[NUM_STREAMS];
    int use_video_encoder_rate_control;
    uint32_t streams_max_latency;
    uint64_t streams_max_bit_rate;
    
    //for client render
    int render_width;
    int render_height;
    SpiceBitmap render_bitmap;
    SpiceRect render_area;
    SpiceCanvas* render_canvas;
};

#define DCC_TO_WORKER(dcc)                                              \
    (SPICE_CONTAINEROF((dcc)->common.base.channel, CommonChannel, base)->worker)
#define DCC_TO_DC(dcc)                                                  \
     SPICE_CONTAINEROF((dcc)->common.base.channel, DisplayChannel, common.base)
#define RCC_TO_DCC(rcc) SPICE_CONTAINEROF((rcc), DisplayChannelClient, common.base)


enum {
    PIPE_ITEM_TYPE_DRAW = PIPE_ITEM_TYPE_COMMON_LAST,
    PIPE_ITEM_TYPE_IMAGE,
    PIPE_ITEM_TYPE_STREAM_CREATE,
    PIPE_ITEM_TYPE_STREAM_CLIP,
    PIPE_ITEM_TYPE_STREAM_DESTROY,
    PIPE_ITEM_TYPE_UPGRADE,
    PIPE_ITEM_TYPE_MIGRATE_DATA,
    PIPE_ITEM_TYPE_PIXMAP_SYNC,
    PIPE_ITEM_TYPE_PIXMAP_RESET,
    PIPE_ITEM_TYPE_INVAL_PALETTE_CACHE,
    PIPE_ITEM_TYPE_CREATE_SURFACE,
    PIPE_ITEM_TYPE_DESTROY_SURFACE,
    PIPE_ITEM_TYPE_MONITORS_CONFIG,
    PIPE_ITEM_TYPE_STREAM_ACTIVATE_REPORT,
};



typedef struct DrawablePipeItem {
    RingItem base;  /* link for a list of pipe items held by Drawable */
    PipeItem dpi_pipe_item; /* link for the client's pipe itself */
    Drawable *drawable;
    DisplayChannelClient *dcc;
    uint8_t refs;
} DrawablePipeItem;

DrawablePipeItem*          drawable_pipe_item_new                    (DisplayChannelClient *dcc,
                                                                      Drawable *drawable);
void                       drawable_pipe_item_unref                  (DrawablePipeItem *dpi);
DrawablePipeItem*          drawable_pipe_item_ref                    (DrawablePipeItem *dpi);

typedef struct MonitorsConfig {
    int refs;
    int count;
    int max_allowed;
    QXLHead heads[0];
} MonitorsConfig;

typedef struct MonitorsConfigItem {
    PipeItem pipe_item;
    MonitorsConfig *monitors_config;
} MonitorsConfigItem;

MonitorsConfig*            monitors_config_new                       (QXLHead *heads, ssize_t nheads,
                                                                      ssize_t max);
MonitorsConfig *           monitors_config_ref                       (MonitorsConfig *config);
void                       monitors_config_unref                     (MonitorsConfig *config);

#define TRACE_ITEMS_SHIFT 3
#define NUM_TRACE_ITEMS (1 << TRACE_ITEMS_SHIFT)
#define ITEMS_TRACE_MASK (NUM_TRACE_ITEMS - 1)

typedef struct DrawContext {
    SpiceCanvas *canvas;
    int canvas_draws_on_surface;
    int top_down;
    uint32_t width;
    uint32_t height;
    int32_t stride;
    uint32_t format;
    void *line_0;
} DrawContext;

typedef struct RedSurface {
    uint32_t refs;
    Ring current;
    Ring current_list;
    DrawContext context;

    Ring depend_on_me;
    QRegion draw_dirty_region;

    //fix me - better handling here
    QXLReleaseInfoExt create, destroy;
} RedSurface;

#define NUM_DRAWABLES 1000
typedef struct _Drawable _Drawable;
struct _Drawable {
    union {
        Drawable drawable;
        _Drawable *next;
    } u;
};

struct DisplayChannel {
    CommonChannel common; // Must be the first thing

    MonitorsConfig *monitors_config;

    uint32_t num_renderers;
    uint32_t renderers[RED_RENDERER_LAST];
    uint32_t renderer;
    int enable_jpeg;
    int jpeg_quality;
    int enable_zlib_glz_wrap;
    int zlib_level;

    Ring current_list; // of TreeItem
    uint32_t current_size;

    uint32_t drawable_count;
    _Drawable drawables[NUM_DRAWABLES];
    _Drawable *free_drawables;

    int stream_video;
    uint32_t num_video_codecs;
    RedVideoCodec video_codecs[RED_MAX_VIDEO_CODECS];
    uint32_t stream_count;
    Stream streams_buf[NUM_STREAMS];
    Stream *free_streams;
    Ring streams;
    ItemTrace items_trace[NUM_TRACE_ITEMS];
    uint32_t next_item_trace;
    uint64_t streams_size_total;
    uint32_t fullstream_id;

    RedSurface surfaces[NUM_SURFACES];
    uint32_t n_surfaces;
    SpiceImageSurfaces image_surfaces;

    ImageCache image_cache;
    RedCompressBuf *free_compress_bufs;

/* TODO: some day unify this, make it more runtime.. */
#ifdef RED_WORKER_STAT
    stat_info_t add_stat;
    stat_info_t exclude_stat;
    stat_info_t __exclude_stat;
    uint32_t add_count;
    uint32_t add_with_shadow_count;
#endif
#ifdef RED_STATISTICS
    uint64_t *cache_hits_counter;
    uint64_t *add_to_cache_counter;
    uint64_t *non_cache_counter;
#endif
#ifdef COMPRESS_STAT
    stat_info_t lz_stat;
    stat_info_t glz_stat;
    stat_info_t quic_stat;
    stat_info_t jpeg_stat;
    stat_info_t zlib_glz_stat;
    stat_info_t jpeg_alpha_stat;
    stat_info_t lz4_stat;
#endif
};

#define LINK_TO_DCC(ptr) SPICE_CONTAINEROF(ptr, DisplayChannelClient,   \
                                           common.base.channel_link)
#define DCC_FOREACH_SAFE(link, next, dcc, channel)                      \
    SAFE_FOREACH(link, next, channel,  &(channel)->clients, dcc, LINK_TO_DCC(link))


#define FOREACH_DCC(display_channel, link, next, dcc)                   \
    DCC_FOREACH_SAFE(link, next, dcc, RED_CHANNEL(display_channel))

static inline int get_stream_id(DisplayChannel *display, Stream *stream)
{
    return (int)(stream - display->streams_buf);
}

typedef struct SurfaceDestroyItem {
    SpiceMsgSurfaceDestroy surface_destroy;
    PipeItem pipe_item;
} SurfaceDestroyItem;

typedef struct SurfaceCreateItem {
    SpiceMsgSurfaceCreate surface_create;
    PipeItem pipe_item;
} SurfaceCreateItem;

///////////////////////////////////////////////////
//DisplayChannel
void                       display_channel_set_stream_video          (DisplayChannel *display,
                                                                      int stream_video);
void                       display_channel_set_video_codecs          (DisplayChannel *display,
                                                                      int num_video_codecs,
                                                                      RedVideoCodec *video_codecs);
int                        display_channel_get_streams_timeout       (DisplayChannel *display);
void                       display_channel_compress_stats_print      (const DisplayChannel *display);
void                       display_channel_compress_stats_reset      (DisplayChannel *display);
void                       display_channel_drawable_unref            (DisplayChannel *display, Drawable *drawable);
void                       display_channel_surface_unref             (DisplayChannel *display,
                                                                      uint32_t surface_id);
bool                       display_channel_surface_has_canvas        (DisplayChannel *display,
                                                                      uint32_t surface_id);
int                        display_channel_add_drawable              (DisplayChannel *display,
                                                                      Drawable *drawable);

void 											 display_channel_stop_streams							 (DisplayChannel *display);

///////////////////////////////////////////////////
//DisplayChannelClient

DisplayChannelClient*      dcc_new                                   (DisplayChannel *display,
                                                                      RedClient *client,
                                                                      RedsStream *stream,
                                                                      int mig_target,
                                                                      uint32_t *common_caps,
                                                                      int num_common_caps,
                                                                      uint32_t *caps,
                                                                      int num_caps,
                                                                      SpiceImageCompression image_compression,
                                                                      spice_wan_compression_t jpeg_state,
                                                                      spice_wan_compression_t zlib_glz_state);
void                       dcc_push_monitors_config                  (DisplayChannelClient *dcc);
void                       dcc_push_destroy_surface                  (DisplayChannelClient *dcc,
                                                                      uint32_t surface_id);
void                       dcc_add_stream_agent_clip                 (DisplayChannelClient* dcc,
                                                                      StreamAgent *agent);
int                        dcc_create_stream                         (DisplayChannelClient *dcc,
                                                                      Stream *stream);
                                                                      
bool 											 dcc_allocate_render										   (DisplayChannelClient *dcc, const uint32_t render_type, DrawContext* surface_context, SpiceImageCache* image_cache, SpiceImageSurfaces* image_surface);
void											 dcc_deallocate_render										 (DisplayChannelClient *dcc);


static inline int is_equal_path(SpicePath *path1, SpicePath *path2)
{
    SpicePathSeg *seg1, *seg2;
    int i, j;

    if (path1->num_segments != path2->num_segments)
        return FALSE;

    for (i = 0; i < path1->num_segments; i++) {
        seg1 = path1->segments[i];
        seg2 = path2->segments[i];

        if (seg1->flags != seg2->flags ||
            seg1->count != seg2->count) {
            return FALSE;
        }
        for (j = 0; j < seg1->count; j++) {
            if (seg1->points[j].x != seg2->points[j].x ||
                seg1->points[j].y != seg2->points[j].y) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

// partial imp
static inline int is_equal_brush(SpiceBrush *b1, SpiceBrush *b2)
{
    return b1->type == b2->type &&
           b1->type == SPICE_BRUSH_TYPE_SOLID &&
           b1->u.color == b2->u.color;
}

// partial imp
static inline int is_equal_line_attr(SpiceLineAttr *a1, SpiceLineAttr *a2)
{
    return a1->flags == a2->flags &&
           a1->style_nseg == a2->style_nseg &&
           a1->style_nseg == 0;
}

// partial imp
static inline int is_same_geometry(Drawable *d1, Drawable *d2)
{
    if (d1->red_drawable->type != d2->red_drawable->type) {
        return FALSE;
    }

    switch (d1->red_drawable->type) {
    case QXL_DRAW_STROKE:
        return is_equal_line_attr(&d1->red_drawable->u.stroke.attr,
                                  &d2->red_drawable->u.stroke.attr) &&
               is_equal_path(d1->red_drawable->u.stroke.path,
                             d2->red_drawable->u.stroke.path);
    case QXL_DRAW_FILL:
        return rect_is_equal(&d1->red_drawable->bbox, &d2->red_drawable->bbox);
    default:
        return FALSE;
    }
}

static inline int is_same_drawable(Drawable *d1, Drawable *d2)
{
    if (!is_same_geometry(d1, d2)) {
        return FALSE;
    }

    switch (d1->red_drawable->type) {
    case QXL_DRAW_STROKE:
        return is_equal_brush(&d1->red_drawable->u.stroke.brush,
                              &d2->red_drawable->u.stroke.brush);
    case QXL_DRAW_FILL:
        return is_equal_brush(&d1->red_drawable->u.fill.brush,
                              &d2->red_drawable->u.fill.brush);
    default:
        return FALSE;
    }
}

static inline int is_drawable_independent_from_surfaces(Drawable *drawable)
{
    int x;

    for (x = 0; x < 3; ++x) {
        if (drawable->surface_deps[x] != -1) {
            return FALSE;
        }
    }
    return TRUE;
}

static inline int has_shadow(RedDrawable *drawable)
{
    return drawable->type == QXL_COPY_BITS;
}

static inline int is_primary_surface(DisplayChannel *display, uint32_t surface_id)
{
    if (surface_id == 0) {
        return TRUE;
    }
    return FALSE;
}

static inline void region_add_clip_rects(QRegion *rgn, SpiceClipRects *data)
{
    int i;

    for (i = 0; i < data->num_rects; i++) {
        region_add(rgn, data->rects + i);
    }
}

void red_pipes_add_drawable(DisplayChannel *display, Drawable *drawable);
void current_remove_drawable(DisplayChannel *display, Drawable *item);
void red_pipes_add_drawable_after(DisplayChannel *display,
                                  Drawable *drawable, Drawable *pos_after);
void red_pipes_remove_drawable(Drawable *drawable);
void dcc_add_drawable(DisplayChannelClient *dcc, Drawable *drawable);
void current_remove(DisplayChannel *display, TreeItem *item);
void detach_streams_behind(DisplayChannel *display, QRegion *region, Drawable *drawable);

#endif /* DISPLAY_CHANNEL_H_ */
