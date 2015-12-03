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

#ifndef _H_REDWORKER
#define _H_REDWORKER

#include <unistd.h>
#include <errno.h>
#include "red_common.h"
#include "red_dispatcher.h"

typedef struct RedWorker RedWorker;

typedef struct CommonChannelClient {
    RedChannelClient base;

    uint32_t id;
    struct RedWorker *worker;
    int is_low_bandwidth;
} CommonChannelClient;

#define COMMON_CHANNEL_CLIENT(Client) ((CommonChannelClient*)(Client))
#define DISPLAY_CLIENT_TIMEOUT 30000000000ULL //nano

#define CHANNEL_RECEIVE_BUF_SIZE 1024
typedef struct CommonChannel {
    RedChannel base; // Must be the first thing

    struct RedWorker *worker;
    uint8_t recv_buf[CHANNEL_RECEIVE_BUF_SIZE];
    uint32_t id_alloc; // bitfield. TODO - use this instead of shift scheme.
    int during_target_migrate; /* TRUE when the client that is associated with the channel
                                  is during migration. Turned off when the vm is started.
                                  The flag is used to avoid sending messages that are artifacts
                                  of the transition from stopped vm to loaded vm (e.g., recreation
                                  of the primary surface) */
} CommonChannel;

#define COMMON_CHANNEL(Channel) ((CommonChannel*)(Channel))

enum {
    PIPE_ITEM_TYPE_VERB = PIPE_ITEM_TYPE_CHANNEL_BASE,
    PIPE_ITEM_TYPE_INVAL_ONE,

    PIPE_ITEM_TYPE_COMMON_LAST
};

typedef struct VerbItem {
    PipeItem base;
    uint16_t verb;
} VerbItem;

static inline void red_marshall_verb(RedChannelClient *rcc, VerbItem *item)
{
    red_channel_client_init_send_data(rcc, item->verb, NULL);
}

static inline void red_pipe_add_verb(RedChannelClient* rcc, uint16_t verb)
{
    VerbItem *item = spice_new(VerbItem, 1);

    red_channel_pipe_item_init(rcc->channel, &item->base, PIPE_ITEM_TYPE_VERB);
    item->verb = verb;
    red_channel_client_pipe_add(rcc, &item->base);
}

/* a generic safe for loop macro  */
#define SAFE_FOREACH(link, next, cond, ring, data, get_data)               \
    for ((((link) = ((cond) ? ring_get_head(ring) : NULL)), \
          ((next) = ((link) ? ring_next((ring), (link)) : NULL)),          \
          ((data) = ((link)? (get_data) : NULL)));                         \
         (link);                                                           \
         (((link) = (next)),                                               \
          ((next) = ((link) ? ring_next((ring), (link)) : NULL)),          \
          ((data) = ((link)? (get_data) : NULL))))

#define LINK_TO_RCC(ptr) SPICE_CONTAINEROF(ptr, RedChannelClient, channel_link)
#define RCC_FOREACH_SAFE(link, next, rcc, channel) \
    SAFE_FOREACH(link, next, channel,  &(channel)->clients, rcc, LINK_TO_RCC(link))


static inline void red_pipes_add_verb(RedChannel *channel, uint16_t verb)
{
    RedChannelClient *rcc;
    RingItem *link, *next;

    RCC_FOREACH_SAFE(link, next, rcc, channel) {
        red_pipe_add_verb(rcc, verb);
    }
}

RedWorker* red_worker_new(QXLInstance *qxl, RedDispatcher *red_dispatcher);
bool       red_worker_run(RedWorker *worker);
QXLInstance* red_worker_get_qxl(RedWorker *worker);
RedChannel* red_worker_get_cursor_channel(RedWorker *worker);
RedChannel* red_worker_get_display_channel(RedWorker *worker);
clockid_t red_worker_get_clockid(RedWorker *worker);

RedChannel *red_worker_new_channel(RedWorker *worker, int size,
                                   const char *name,
                                   uint32_t channel_type, int migration_flags,
                                   ChannelCbs *channel_cbs,
                                   channel_handle_parsed_proc handle_parsed);

CommonChannelClient *common_channel_new_client(CommonChannel *common,
                                               int size,
                                               RedClient *client,
                                               RedsStream *stream,
                                               int mig_target,
                                               int monitor_latency,
                                               uint32_t *common_caps,
                                               int num_common_caps,
                                               uint32_t *caps,
                                               int num_caps);

#endif
