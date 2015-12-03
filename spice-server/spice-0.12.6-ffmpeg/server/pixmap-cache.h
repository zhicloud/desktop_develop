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
#ifndef _PIXMAP_CACHE_H
# define _PIXMAP_CACHE_H

#include "red_channel.h"
#include "spice_server_utils.h"

#define MAX_CACHE_CLIENTS 4

#define BITS_CACHE_HASH_SHIFT 10
#define BITS_CACHE_HASH_SIZE (1 << BITS_CACHE_HASH_SHIFT)
#define BITS_CACHE_HASH_MASK (BITS_CACHE_HASH_SIZE - 1)
#define BITS_CACHE_HASH_KEY(id) ((id) & BITS_CACHE_HASH_MASK)

typedef struct PixmapCache PixmapCache;
typedef struct NewCacheItem NewCacheItem;

struct NewCacheItem {
    RingItem lru_link;
    NewCacheItem *next;
    uint64_t id;
    uint64_t sync[MAX_CACHE_CLIENTS];
    size_t size;
    int lossy;
};

struct PixmapCache {
    RingItem base;
    pthread_mutex_t lock;
    uint8_t id;
    uint32_t refs;
    NewCacheItem *hash_table[BITS_CACHE_HASH_SIZE];
    Ring lru;
    int64_t available;
    int64_t size;
    int32_t items;

    int freezed;
    RingItem *freezed_head;
    RingItem *freezed_tail;

    uint32_t generation;
    struct {
        uint8_t client;
        uint64_t message;
    } generation_initiator;
    uint64_t sync[MAX_CACHE_CLIENTS]; // here CLIENTS refer to different channel
                                      // clients of the same client
    RedClient *client;
};

PixmapCache *pixmap_cache_get(RedClient *client, uint8_t id, int64_t size);
void         pixmap_cache_unref(PixmapCache *cache);
void         pixmap_cache_clear(PixmapCache *cache);
int          pixmap_cache_unlocked_set_lossy(PixmapCache *cache, uint64_t id, int lossy);
int          pixmap_cache_freeze(PixmapCache *cache);

#endif /* _PIXMAP_CACHE_H */
