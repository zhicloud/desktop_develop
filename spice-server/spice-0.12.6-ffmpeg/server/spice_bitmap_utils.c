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
#include <stdio.h>

#include "common/log.h"
#include "common/draw.h"

#include "spice_bitmap_utils.h"

int spice_bitmap_from_surface_type(uint32_t surface_format)
{
    switch (surface_format) {
    case SPICE_SURFACE_FMT_16_555:
        return SPICE_BITMAP_FMT_16BIT;
    case SPICE_SURFACE_FMT_32_xRGB:
        return SPICE_BITMAP_FMT_32BIT;
    case SPICE_SURFACE_FMT_32_ARGB:
        return SPICE_BITMAP_FMT_RGBA;
    case SPICE_SURFACE_FMT_8_A:
        return SPICE_BITMAP_FMT_8BIT_A;
    default:
        spice_critical("Unsupported surface format");
    }
    return 0;
}

#define RAM_PATH "/tmp/tmpfs"

static void dump_palette(FILE *f, SpicePalette* plt)
{
    int i;
    for (i = 0; i < plt->num_ents; i++) {
        if( 1 != fwrite(plt->ents + i, sizeof(uint32_t), 1, f)){
        	spice_error("dump_palette error");
        	return;
        }
    }
}

static void dump_line(FILE *f, uint8_t* line, uint16_t n_pixel_bits, int width, int row_size)
{
    int i;
    int copy_bytes_size = SPICE_ALIGN(n_pixel_bits * width, 8) / 8;
		if( copy_bytes_size != fwrite(line, 1, copy_bytes_size, f)){
			spice_error("dump_line error");
      return;
		}
    if (row_size > copy_bytes_size) {
        // each line should be 4 bytes aligned
        for (i = copy_bytes_size; i < row_size; i++) {
            fprintf(f, "%c", 0);
        }
    }
}
void dump_bitmap(SpiceBitmap *bitmap)
{
    static uint32_t file_id = 0;

    char file_str[200];
    int rgb = TRUE;
    uint16_t n_pixel_bits;
    SpicePalette *plt = NULL;
    uint32_t id;
    int row_size;
    uint32_t file_size;
    int alpha = 0;
    uint32_t header_size = 14 + 40;
    uint32_t bitmap_data_offset;
    uint32_t tmp_u32;
    int32_t tmp_32;
    uint16_t tmp_u16;
    FILE *f;
    int i, j;

    switch (bitmap->format) {
    case SPICE_BITMAP_FMT_1BIT_BE:
    case SPICE_BITMAP_FMT_1BIT_LE:
        rgb = FALSE;
        n_pixel_bits = 1;
        break;
    case SPICE_BITMAP_FMT_4BIT_BE:
    case SPICE_BITMAP_FMT_4BIT_LE:
        rgb = FALSE;
        n_pixel_bits = 4;
        break;
    case SPICE_BITMAP_FMT_8BIT:
        rgb = FALSE;
        n_pixel_bits = 8;
        break;
    case SPICE_BITMAP_FMT_16BIT:
        n_pixel_bits = 16;
        break;
    case SPICE_BITMAP_FMT_24BIT:
        n_pixel_bits = 24;
        break;
    case SPICE_BITMAP_FMT_32BIT:
        n_pixel_bits = 32;
        break;
    case SPICE_BITMAP_FMT_RGBA:
        n_pixel_bits = 32;
        alpha = 1;
        break;
    default:
        spice_error("invalid bitmap format  %u", bitmap->format);
        return;
    }

    if (!rgb) {
        if (!bitmap->palette) {
            return; // dont dump masks.
        }
        plt = bitmap->palette;
    }
    row_size = (((bitmap->x * n_pixel_bits) + 31) / 32) * 4;
    bitmap_data_offset = header_size;

    if (plt) {
        bitmap_data_offset += plt->num_ents * 4;
    }
    file_size = bitmap_data_offset + (bitmap->y * row_size);

    id = ++file_id;
    sprintf(file_str, "%s/%u.bmp", RAM_PATH, id);

    f = fopen(file_str, "wb");
    if (!f) {
        spice_error("Error creating bmp");
        return;
    }

		int result;
    /* writing the bmp v3 header */
    fprintf(f, "BM");
    result = fwrite(&file_size, sizeof(file_size), 1, f);
    tmp_u16 = alpha ? 1 : 0;
    result = fwrite(&tmp_u16, sizeof(tmp_u16), 1, f); // reserved for application
    tmp_u16 = 0;
    result = fwrite(&tmp_u16, sizeof(tmp_u16), 1, f);
    result = fwrite(&bitmap_data_offset, sizeof(bitmap_data_offset), 1, f);
    tmp_u32 = header_size - 14;
    result = fwrite(&tmp_u32, sizeof(tmp_u32), 1, f); // sub header size
    tmp_32 = bitmap->x;
    result = fwrite(&tmp_32, sizeof(tmp_32), 1, f);
    tmp_32 = bitmap->y;
    result = fwrite(&tmp_32, sizeof(tmp_32), 1, f);

    tmp_u16 = 1;
    result = fwrite(&tmp_u16, sizeof(tmp_u16), 1, f); // color plane
    result = fwrite(&n_pixel_bits, sizeof(n_pixel_bits), 1, f); // pixel depth

    tmp_u32 = 0;
    result = fwrite(&tmp_u32, sizeof(tmp_u32), 1, f); // compression method

    tmp_u32 = 0; //file_size - bitmap_data_offset;
    result = fwrite(&tmp_u32, sizeof(tmp_u32), 1, f); // image size
    tmp_32 = 0;
    result = fwrite(&tmp_32, sizeof(tmp_32), 1, f);
    result = fwrite(&tmp_32, sizeof(tmp_32), 1, f);
    tmp_u32 = (!plt) ? 0 : plt->num_ents; // plt entries
    result = fwrite(&tmp_u32, sizeof(tmp_u32), 1, f);
    tmp_u32 = 0;
    result = fwrite(&tmp_u32, sizeof(tmp_u32), 1, f);
    
    if(1 != result){
    	spice_error("write bmp header fail");
      return;
    }

    if (plt) {
        dump_palette(f, plt);
    }
    /* writing the data */
    for (i = 0; i < bitmap->data->num_chunks; i++) {
        SpiceChunk *chunk = &bitmap->data->chunk[i];
        int num_lines = chunk->len / bitmap->stride;
        for (j = 0; j < num_lines; j++) {
            dump_line(f, chunk->data + (j * bitmap->stride), n_pixel_bits, bitmap->x, row_size);
        }
    }
    fclose(f);
}
