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
#ifndef UTILS_H_
# define UTILS_H_

#include <time.h>
#include <stdint.h>

typedef int64_t red_time_t;

/* FIXME: consider g_get_monotonic_time (), but in microseconds */
static inline red_time_t red_get_monotonic_time(void)
{
    struct timespec time;

    clock_gettime(CLOCK_MONOTONIC, &time);
    return (red_time_t) time.tv_sec * (1000 * 1000 * 1000) + time.tv_nsec;
}

#endif /* UTILS_H_ */
