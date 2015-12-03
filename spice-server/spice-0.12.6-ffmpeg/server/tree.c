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

#include <spice/qxl_dev.h>
#include "red_parse_qxl.h"
#include "display-channel.h"

#include "tree.h"

static const char *draw_type_to_str(uint8_t type)
{
    switch (type) {
    case QXL_DRAW_FILL:
        return "QXL_DRAW_FILL";
    case QXL_DRAW_OPAQUE:
        return "QXL_DRAW_OPAQUE";
    case QXL_DRAW_COPY:
        return "QXL_DRAW_COPY";
    case QXL_DRAW_TRANSPARENT:
        return "QXL_DRAW_TRANSPARENT";
    case QXL_DRAW_ALPHA_BLEND:
        return "QXL_DRAW_ALPHA_BLEND";
    case QXL_COPY_BITS:
        return "QXL_COPY_BITS";
    case QXL_DRAW_BLEND:
        return "QXL_DRAW_BLEND";
    case QXL_DRAW_BLACKNESS:
        return "QXL_DRAW_BLACKNESS";
    case QXL_DRAW_WHITENESS:
        return "QXL_DRAW_WHITENESS";
    case QXL_DRAW_INVERS:
        return "QXL_DRAW_INVERS";
    case QXL_DRAW_ROP3:
        return "QXL_DRAW_ROP3";
    case QXL_DRAW_COMPOSITE:
        return "QXL_DRAW_COMPOSITE";
    case QXL_DRAW_STROKE:
        return "QXL_DRAW_STROKE";
    case QXL_DRAW_TEXT:
        return "QXL_DRAW_TEXT";
    default:
        return "?";
    }
}

static void show_red_drawable(RedDrawable *drawable, const char *prefix)
{
    if (prefix) {
        printf("%s: ", prefix);
    }

    printf("%s effect %d bbox(%d %d %d %d)",
           draw_type_to_str(drawable->type),
           drawable->effect,
           drawable->bbox.top,
           drawable->bbox.left,
           drawable->bbox.bottom,
           drawable->bbox.right);

    switch (drawable->type) {
    case QXL_DRAW_FILL:
    case QXL_DRAW_OPAQUE:
    case QXL_DRAW_COPY:
    case QXL_DRAW_TRANSPARENT:
    case QXL_DRAW_ALPHA_BLEND:
    case QXL_COPY_BITS:
    case QXL_DRAW_BLEND:
    case QXL_DRAW_BLACKNESS:
    case QXL_DRAW_WHITENESS:
    case QXL_DRAW_INVERS:
    case QXL_DRAW_ROP3:
    case QXL_DRAW_COMPOSITE:
    case QXL_DRAW_STROKE:
    case QXL_DRAW_TEXT:
        break;
    default:
        spice_error("bad drawable type");
    }
    printf("\n");
}

static void show_draw_item(DrawItem *draw_item, const char *prefix)
{
    if (prefix) {
        printf("%s: ", prefix);
    }
    printf("effect %d bbox(%d %d %d %d)\n",
           draw_item->effect,
           draw_item->base.rgn.extents.x1,
           draw_item->base.rgn.extents.y1,
           draw_item->base.rgn.extents.x2,
           draw_item->base.rgn.extents.y2);
}

typedef struct DumpItem {
    int level;
    Container *container;
} DumpItem;

static void dump_item(TreeItem *item, void *data)
{
    DumpItem *di = data;
    const char *item_prefix = "|--";
    int i;

    if (di->container) {
        while (di->container != item->container) {
            di->level--;
            di->container = di->container->base.container;
        }
    }

    switch (item->type) {
    case TREE_ITEM_TYPE_DRAWABLE: {
        Drawable *drawable = SPICE_CONTAINEROF(item, Drawable, tree_item);
        const int max_indent = 200;
        char indent_str[max_indent + 1];
        int indent_str_len;

        for (i = 0; i < di->level; i++) {
            printf("  ");
        }
        printf(item_prefix, 0);
        show_red_drawable(drawable->red_drawable, NULL);
        for (i = 0; i < di->level; i++) {
            printf("  ");
        }
        printf("|  ");
        show_draw_item(&drawable->tree_item, NULL);
        indent_str_len = MIN(max_indent, strlen(item_prefix) + di->level * 2);
        memset(indent_str, ' ', indent_str_len);
        indent_str[indent_str_len] = 0;
        region_dump(&item->rgn, indent_str);
        printf("\n");
        break;
    }
    case TREE_ITEM_TYPE_CONTAINER:
        di->level++;
        di->container = (Container *)item;
        break;
    case TREE_ITEM_TYPE_SHADOW:
        break;
    }
}

static void tree_foreach(TreeItem *item, void (*f)(TreeItem *, void *), void * data)
{
    if (!item)
        return;

    f(item, data);

    if (item->type == TREE_ITEM_TYPE_CONTAINER) {
        Container *container = (Container*)item;
        RingItem *it;

        RING_FOREACH(it, &container->items) {
            tree_foreach(SPICE_CONTAINEROF(it, TreeItem, siblings_link), f, data);
        }
    }
}

void tree_item_dump(TreeItem *item)
{
    DumpItem di = { 0, };

    spice_return_if_fail(item != NULL);
    tree_foreach(item, dump_item, &di);
}

Shadow* shadow_new(DrawItem *item, const SpicePoint *delta)
{
    spice_return_val_if_fail(item->shadow == NULL, NULL);
    if (!delta->x && !delta->y) {
        return NULL;
    }

    Shadow *shadow = spice_new(Shadow, 1);

    shadow->base.type = TREE_ITEM_TYPE_SHADOW;
    shadow->base.container = NULL;
    shadow->owner = item;
    region_clone(&shadow->base.rgn, &item->base.rgn);
    region_offset(&shadow->base.rgn, delta->x, delta->y);
    ring_item_init(&shadow->base.siblings_link);
    region_init(&shadow->on_hold);
    item->shadow = shadow;

    return shadow;
}

Container* container_new(DrawItem *item)
{
    Container *container = spice_new(Container, 1);

    container->base.type = TREE_ITEM_TYPE_CONTAINER;
    container->base.container = item->base.container;
    item->base.container = container;
    item->container_root = TRUE;
    region_clone(&container->base.rgn, &item->base.rgn);
    ring_item_init(&container->base.siblings_link);
    ring_add_after(&container->base.siblings_link, &item->base.siblings_link);
    ring_remove(&item->base.siblings_link);
    ring_init(&container->items);
    ring_add(&container->items, &item->base.siblings_link);

    return container;
}

void container_free(Container *container)
{
    spice_return_if_fail(ring_is_empty(&container->items));

    ring_remove(&container->base.siblings_link);
    region_destroy(&container->base.rgn);
    free(container);
}

void container_cleanup(Container *container)
{
    /* visit upward, removing containers */
    /* non-empty container get its element moving up ?? */
    while (container && container->items.next == container->items.prev) {
        Container *next = container->base.container;
        if (container->items.next != &container->items) {
            TreeItem *item = (TreeItem *)ring_get_head(&container->items);
            spice_assert(item);
            ring_remove(&item->siblings_link);
            ring_add_after(&item->siblings_link, &container->base.siblings_link);
            item->container = container->base.container;
        }
        container_free(container);
        container = next;
    }
}

/* FIXME: document weird function: go down containers, and return drawable->shadow? */
Shadow* tree_item_find_shadow(TreeItem *item)
{
    while (item->type == TREE_ITEM_TYPE_CONTAINER) {
        if (!(item = (TreeItem *)ring_get_tail(&((Container *)item)->items))) {
            return NULL;
        }
    }

    if (item->type != TREE_ITEM_TYPE_DRAWABLE) {
        return NULL;
    }

    return ((DrawItem *)item)->shadow;
}

Ring *tree_item_container_items(TreeItem *item, Ring *ring)
{
    return (item->container) ? &item->container->items : ring;
}

int tree_item_contained_by(TreeItem *item, Ring *ring)
{
    spice_assert(item && ring);
    do {
        Ring *now = tree_item_container_items(item, ring);
        if (now == ring) {
            return TRUE;
        }
    } while ((item = (TreeItem *)item->container));

    return FALSE;
}

void draw_item_remove_shadow(DrawItem *item)
{
    Shadow *shadow;

    if (!item->shadow) {
        return;
    }
    shadow = item->shadow;
    item->shadow = NULL;
    ring_remove(&shadow->base.siblings_link);
    region_destroy(&shadow->base.rgn);
    region_destroy(&shadow->on_hold);
    free(shadow);
}
