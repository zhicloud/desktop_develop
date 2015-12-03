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

#ifndef _H_STAT
#define _H_STAT

#include <stdint.h>

typedef uint32_t StatNodeRef;
#define INVALID_STAT_REF (~(StatNodeRef)0)

#ifdef RED_STATISTICS
StatNodeRef stat_add_node(StatNodeRef parent, const char *name, int visible);
void stat_remove_node(StatNodeRef node);
uint64_t *stat_add_counter(StatNodeRef parent, const char *name, int visible);
void stat_remove_counter(uint64_t *counter);

#define stat_inc_counter(counter, value) {  \
    if (counter) {                          \
        *(counter) += (value);              \
    }                                       \
}

#else
#define stat_add_node(p, n, v) INVALID_STAT_REF
#define stat_remove_node(n)
#define stat_add_counter(p, n, v) NULL
#define stat_remove_counter(c)
#define stat_inc_counter(c, v)
#endif /* RED_STATISTICS */

typedef unsigned long stat_time_t;

static inline stat_time_t stat_now(clockid_t clock_id)
{
    struct timespec ts;

    clock_gettime(clock_id, &ts);
    return ts.tv_nsec + ts.tv_sec * 1000 * 1000 * 1000;
}

static inline double stat_cpu_time_to_sec(stat_time_t time)
{
    return (double)time / (1000 * 1000 * 1000);
}

typedef struct stat_info_s {
    const char *name;
    clockid_t clock;
    uint32_t count;
    stat_time_t max;
    stat_time_t min;
    stat_time_t total;
#ifdef COMPRESS_STAT
    uint64_t orig_size;
    uint64_t comp_size;
#endif
} stat_info_t;

static inline void stat_reset(stat_info_t *info)
{
    info->count = info->max = info->total = 0;
    info->min = ~(stat_time_t)0;
#ifdef COMPRESS_STAT
    info->orig_size = info->comp_size = 0;
#endif
}

#ifdef COMPRESS_STAT
static inline void stat_compress_init(stat_info_t *info, const char *name)
{
    info->name = name;
    stat_reset(info);
}

static inline void stat_compress_add(stat_info_t *info,
                                     stat_time_t start, int orig_size,
                                     int comp_size)
{
    stat_time_t time;
    ++info->count;
    time = stat_now(info->clock) - start;
    info->total += time;
    info->max = MAX(info->max, time);
    info->min = MIN(info->min, time);
    info->orig_size += orig_size;
    info->comp_size += comp_size;
}

static inline double stat_byte_to_mega(uint64_t size)
{
    return (double)size / (1000 * 1000);
}

#else
#define stat_compress_init(a, b)
#define stat_compress_add(a, b, c, d)
#endif

#ifdef RED_WORKER_STAT
static inline void stat_init(stat_info_t *info, const char *name, clockid_t clock)
{
    info->name = name;
    info->clock = clock;
    stat_reset(info);
}

static inline void stat_add(stat_info_t *info, stat_time_t start)
{
    stat_time_t time;
    ++info->count;
    time = stat_now(info->clock) - start;
    info->total += time;
    info->max = MAX(info->max, time);
    info->min = MIN(info->min, time);
}

#else
#define stat_add(a, b)
#define stat_init(a, b, c)
#endif /* RED_WORKER_STAT */

#endif /* _H_STAT */
