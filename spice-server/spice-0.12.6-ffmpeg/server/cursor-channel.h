/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
#ifndef CURSOR_CHANNEL_H_
# define CURSOR_CHANNEL_H_

#include "spice.h"
#include "reds.h"
#include "red_worker.h"
#include "red_parse_qxl.h"
#include "cache-item.h"
#include "stat.h"

typedef struct CursorChannel CursorChannel;
typedef struct CursorChannelClient CursorChannelClient;

#define CURSOR_CHANNEL_CLIENT(Client) ((CursorChannelClient*)(Client))

CursorChannel*       cursor_channel_new         (RedWorker *worker);
void                 cursor_channel_disconnect  (CursorChannel *cursor_channel);
void                 cursor_channel_reset       (CursorChannel *cursor);
void                 cursor_channel_init        (CursorChannel *cursor, CursorChannelClient* client);
void                 cursor_channel_process_cmd (CursorChannel *cursor, RedCursorCmd *cursor_cmd,
                                                 uint32_t group_id);
void                 cursor_channel_set_mouse_mode(CursorChannel *cursor, uint32_t mode);

CursorChannelClient* cursor_channel_client_new(CursorChannel *cursor,
                                               RedClient *client, RedsStream *stream,
                                               int mig_target,
                                               uint32_t *common_caps, int num_common_caps,
                                               uint32_t *caps, int num_caps);
void                 cursor_channel_client_migrate(CursorChannelClient* client);

#endif /* CURSOR_CHANNEL_H_ */
