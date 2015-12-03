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
#ifndef MAIN_DISPATCHER_H
#define MAIN_DISPATCHER_H

#include <spice.h>
#include "red_channel.h"

void main_dispatcher_channel_event(int event, SpiceChannelEventInfo *info);
void main_dispatcher_seamless_migrate_dst_complete(RedClient *client);
void main_dispatcher_set_mm_time_latency(RedClient *client, uint32_t latency);
/*
 * Disconnecting the client is always executed asynchronously,
 * in order to protect from expired references in the routines
 * that triggered the client destruction.
 */
void main_dispatcher_client_disconnect(RedClient *client);

void main_dispatcher_init(SpiceCoreInterface *core);

#endif //MAIN_DISPATCHER_H
