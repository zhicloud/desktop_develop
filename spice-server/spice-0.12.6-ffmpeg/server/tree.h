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
#ifndef TREE_H_
# define TREE_H_

#include <stdint.h>
#include "common/region.h"
#include "common/ring.h"
#include "spice-bitmap-utils.h"

enum {
    TREE_ITEM_TYPE_NONE,
    TREE_ITEM_TYPE_DRAWABLE,
    TREE_ITEM_TYPE_CONTAINER,
    TREE_ITEM_TYPE_SHADOW,

    TREE_ITEM_TYPE_LAST,
};

typedef struct TreeItem TreeItem;
typedef struct Shadow Shadow;
typedef struct Container Container;
typedef struct DrawItem DrawItem;

/* TODO consider GNode instead */
struct TreeItem {
    RingItem siblings_link;
    uint32_t type;
    Container *container;
    QRegion rgn;
};

/* A region "below" a copy, or the src region of the copy */
struct Shadow {
    TreeItem base;
    QRegion on_hold;
    DrawItem* owner;
};

#define IS_SHADOW(item) ((item)->type == TREE_ITEM_TYPE_SHADOW)
#define SHADOW(item) ((Shadow*)(item))

struct Container {
    TreeItem base;
    Ring items;
};

#define IS_CONTAINER(item) ((item)->type == TREE_ITEM_TYPE_CONTAINER)
#define CONTAINER(item) ((Container*)(item))

struct DrawItem {
    TreeItem base;
    uint8_t effect;
    uint8_t container_root;
    Shadow *shadow;
};

#define IS_DRAW_ITEM(item) ((item)->type == TREE_ITEM_TYPE_DRAWABLE)
#define DRAW_ITEM(item) ((DrawItem*)(item))

static inline int is_opaque_item(TreeItem *item)
{
    return item->type == TREE_ITEM_TYPE_CONTAINER ||
        (IS_DRAW_ITEM(item) && ((DrawItem *)item)->effect == QXL_EFFECT_OPAQUE);
}

void       tree_item_dump                           (TreeItem *item);
Shadow*    tree_item_find_shadow                    (TreeItem *item);
int        tree_item_contained_by                   (TreeItem *item, Ring *ring);
Ring*      tree_item_container_items                (TreeItem *item, Ring *ring);

void       draw_item_remove_shadow                  (DrawItem *item);
Shadow*    shadow_new                               (DrawItem *item, const SpicePoint *delta);
Container* container_new                            (DrawItem *item);
void       container_free                           (Container *container);
void       container_cleanup                        (Container *container);

#endif /* TREE_H_ */
