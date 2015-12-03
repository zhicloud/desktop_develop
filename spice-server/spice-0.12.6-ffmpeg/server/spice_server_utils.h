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
#ifndef H_SPICE_SERVER_UTIL
#define H_SPICE_SERVER_UTIL

#include <unistd.h>
#include <glib.h>

static inline void set_bit(int index, uint32_t *addr)
{
    uint32_t mask = 1 << index;
    __sync_or_and_fetch(addr, mask);
}

static inline void clear_bit(int index, uint32_t *addr)
{
    uint32_t mask = ~(1 << index);
    __sync_and_and_fetch(addr, mask);
}

static inline int test_bit(int index, uint32_t val)
{
    return val & (1u << index);
}

#endif
