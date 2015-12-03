/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
   Copyright (C) 2009 Red Hat, Inc.

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

#include <glib.h>
#include "common/generated_server_marshallers.h"
#include "cursor-channel.h"

#define CLIENT_CURSOR_CACHE_SIZE 256

#define CURSOR_CACHE_HASH_SHIFT 8
#define CURSOR_CACHE_HASH_SIZE (1 << CURSOR_CACHE_HASH_SHIFT)
#define CURSOR_CACHE_HASH_MASK (CURSOR_CACHE_HASH_SIZE - 1)
#define CURSOR_CACHE_HASH_KEY(id) ((id) & CURSOR_CACHE_HASH_MASK)

enum {
    PIPE_ITEM_TYPE_CURSOR = PIPE_ITEM_TYPE_COMMON_LAST,
    PIPE_ITEM_TYPE_CURSOR_INIT,
    PIPE_ITEM_TYPE_INVAL_CURSOR_CACHE,
};

typedef struct CursorItem {
    QXLInstance *qxl;
    uint32_t group_id;
    int refs;
    RedCursorCmd *red_cursor;
} CursorItem;

G_STATIC_ASSERT(sizeof(CursorItem) <= QXL_CURSUR_DEVICE_DATA_SIZE);

typedef struct LocalCursor {
    CursorItem base;
    SpicePoint16 position;
    uint32_t data_size;
    SpiceCursor red_cursor;
} LocalCursor;

typedef struct CursorPipeItem {
    PipeItem base;
    CursorItem *cursor_item;
    int refs;
} CursorPipeItem;

struct CursorChannel {
    CommonChannel common; // Must be the first thing

    CursorItem *item;
    int cursor_visible;
    SpicePoint16 cursor_position;
    uint16_t cursor_trail_length;
    uint16_t cursor_trail_frequency;
    uint32_t mouse_mode;

#ifdef RED_STATISTICS
    StatNodeRef stat;
#endif
};

struct CursorChannelClient {
    CommonChannelClient common;

    CacheItem *cursor_cache[CURSOR_CACHE_HASH_SIZE];
    Ring cursor_cache_lru;
    long cursor_cache_available;
    uint32_t cursor_cache_items;
};


#define RCC_TO_CCC(rcc) SPICE_CONTAINEROF((rcc), CursorChannelClient, common.base)

#define CLIENT_CURSOR_CACHE
#include "cache_item.tmpl.c"
#undef CLIENT_CURSOR_CACHE

static CursorItem *cursor_item_new(QXLInstance *qxl, RedCursorCmd *cmd, uint32_t group_id)
{
    CursorItem *cursor_item;

    spice_return_val_if_fail(cmd != NULL, NULL);

    cursor_item = g_slice_new0(CursorItem);
    cursor_item->qxl = qxl;
    cursor_item->refs = 1;
    cursor_item->group_id = group_id;
    cursor_item->red_cursor = cmd;

    return cursor_item;
}

static CursorItem *cursor_item_ref(CursorItem *item)
{
    spice_return_val_if_fail(item != NULL, NULL);
    spice_return_val_if_fail(item->refs > 0, NULL);

    item->refs++;

    return item;
}

static void cursor_item_unref(CursorItem *item)
{
    QXLReleaseInfoExt release_info_ext;
    RedCursorCmd *cursor_cmd;

    spice_return_if_fail(item != NULL);

    if (--item->refs)
        return;

    cursor_cmd = item->red_cursor;
    release_info_ext.group_id = item->group_id;
    release_info_ext.info = cursor_cmd->release_info;
    item->qxl->st->qif->release_resource(item->qxl, release_info_ext);
    red_put_cursor_cmd(cursor_cmd);
    free(cursor_cmd);

    g_slice_free(CursorItem, item);

}

static void cursor_set_item(CursorChannel *cursor, CursorItem *item)
{
    if (cursor->item)
        cursor_item_unref(cursor->item);

    cursor->item = item ? cursor_item_ref(item) : NULL;
}

static PipeItem *new_cursor_pipe_item(RedChannelClient *rcc, void *data, int num)
{
    CursorPipeItem *item = spice_malloc0(sizeof(CursorPipeItem));

    red_channel_pipe_item_init(rcc->channel, &item->base, PIPE_ITEM_TYPE_CURSOR);
    item->refs = 1;
    item->cursor_item = data;
    item->cursor_item->refs++;
    return &item->base;
}

typedef struct {
    void *data;
    uint32_t size;
} AddBufInfo;

static void add_buf_from_info(SpiceMarshaller *m, AddBufInfo *info)
{
    if (info->data) {
        spice_marshaller_add_ref(m, info->data, info->size);
    }
}

static void cursor_fill(CursorChannelClient *ccc, SpiceCursor *red_cursor,
                        CursorItem *cursor, AddBufInfo *addbuf)
{
    RedCursorCmd *cursor_cmd;
    addbuf->data = NULL;

    if (!cursor) {
        red_cursor->flags = SPICE_CURSOR_FLAGS_NONE;
        return;
    }

    cursor_cmd = cursor->red_cursor;
    *red_cursor = cursor_cmd->u.set.shape;

    if (red_cursor->header.unique) {
        if (red_cursor_cache_find(ccc, red_cursor->header.unique)) {
            red_cursor->flags |= SPICE_CURSOR_FLAGS_FROM_CACHE;
            return;
        }
        if (red_cursor_cache_add(ccc, red_cursor->header.unique, 1)) {
            red_cursor->flags |= SPICE_CURSOR_FLAGS_CACHE_ME;
        }
    }

    if (red_cursor->data_size) {
        addbuf->data = red_cursor->data;
        addbuf->size = red_cursor->data_size;
    }
}


static void red_reset_cursor_cache(RedChannelClient *rcc)
{
    red_cursor_cache_reset(RCC_TO_CCC(rcc), CLIENT_CURSOR_CACHE_SIZE);
}

void cursor_channel_disconnect(CursorChannel *cursor_channel)
{
    RedChannel *channel = (RedChannel *)cursor_channel;

    if (!channel || !red_channel_is_connected(channel)) {
        return;
    }
    red_channel_apply_clients(channel, red_reset_cursor_cache);
    red_channel_disconnect(channel);
}


static void put_cursor_pipe_item(CursorChannelClient *ccc, CursorPipeItem *pipe_item)
{
    spice_return_if_fail(pipe_item);
    spice_return_if_fail(pipe_item->refs > 0);

    if (--pipe_item->refs) {
        return;
    }

    spice_assert(!pipe_item_is_linked(&pipe_item->base));

    cursor_item_unref(pipe_item->cursor_item);
    free(pipe_item);
}

static void cursor_channel_client_on_disconnect(RedChannelClient *rcc)
{
    if (!rcc) {
        return;
    }
    red_reset_cursor_cache(rcc);
}

// TODO: share code between before/after_push since most of the items need the same
// release
static void cursor_channel_client_release_item_before_push(CursorChannelClient *ccc,
                                                           PipeItem *item)
{
    switch (item->type) {
    case PIPE_ITEM_TYPE_CURSOR: {
        CursorPipeItem *cursor_pipe_item = SPICE_CONTAINEROF(item, CursorPipeItem, base);
        put_cursor_pipe_item(ccc, cursor_pipe_item);
        break;
    }
    case PIPE_ITEM_TYPE_INVAL_ONE:
    case PIPE_ITEM_TYPE_VERB:
    case PIPE_ITEM_TYPE_CURSOR_INIT:
    case PIPE_ITEM_TYPE_INVAL_CURSOR_CACHE:
        free(item);
        break;
    default:
        spice_error("invalid pipe item type");
    }
}

static void cursor_channel_client_release_item_after_push(CursorChannelClient *ccc,
                                                          PipeItem *item)
{
    switch (item->type) {
        case PIPE_ITEM_TYPE_CURSOR: {
            CursorPipeItem *cursor_pipe_item = SPICE_CONTAINEROF(item, CursorPipeItem, base);
            put_cursor_pipe_item(ccc, cursor_pipe_item);
            break;
        }
        default:
            spice_critical("invalid item type");
    }
}

static void red_marshall_cursor_init(RedChannelClient *rcc, SpiceMarshaller *base_marshaller,
                                     PipeItem *pipe_item)
{
    CursorChannel *cursor_channel;
    CursorChannelClient *ccc = RCC_TO_CCC(rcc);
    SpiceMsgCursorInit msg;
    AddBufInfo info;

    spice_assert(rcc);
    cursor_channel = SPICE_CONTAINEROF(rcc->channel, CursorChannel, common.base);

    red_channel_client_init_send_data(rcc, SPICE_MSG_CURSOR_INIT, NULL);
    msg.visible = cursor_channel->cursor_visible;
    msg.position = cursor_channel->cursor_position;
    msg.trail_length = cursor_channel->cursor_trail_length;
    msg.trail_frequency = cursor_channel->cursor_trail_frequency;

    cursor_fill(ccc, &msg.cursor, cursor_channel->item, &info);
    spice_marshall_msg_cursor_init(base_marshaller, &msg);
    add_buf_from_info(base_marshaller, &info);
}

static void cursor_marshall(RedChannelClient *rcc,
                            SpiceMarshaller *m, CursorPipeItem *cursor_pipe_item)
{
    CursorChannel *cursor_channel = SPICE_CONTAINEROF(rcc->channel, CursorChannel, common.base);
    CursorChannelClient *ccc = RCC_TO_CCC(rcc);
    CursorItem *item = cursor_pipe_item->cursor_item;
    PipeItem *pipe_item = &cursor_pipe_item->base;
    RedCursorCmd *cmd;

    spice_return_if_fail(cursor_channel);

    cmd = item->red_cursor;
    switch (cmd->type) {
    case QXL_CURSOR_MOVE:
        {
            SpiceMsgCursorMove cursor_move;
            red_channel_client_init_send_data(rcc, SPICE_MSG_CURSOR_MOVE, pipe_item);
            cursor_move.position = cmd->u.position;
            spice_marshall_msg_cursor_move(m, &cursor_move);
            break;
        }
    case QXL_CURSOR_SET:
        {
            SpiceMsgCursorSet cursor_set;
            AddBufInfo info;

            red_channel_client_init_send_data(rcc, SPICE_MSG_CURSOR_SET, pipe_item);
            cursor_set.position = cmd->u.set.position;
            cursor_set.visible = cursor_channel->cursor_visible;

            cursor_fill(ccc, &cursor_set.cursor, item, &info);
            spice_marshall_msg_cursor_set(m, &cursor_set);
            add_buf_from_info(m, &info);
            break;
        }
    case QXL_CURSOR_HIDE:
        red_channel_client_init_send_data(rcc, SPICE_MSG_CURSOR_HIDE, pipe_item);
        break;
    case QXL_CURSOR_TRAIL:
        {
            SpiceMsgCursorTrail cursor_trail;

            red_channel_client_init_send_data(rcc, SPICE_MSG_CURSOR_TRAIL, pipe_item);
            cursor_trail.length = cmd->u.trail.length;
            cursor_trail.frequency = cmd->u.trail.frequency;
            spice_marshall_msg_cursor_trail(m, &cursor_trail);
        }
        break;
    default:
        spice_error("bad cursor command %d", cmd->type);
    }
}

static inline void red_marshall_inval(RedChannelClient *rcc,
                                      SpiceMarshaller *base_marshaller, CacheItem *cach_item)
{
    SpiceMsgDisplayInvalOne inval_one;

    red_channel_client_init_send_data(rcc, cach_item->inval_type, NULL);
    inval_one.id = *(uint64_t *)&cach_item->id;

    spice_marshall_msg_cursor_inval_one(base_marshaller, &inval_one);
}

static void cursor_channel_send_item(RedChannelClient *rcc, PipeItem *pipe_item)
{
    SpiceMarshaller *m = red_channel_client_get_marshaller(rcc);
    CursorChannelClient *ccc = RCC_TO_CCC(rcc);

    switch (pipe_item->type) {
    case PIPE_ITEM_TYPE_CURSOR:
        cursor_marshall(rcc, m, SPICE_CONTAINEROF(pipe_item, CursorPipeItem, base));
        break;
    case PIPE_ITEM_TYPE_INVAL_ONE:
        red_marshall_inval(rcc, m, (CacheItem *)pipe_item);
        break;
    case PIPE_ITEM_TYPE_VERB:
        red_marshall_verb(rcc, (VerbItem*)pipe_item);
        break;
    case PIPE_ITEM_TYPE_CURSOR_INIT:
        red_reset_cursor_cache(rcc);
        red_marshall_cursor_init(rcc, m, pipe_item);
        break;
    case PIPE_ITEM_TYPE_INVAL_CURSOR_CACHE:
        red_reset_cursor_cache(rcc);
        red_channel_client_init_send_data(rcc, SPICE_MSG_CURSOR_INVAL_ALL, NULL);
        break;
    default:
        spice_error("invalid pipe item type");
    }

    cursor_channel_client_release_item_before_push(ccc, pipe_item);
    red_channel_client_begin_send_message(rcc);
}

static CursorPipeItem *cursor_pipe_item_ref(CursorPipeItem *item)
{
    spice_return_val_if_fail(item, NULL);
    spice_return_val_if_fail(item->refs > 0, NULL);

    item->refs++;
    return item;
}


static void cursor_channel_hold_pipe_item(RedChannelClient *rcc, PipeItem *item)
{
    CursorPipeItem *cursor_pipe_item;

    spice_return_if_fail(item);

    cursor_pipe_item = SPICE_CONTAINEROF(item, CursorPipeItem, base);
    cursor_pipe_item_ref(cursor_pipe_item);
}

static void cursor_channel_release_item(RedChannelClient *rcc, PipeItem *item, int item_pushed)
{
    CursorChannelClient *ccc = RCC_TO_CCC(rcc);

    spice_assert(item);

    if (item_pushed) {
        cursor_channel_client_release_item_after_push(ccc, item);
    } else {
        spice_debug("not pushed (%d)", item->type);
        cursor_channel_client_release_item_before_push(ccc, item);
    }
}

CursorChannel* cursor_channel_new(RedWorker *worker)
{
    CursorChannel *cursor_channel;
    RedChannel *channel = NULL;
    ChannelCbs cbs = {
        .on_disconnect =  cursor_channel_client_on_disconnect,
        .send_item = cursor_channel_send_item,
        .hold_item = cursor_channel_hold_pipe_item,
        .release_item = cursor_channel_release_item
    };

    spice_info("create cursor channel");
    channel = red_worker_new_channel(worker, sizeof(CursorChannel), "cursor_channel",
                                     SPICE_CHANNEL_CURSOR, 0,
                                     &cbs, red_channel_client_handle_message);

    cursor_channel = (CursorChannel *)channel;
    cursor_channel->cursor_visible = TRUE;
    cursor_channel->mouse_mode = SPICE_MOUSE_MODE_SERVER;

    return cursor_channel;
}

void cursor_channel_client_migrate(CursorChannelClient* client)
{
    RedChannelClient *rcc;

    spice_return_if_fail(client);
    rcc = RED_CHANNEL_CLIENT(client);

    red_channel_client_pipe_add_type(rcc, PIPE_ITEM_TYPE_INVAL_CURSOR_CACHE);
    red_channel_client_default_migrate(rcc);
}

CursorChannelClient* cursor_channel_client_new(CursorChannel *cursor, RedClient *client, RedsStream *stream,
                                               int mig_target,
                                               uint32_t *common_caps, int num_common_caps,
                                               uint32_t *caps, int num_caps)
{
    spice_return_val_if_fail(cursor, NULL);
    spice_return_val_if_fail(client, NULL);
    spice_return_val_if_fail(stream, NULL);
    spice_return_val_if_fail(!num_common_caps || common_caps, NULL);
    spice_return_val_if_fail(!num_caps || caps, NULL);

    CursorChannelClient *ccc =
        (CursorChannelClient*)common_channel_new_client(&cursor->common,
                                                        sizeof(CursorChannelClient),
                                                        client, stream,
                                                        mig_target,
                                                        FALSE,
                                                        common_caps,
                                                        num_common_caps,
                                                        caps,
                                                        num_caps);
    spice_return_val_if_fail(ccc != NULL, NULL);

    ring_init(&ccc->cursor_cache_lru);
    ccc->cursor_cache_available = CLIENT_CURSOR_CACHE_SIZE;

    return ccc;
}

void cursor_channel_process_cmd(CursorChannel *cursor, RedCursorCmd *cursor_cmd,
                                uint32_t group_id)
{
    CursorItem *cursor_item;
    int cursor_show = FALSE;

    spice_return_if_fail(cursor);
    spice_return_if_fail(cursor_cmd);

    cursor_item = cursor_item_new(red_worker_get_qxl(cursor->common.worker),
                                  cursor_cmd, group_id);

    switch (cursor_cmd->type) {
    case QXL_CURSOR_SET:
        cursor->cursor_visible = cursor_cmd->u.set.visible;
        cursor_set_item(cursor, cursor_item);
        break;
    case QXL_CURSOR_MOVE:
        cursor_show = !cursor->cursor_visible;
        cursor->cursor_visible = TRUE;
        cursor->cursor_position = cursor_cmd->u.position;
        break;
    case QXL_CURSOR_HIDE:
        cursor->cursor_visible = FALSE;
        break;
    case QXL_CURSOR_TRAIL:
        cursor->cursor_trail_length = cursor_cmd->u.trail.length;
        cursor->cursor_trail_frequency = cursor_cmd->u.trail.frequency;
        break;
    default:
        spice_warning("invalid cursor command %u", cursor_cmd->type);
        return;
    }

    if (red_channel_is_connected(&cursor->common.base) &&
        (cursor->mouse_mode == SPICE_MOUSE_MODE_SERVER
         || cursor_cmd->type != QXL_CURSOR_MOVE
         || cursor_show)) {
        red_channel_pipes_new_add(&cursor->common.base,
                                  new_cursor_pipe_item, cursor_item);
    }

    cursor_item_unref(cursor_item);
}

void cursor_channel_reset(CursorChannel *cursor)
{
    RedChannel *channel = &cursor->common.base;

    spice_return_if_fail(cursor);

    cursor_set_item(cursor, NULL);
    cursor->cursor_visible = TRUE;
    cursor->cursor_position.x = cursor->cursor_position.y = 0;
    cursor->cursor_trail_length = cursor->cursor_trail_frequency = 0;

    if (red_channel_is_connected(channel)) {
        red_channel_pipes_add_type(channel, PIPE_ITEM_TYPE_INVAL_CURSOR_CACHE);
        if (!cursor->common.during_target_migrate) {
            red_pipes_add_verb(channel, SPICE_MSG_CURSOR_RESET);
        }
        if (!red_channel_wait_all_sent(&cursor->common.base,
                                       DISPLAY_CLIENT_TIMEOUT)) {
            red_channel_apply_clients(channel,
                                      red_channel_client_disconnect_if_pending_send);
        }
    }
}

void cursor_channel_init(CursorChannel *cursor, CursorChannelClient *client)
{
    spice_return_if_fail(cursor);

    if (!red_channel_is_connected(&cursor->common.base)
        || COMMON_CHANNEL(cursor)->during_target_migrate) {
        spice_debug("during_target_migrate: skip init");
        return;
    }

    if (client)
        red_channel_client_pipe_add_type(RED_CHANNEL_CLIENT(client),
                                         PIPE_ITEM_TYPE_CURSOR_INIT);
    else
        red_channel_pipes_add_type(RED_CHANNEL(cursor), PIPE_ITEM_TYPE_CURSOR_INIT);
}

void cursor_channel_set_mouse_mode(CursorChannel *cursor, uint32_t mode)
{
    spice_return_if_fail(cursor);

    cursor->mouse_mode = mode;
}
