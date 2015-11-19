/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
   Copyright (C) 2013 Iordan Iordanov
   Copyright (C) 2010 Red Hat, Inc.

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
#include "config.h"
#endif
#include <glib/gi18n.h>

#include <sys/stat.h>
#define SPICY_C
#include "glib-compat.h"
#include "win32-spice-widget.h"
#include "spice-audio.h"
#include "spice-common.h"
#include "spice-cmdline.h"
#include "win32-spicy.h"
#include "spice_log.h"
#include "win32_spice_interface_priv.h"
#include "spice-types.h"
//#include "spice-session-priv.h"
//#include "android-service.h"

extern struct Win32SpiceGloabInfo * global_wsgi;
G_DEFINE_TYPE (SpiceWindow, spice_window, G_TYPE_OBJECT);

static void connection_destroy(spice_connection *conn);

extern bool display2conn(spice_connection** tmp_conn,SpiceDisplay *display);
extern struct Win32SpiceGloabInfo * global_wsgi;

static void usb_connect_failed(GObject               *object,
                               SpiceUsbDevice        *device,
                               GError                *error,
                               gpointer               data)
{
    SPICE_LOG("\n");
    if (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_CANCELLED)
         return;    
    g_rec_mutex_lock(&global_wsgi->rec_mutex_conn);
    struct spice_connection* tmp_conn = NULL;
    tmp_conn = (spice_connection*) data;
    if(tmp_conn->callback)
    {
        SPICE_USBConnectFaild message;
        memset(&message,0,sizeof(message));
        strcpy(message.err_message,error->message);
        tmp_conn->callback[CB_USB_CONNECT_FAILD].lcb._ud = &message;
        if(tmp_conn->callback[CB_USB_CONNECT_FAILD].lcb._cb)
        {
            tmp_conn->callback[CB_USB_CONNECT_FAILD].lcb._cb(
                    tmp_conn->callback[CB_USB_CONNECT_FAILD].lcb._ctx,
                    tmp_conn->callback[CB_USB_CONNECT_FAILD].lcb._ud);
        }
    }
    g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);
    SPICE_LOG("====================end\n");

}

static void usb_device_error(GObject               *object,
                             SpiceUsbDevice        *device,
                             GError                *error,
                             gpointer               data)
{
    SPICE_LOG("\n");
 
    g_rec_mutex_lock(&global_wsgi->rec_mutex_conn);
    struct spice_connection* tmp_conn = NULL;
    tmp_conn = (spice_connection*) data;
    if(tmp_conn->callback)
    {
        SPICE_USBDeviceErr message;
        memset(&message,0,sizeof(message));
        strcpy(message.err_message,error->message);
        tmp_conn->callback[CB_USB_DEVICE_ERROR].lcb._ud = &message;
        if(tmp_conn->callback[CB_USB_DEVICE_ERROR].lcb._cb)
        {
            tmp_conn->callback[CB_USB_DEVICE_ERROR].lcb._cb(
                    tmp_conn->callback[CB_USB_DEVICE_ERROR].lcb._ctx,
                    tmp_conn->callback[CB_USB_DEVICE_ERROR].lcb._ud);
        }
    }

    g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);
    SPICE_LOG("====================end\n");

}

static void device_added_cb(SpiceUsbDeviceManager *manager, 
                            SpiceUsbDevice *device, 
                            gpointer data)
{
    SPICE_LOG("\n");
 
    g_rec_mutex_lock(&global_wsgi->rec_mutex_conn);
    struct spice_connection* tmp_conn = NULL;
    tmp_conn = (spice_connection*) data;
    if(tmp_conn->callback)
    {
        gchar *desc;
        SPICE_USBAddDev dev;
        memset(&dev,0,sizeof(dev));
        dev.device = device;
        desc = spice_usb_device_get_description(device,NULL);
        strncpy(dev.desc,desc,sizeof(dev.desc) - 1);
        g_free(desc);
        
	SpiceUsbDeviceManager *manager;
        GError *error = NULL;
	manager = spice_usb_device_manager_get(tmp_conn->session, &error);
        if (error)
        {
            
            g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);
            g_clear_error(&error);
            return;
        }
        if (manager == NULL)
        {
	    
            g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);
            return;  
        }
        if (spice_usb_device_manager_is_device_connected(manager,device))
        {
            dev.is_connect = 1;
        }
       
        tmp_conn->callback[CB_USB_ADD_DEV].lcb._ud = &dev;
        if(tmp_conn->callback[CB_USB_ADD_DEV].lcb._cb)
        {
            tmp_conn->callback[CB_USB_ADD_DEV].lcb._cb(
                    tmp_conn->callback[CB_USB_ADD_DEV].lcb._ctx,
                    tmp_conn->callback[CB_USB_ADD_DEV].lcb._ud);
        }
    }

    g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);
    SPICE_LOG("====================end\n");


}

static void device_removed_cb(SpiceUsbDeviceManager *manager,
                              SpiceUsbDevice *device, 
                              gpointer data)
{
    SPICE_LOG("\n");
 
    g_rec_mutex_lock(&global_wsgi->rec_mutex_conn);
    struct spice_connection* tmp_conn = NULL;
    tmp_conn = (spice_connection*) data;
    
    if(tmp_conn->callback)
    {
        SPICE_USBRemove dev = {device};
        tmp_conn->callback[CB_USB_REMOVE].lcb._ud = &dev;
        if(tmp_conn->callback[CB_USB_REMOVE].lcb._cb)
        {
            tmp_conn->callback[CB_USB_REMOVE].lcb._cb(
                    tmp_conn->callback[CB_USB_REMOVE].lcb._ctx,
                    tmp_conn->callback[CB_USB_REMOVE].lcb._ud);
        }
    }

    g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);
    SPICE_LOG("====================end\n");

}



/* ------------------------------------------------------------------ */

static SpiceWindow *create_spice_window(spice_connection *conn, SpiceChannel *channel, int id)
{
    SPICE_LOG(" \n");
    SpiceWindow *win;

    win = g_new0 (SpiceWindow, 1);
    win->id = id;
    //win->monitor_id = monitor_id;
    win->conn = conn;
    win->display_channel = channel;

    win->spice = (spice_display_new(conn->session, id));
    //add by lcx for remember global_display
    conn->global_display = win->spice;
    return win;
}

static void destroy_spice_window(spice_connection *conn,SpiceWindow *win)
{
    if (win == NULL)
        return;

    SPICE_DEBUG("destroy window (#%d:%d)", win->id, win->monitor_id);
    //g_object_unref(win->ag);
    //g_object_unref(win->ui);
    //gtk_widget_destroy(win->toplevel);

    g_rec_mutex_lock(&global_wsgi->rec_mutex_conn);

    conn->global_display  = NULL;

    g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);
    g_object_unref(win->spice);

    g_object_unref(win);
}

/* ------------------------------------------------------------------ */

static void main_channel_event(SpiceChannel *channel, SpiceChannelEvent event,
                               gpointer data)
{
    spice_connection *conn = data;
    char password[64];
    int rc = -1;
    SPICE_LOG("\n");
    g_rec_mutex_lock(&global_wsgi->rec_mutex_conn);
    if(!conn->callback)
    {
        g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);
        return ;
    }
    SpiceCallBacks * main_cb  = &conn->callback[CB_MAIN_CHANNEL_EVENT];
    SPICE_MainChannelEvent smet;

    switch (event) {
    case SPICE_CHANNEL_OPENED:
   
        if(SPICE_IS_MAIN_CHANNEL(channel))
        {
            g_message("main channel: opened");
            smet.et = LCX_SPICE_CHANNEL_OPENED; 

            main_cb->lcb._ud = &smet;
            if(main_cb->lcb._cb)
            {
                 main_cb->lcb._cb(main_cb->lcb._ctx,main_cb->lcb._ud);
            }
        }
        break;
    case SPICE_CHANNEL_SWITCHING:
        if(SPICE_IS_MAIN_CHANNEL(channel))
        {
            g_message("main channel: switching host");
            //SPICE_LOG("main channel: switching host\n");        
            smet.et = LCX_SPICE_CHANNEL_SWITCHING; 
            
            main_cb->lcb._ud = &smet;
            if(main_cb->lcb._cb)
            {
                 main_cb->lcb._cb(main_cb->lcb._ctx,main_cb->lcb._ud);
            }
        }
        break;
    case SPICE_CHANNEL_CLOSED:
        /* this event is only sent if the channel was succesfully opened before */

        //g_message("main channel: closed");
        //SPICE_LOG("main channel: closed\n");
        if(SPICE_IS_MAIN_CHANNEL(channel) || SPICE_IS_DISPLAY_CHANNEL(channel) || SPICE_IS_INPUTS_CHANNEL(channel) || SPICE_IS_CURSOR_CHANNEL(channel))
        {
            printf("main channel: closed yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\n");
            smet.et = LCX_SPICE_CHANNEL_CLOSED; 

            main_cb->lcb._ud = &smet;
            if(main_cb->lcb._cb)
            {
                printf("main channel: closed dddddddddddddddddddddddddddddddddddddddddddddddddddd\n");
                main_cb->lcb._cb(main_cb->lcb._ctx,main_cb->lcb._ud);
            }
            for (int i = 0; i < CB_NULL; i++)
            {
                SpiceCallBacks * _cb  = &conn->callback[i];
                _cb->lcb._cb = NULL;
            }

        }

        g_signal_handlers_disconnect_by_func( channel,
                main_channel_event,conn);                                 
        //connection_disconnect(conn);
        break;
    case SPICE_CHANNEL_ERROR_IO:
        {
            g_message("channel: error io");
            if(SPICE_IS_MAIN_CHANNEL(channel) || SPICE_IS_DISPLAY_CHANNEL(channel) || SPICE_IS_INPUTS_CHANNEL(channel) || SPICE_IS_CURSOR_CHANNEL(channel))
            {
                printf("main channel: error io yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\n");
                smet.et = LCX_SPICE_CHANNEL_ERROR_IO; 

                main_cb->lcb._ud = &smet;
                if(main_cb->lcb._cb)
                {
                    printf("main channel: error io dddddddddddddddddddddddddddddddddddddddddddddddddddd\n");
                    main_cb->lcb._cb(main_cb->lcb._ctx,main_cb->lcb._ud);

                }
                for (int i = 0; i < CB_NULL; i++)
                {
                    SpiceCallBacks * _cb  = &conn->callback[i];
                    _cb->lcb._cb = NULL;
                }


            }
            g_signal_handlers_disconnect_by_func( channel,
                    main_channel_event,conn);                                 

            break;
        }
    case SPICE_CHANNEL_ERROR_TLS:
    case SPICE_CHANNEL_ERROR_LINK:
    case SPICE_CHANNEL_ERROR_CONNECT:
        {

            g_message("main channel: failed to connect");
            if(SPICE_IS_MAIN_CHANNEL(channel) || SPICE_IS_DISPLAY_CHANNEL(channel) || SPICE_IS_INPUTS_CHANNEL(channel) || SPICE_IS_CURSOR_CHANNEL(channel))
            {
                printf("main channel: error tls yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\n");


                smet.et = (event == SPICE_CHANNEL_ERROR_TLS) ? LCX_SPICE_CHANNEL_ERROR_TLS : 
                    (event == SPICE_CHANNEL_ERROR_LINK) ? LCX_SPICE_CHANNEL_ERROR_LINK : LCX_SPICE_CHANNEL_ERROR_CONNECT; 

                main_cb->lcb._ud = &smet;
                if(main_cb->lcb._cb)
                {
                    printf("main channel: error tls dddddddddddddddddddddddddddddddddddddddddddddddddddd\n");
                    main_cb->lcb._cb(main_cb->lcb._ctx,main_cb->lcb._ud);
                }
                for (int i = 0; i < CB_NULL; i++)
                {
                    SpiceCallBacks * _cb  = &conn->callback[i];
                    _cb->lcb._cb = NULL;
                }

            }

            g_signal_handlers_disconnect_by_func( channel,
                    main_channel_event,conn);                                 
            break;
        }
        /* //rc = connect_dialog(conn->session); if (rc == 0) {
           connection_connect(conn);
           } else {
           connection_disconnect(conn);
           }
           */
    case SPICE_CHANNEL_ERROR_AUTH:
        g_warning("main channel: auth failure (wrong password?)");
        SPICE_LOG("main channel: auth failure (wrong password?\n");
        strcpy(password, "");
        /* FIXME i18 */
        //rc = ask_user(NULL, _("Authentication"),
        //              _("Please enter the spice server password"),
        //              password, sizeof(password), true);

        smet.et = LCX_SPICE_CHANNEL_ERROR_AUTH; 
        main_cb->lcb._ud = &smet;
        if(main_cb->lcb._cb)
        {
            main_cb->lcb._cb(main_cb->lcb._ctx,main_cb->lcb._ud);
        }
        /*
           if (rc == 0) {
           g_object_set(conn->session, "password", password, NULL);
           connection_connect(conn);
           } else {
           connection_disconnect(conn);
           }
           */

        g_signal_handlers_disconnect_by_func( channel,
                main_channel_event,conn);                                 
        break;
    default:
        /* TODO: more sophisticated error handling */
        g_warning("unknown main channel event: %d", event);
        SPICE_LOG("unknown main channel event: %d\n",event);
        smet.et = LCX_SPICE_CHANNEL_NONE;
        main_cb->lcb._ud = &smet;
        if(main_cb->lcb._cb)
        {
            main_cb->lcb._cb(main_cb->lcb._ctx,main_cb->lcb._ud);
        }
        /* connection_disconnect(conn); */
        break;
    }
    g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);
    /*
       main_cb->lcb._ud = &smet;
       if(main_cb->lcb._cb)
       {
       main_cb->lcb._cb(main_cb->lcb._ctx,main_cb->lcb._ud);
       }
       g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);
       */
}

static void channel_new(SpiceSession *s, SpiceChannel *channel, gpointer data)
{
    spice_connection *conn = data;
    int id;

    SPICE_LOG("\n");
    g_object_get(channel, "channel-id", &id, NULL);
    conn->channels++;
    SPICE_DEBUG("new channel (#%d)", id);
    SPICE_LOG("new channel (#%d)\n",id);

    g_signal_connect(channel, "channel-event",
            G_CALLBACK(main_channel_event), conn);


    if (SPICE_IS_MAIN_CHANNEL(channel)) {
        //SPICE_LOG("new main channel\n");
        //SPICE_DEBUG("new main channel");
        conn->main = SPICE_MAIN_CHANNEL(channel);
        //        g_signal_connect(channel, "channel-event",
        //               G_CALLBACK(main_channel_event), conn);

        //g_signal_connect(channel, "main-mouse-update",
        //                 G_CALLBACK(main_mouse_update), conn);
        //g_signal_connect(channel, "main-agent-update",
        //                 G_CALLBACK(main_agent_update), conn);
        //main_mouse_update(channel, conn);
        //main_agent_update(channel, conn);
        //SPICE_LOG("111111111111111111111111111111111111111111111111\n");
    }

    if (SPICE_IS_DISPLAY_CHANNEL(channel)) {
        if (id >= SPICE_N_ELEMENTS(conn->wins))
        {
            return;
        }
        if (conn->wins[id] != NULL)
        {
            return;
        }
        SPICE_LOG("new display channel\n");
        SPICE_DEBUG("new display channel (#%d)", id);
        conn->wins[id] = create_spice_window(conn, channel, id);
        //g_signal_connect(channel, "display-mark",
        //                 G_CALLBACK(display_mark), conn->wins[id]);
        //update_auto_usbredir_sensitive(conn);
    }

    if (SPICE_IS_INPUTS_CHANNEL(channel)) {
        SPICE_LOG("new inputs channel\n");
        SPICE_DEBUG("new inputs channel");
        //g_signal_connect(channel, "inputs-modifiers",
        //                 G_CALLBACK(inputs_modifiers), conn);
    }
    if (SPICE_IS_PLAYBACK_CHANNEL(channel)) {
        SPICE_LOG("new audio channel\n");
        SPICE_DEBUG("new audio channel");
        conn->audio = spice_audio_get(s, NULL);
    }

    /*
       if (soundEnabled && SPICE_IS_PLAYBACK_CHANNEL(channel)) {
       SPICE_DEBUG("new audio channel");
       conn->audio = spice_audio_get(s, NULL);
       }
       */

    //if (SPICE_IS_USBREDIR_CHANNEL(channel)) {
    //    update_auto_usbredir_sensitive(conn);
    //}

    //if (SPICE_IS_PORT_CHANNEL(channel)) {
    //    g_signal_connect(channel, "notify::port-opened",
    //                     G_CALLBACK(port_opened), conn);
    //    g_signal_connect(channel, "port-data",
    //                     G_CALLBACK(port_data), conn);
    //    spice_channel_connect(channel);
    //}
}

static void channel_destroy(SpiceSession *s, SpiceChannel *channel, gpointer data)
{

    spice_connection *conn = data;
    int id;

    //printf("[file :%s][fun : %s][line : %d] ======  !!!!!!!!!!!!!!!!!!!!!\n",__FILE__,__FUNCTION__,__LINE__);

    g_object_get(channel, "channel-id", &id, NULL);
   
    if (SPICE_IS_MAIN_CHANNEL(channel))
    {
        conn->main = NULL;
    }

    if (SPICE_IS_DISPLAY_CHANNEL(channel)) {
        //printf("[file :%s][fun : %s][line : %d] SPICE_IS_DISPLAY_CHANNEL\n",__FILE__,__FUNCTION__,__LINE__);
        if (id >= SPICE_N_ELEMENTS(conn->wins))
            return;
        if (conn->wins[id] == NULL)
            return;
        SPICE_DEBUG("zap display channel (#%d)", id);
        destroy_spice_window(conn,conn->wins[id]);
        conn->wins[id] = NULL;
    }

    g_signal_handlers_disconnect_by_func(channel, main_channel_event,conn);                                 
   
    conn->channels--;

    // char buf[100];
    // snprintf (buf, 100, "Number of channels: %d", conn->channels);
    //printf("[file :%s][fun : %s][line : %d] %s \n",__FILE__,__FUNCTION__,__LINE__,buf);

    if (conn->channels > 0) {
        return;
    }
    connection_destroy(conn);
}

static void migration_state(GObject *session,
        GParamSpec *pspec, gpointer data)
{
    SpiceSessionMigration mig;

    g_object_get(session, "migration-state", &mig, NULL);
    if (mig == SPICE_SESSION_MIGRATION_SWITCHING)
        g_message("migrating session");
}

spice_connection *connection_new(void)
{
    spice_connection *conn = NULL;
    SpiceUsbDeviceManager *manager;

    SPICE_LOG("\n");
    conn = g_new0(spice_connection, 1);
    SPICE_LOG("========================  %x \n",conn);
    //sleep(2);
    conn->session = spice_session_new();
    //conn->gtk_session = spice_gtk_session_get(conn->session);
    g_signal_connect(conn->session, "channel-new",
                     G_CALLBACK(channel_new), conn);
    g_signal_connect(conn->session, "channel-destroy",
                     G_CALLBACK(channel_destroy), conn);
    g_signal_connect(conn->session, "notify::migration-state",
                     G_CALLBACK(migration_state), conn);
    
    manager = spice_usb_device_manager_get(conn->session, NULL);
    if (manager) {
        g_signal_connect(manager, "auto-connect-failed",
                         G_CALLBACK(usb_connect_failed), conn);
        g_signal_connect(manager, "device-error",
                         G_CALLBACK(usb_device_error), conn);
        g_signal_connect(manager,"device-added",
                         G_CALLBACK(device_added_cb),conn);
        g_signal_connect(manager,"device-removed",
                         G_CALLBACK(device_removed_cb),conn);
    }

    //connections++;
    //SPICE_DEBUG("%s (%d)", __FUNCTION__, connections);
    return conn;
}

static void connection_destroy(spice_connection *conn)
{
    SpiceUsbDeviceManager *manager = spice_usb_device_manager_get(conn->session, NULL);
    if(manager)
    {
        g_signal_handlers_disconnect_by_func( manager,
                device_added_cb,conn);                                 
        g_signal_handlers_disconnect_by_func( manager,
                usb_connect_failed,conn);
        g_signal_handlers_disconnect_by_func( manager,
                usb_device_error,conn);
        g_signal_handlers_disconnect_by_func( manager,
                device_removed_cb,conn);
    }

    g_signal_handlers_disconnect_by_func(conn->session,
            migration_state, conn);
    g_signal_handlers_disconnect_by_func(conn->session,
            channel_destroy, conn);
    g_signal_handlers_disconnect_by_func(conn->session,
            channel_new, conn);

    g_object_unref(conn->session);
    conn->session = NULL;
   
    free(conn);
    conn = NULL;
}

void connection_connect(spice_connection *conn)
{
    conn->disconnecting = false;
    SPICE_LOG("\n");
    spice_session_connect(conn->session);
}

void connection_disconnect(spice_connection *conn)
{
    if (conn->disconnecting)
        return;
    conn->disconnecting = true;
    spice_session_disconnect(conn->session);
}
