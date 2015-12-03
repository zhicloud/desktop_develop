/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
   Copyright (C) 2014 Red Hat, Inc.

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
#ifndef CACHE_ITEM_H_
# define CACHE_ITEM_H_

#include "red_channel.h"
#include "common/ring.h"

typedef struct CacheItem CacheItem;

struct CacheItem {
    union {
        PipeItem pipe_data;
        struct {
            RingItem lru_link;
            CacheItem *next;
        } cache_data;
    } u;
    uint64_t id;
    size_t size;
    uint32_t inval_type;
};

#endif /* CACHE_ITEM_H_ */
