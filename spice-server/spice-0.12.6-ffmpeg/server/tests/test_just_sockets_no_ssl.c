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
#include <config.h>
#include <sys/select.h>
#include <spice.h>
#include "basic_event_loop.h"

int main(void)
{
    SpiceServer *server = spice_server_new();
    SpiceCoreInterface *core = basic_event_loop_init();

    spice_server_set_port(server, 5912);
    spice_server_set_noauth(server);
    spice_server_init(server, core);

    basic_event_loop_mainloop();

    return 0;
}
