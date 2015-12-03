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
#ifndef RED_BITMAP_UTILS_H_
# define RED_BITMAP_UTILS_H_

#include <glib.h>
#include <stdint.h>
#include "common/draw.h"
#include "common/log.h"

typedef enum {
    BITMAP_GRADUAL_INVALID,
    BITMAP_GRADUAL_NOT_AVAIL,
    BITMAP_GRADUAL_LOW,
    BITMAP_GRADUAL_MEDIUM,
    BITMAP_GRADUAL_HIGH,
} BitmapGradualType;

typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t pad;
} rgb32_pixel_t;

G_STATIC_ASSERT(sizeof(rgb32_pixel_t) == 4);

typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
} rgb24_pixel_t;

G_STATIC_ASSERT(sizeof(rgb24_pixel_t) == 3);

typedef uint16_t rgb16_pixel_t;


static inline int bitmap_fmt_get_bytes_per_pixel(uint8_t fmt)
{
    static const int bytes_per_pixel[] = {0, 0, 0, 0, 0, 1, 2, 3, 4, 4, 1};

    spice_return_val_if_fail(fmt < SPICE_N_ELEMENTS(bytes_per_pixel), 0);

    return bytes_per_pixel[fmt];
}


static inline int bitmap_fmt_is_plt(uint8_t fmt)
{
    static const int fmt_is_plt[] = {0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};

    spice_return_val_if_fail(fmt < SPICE_N_ELEMENTS(fmt_is_plt), 0);

    return fmt_is_plt[fmt];
}

static inline int bitmap_fmt_is_rgb(uint8_t fmt)
{
    static const int fmt_is_rgb[] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1};

    spice_return_val_if_fail(fmt < SPICE_N_ELEMENTS(fmt_is_rgb), 0);

    return fmt_is_rgb[fmt];
}

static inline int bitmap_fmt_has_graduality(uint8_t fmt)
{
    return bitmap_fmt_is_rgb(fmt) && fmt != SPICE_BITMAP_FMT_8BIT_A;
}


BitmapGradualType bitmap_get_graduality_level     (SpiceBitmap *bitmap);
int               bitmap_has_extra_stride         (SpiceBitmap *bitmap);

#endif /* RED_BITMAP_UTILS_H_ */
