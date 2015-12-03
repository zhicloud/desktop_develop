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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "spice-bitmap-utils.h"

#define RED_BITMAP_UTILS_RGB16
#include "red_bitmap_utils_tmpl.c"
#define RED_BITMAP_UTILS_RGB24
#include "red_bitmap_utils_tmpl.c"
#define RED_BITMAP_UTILS_RGB32
#include "red_bitmap_utils_tmpl.c"

#define GRADUAL_HIGH_RGB24_TH -0.03
#define GRADUAL_HIGH_RGB16_TH 0

// setting a more permissive threshold for stream identification in order
// not to miss streams that were artificially scaled on the guest (e.g., full screen view
// in window media player 12). see red_stream_add_frame
#define GRADUAL_MEDIUM_SCORE_TH 0.002

// assumes that stride doesn't overflow
BitmapGradualType bitmap_get_graduality_level(SpiceBitmap *bitmap)
{
    double score = 0.0;
    int num_samples = 0;
    int num_lines;
    double chunk_score = 0.0;
    int chunk_num_samples = 0;
    uint32_t x, i;
    SpiceChunk *chunk;

    chunk = bitmap->data->chunk;
    for (i = 0; i < bitmap->data->num_chunks; i++) {
        num_lines = chunk[i].len / bitmap->stride;
        x = bitmap->x;
        switch (bitmap->format) {
        case SPICE_BITMAP_FMT_16BIT:
            compute_lines_gradual_score_rgb16((rgb16_pixel_t *)chunk[i].data, x, num_lines,
                                              &chunk_score, &chunk_num_samples);
            break;
        case SPICE_BITMAP_FMT_24BIT:
            compute_lines_gradual_score_rgb24((rgb24_pixel_t *)chunk[i].data, x, num_lines,
                                              &chunk_score, &chunk_num_samples);
            break;
        case SPICE_BITMAP_FMT_32BIT:
        case SPICE_BITMAP_FMT_RGBA:
            compute_lines_gradual_score_rgb32((rgb32_pixel_t *)chunk[i].data, x, num_lines,
                                              &chunk_score, &chunk_num_samples);
            break;
        default:
            spice_error("invalid bitmap format (not RGB) %u", bitmap->format);
        }
        score += chunk_score;
        num_samples += chunk_num_samples;
    }

    spice_assert(num_samples);
    score /= num_samples;

    if (bitmap->format == SPICE_BITMAP_FMT_16BIT) {
        if (score < GRADUAL_HIGH_RGB16_TH) {
            return BITMAP_GRADUAL_HIGH;
        }
    } else {
        if (score < GRADUAL_HIGH_RGB24_TH) {
            return BITMAP_GRADUAL_HIGH;
        }
    }

    if (score < GRADUAL_MEDIUM_SCORE_TH) {
        return BITMAP_GRADUAL_MEDIUM;
    } else {
        return BITMAP_GRADUAL_LOW;
    }
}

int bitmap_has_extra_stride(SpiceBitmap *bitmap)
{
    spice_assert(bitmap);
    if (bitmap_fmt_is_rgb(bitmap->format)) {
        return ((bitmap->x * bitmap_fmt_get_bytes_per_pixel(bitmap->format)) < bitmap->stride);
    } else {
        switch (bitmap->format) {
        case SPICE_BITMAP_FMT_8BIT:
            return (bitmap->x < bitmap->stride);
        case SPICE_BITMAP_FMT_4BIT_BE:
        case SPICE_BITMAP_FMT_4BIT_LE: {
            int bytes_width = SPICE_ALIGN(bitmap->x, 2) >> 1;
            return bytes_width < bitmap->stride;
        }
        case SPICE_BITMAP_FMT_1BIT_BE:
        case SPICE_BITMAP_FMT_1BIT_LE: {
            int bytes_width = SPICE_ALIGN(bitmap->x, 8) >> 3;
            return bytes_width < bitmap->stride;
        }
        default:
            spice_error("invalid image type %u", bitmap->format);
            return 0;
        }
    }
    return 0;
}
