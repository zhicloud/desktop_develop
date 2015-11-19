#include "win32_service.h"
#include "win32-spicy.h"
#include "spice_log.h"
#include "win32_spice_interface_priv.h"

extern struct Win32SpiceGloabInfo * global_wsgi;


void spice_session_setup(SpiceSession *session, const char *host, const char *port,
                            const char *tls_port, const char *password, const char *ca_file,
                            GByteArray *ca_cert, const char *cert_subj) {

    g_return_if_fail(SPICE_IS_SESSION(session));

    if (host)
        g_object_set(session, "host", host, NULL);
    // If we receive "-1" for a port, we assume the port is not set.
    if (port && strcmp (port, "-1") != 0)
       g_object_set(session, "port", port, NULL);
    if (tls_port && strcmp (tls_port, "-1") != 0)
        g_object_set(session, "tls-port", tls_port, NULL);
    if (password)
        g_object_set(session, "password", password, NULL);
    if (ca_file)
        g_object_set(session, "ca-file", ca_file, NULL);
    if (ca_cert)
        g_object_set(session, "ca", ca_cert, NULL);
    if (cert_subj)
        g_object_set(session, "cert-subject", cert_subj, NULL);
}


int spiceClientConnect (spice_connection *conn,const gchar *h, const gchar *p, const gchar *tp,
        const gchar *pw, const gchar *cf, GByteArray *cc,
        const gchar *cs, const gboolean sound)
{
    /*
    spice_connection *conn;

    conn = connection_new();
*/

    SPICE_LOG("%x  === %s === %s === %s \n",conn->session,h,p,pw);
    spice_session_setup(conn->session, h, p, tp, pw, cf, cc, cs);
    return connectSession(conn);
}

static void spice_session_setup_from_vv(VirtViewerFile *file, SpiceSession *session)
{
    g_return_if_fail(VIRT_VIEWER_IS_FILE(file));
    g_return_if_fail(SPICE_IS_SESSION(session));

    if (virt_viewer_file_is_set(file, "host")) {
        gchar *val = virt_viewer_file_get_host(file);
        g_object_set(G_OBJECT(session), "host", val, NULL);
        g_free(val);
    }

    if (virt_viewer_file_is_set(file, "port")) {
        gchar *port = g_strdup_printf("%d", virt_viewer_file_get_port(file));
        g_object_set(G_OBJECT(session), "port", port, NULL);
        g_free(port);
    }
    if (virt_viewer_file_is_set(file, "tls-port")) {
        gchar *tls_port = g_strdup_printf("%d", virt_viewer_file_get_tls_port(file));
        g_object_set(G_OBJECT(session), "tls-port", tls_port, NULL);
        g_free(tls_port);
    }
    if (virt_viewer_file_is_set(file, "password")) {
        gchar *val = virt_viewer_file_get_password(file);
        g_object_set(G_OBJECT(session), "password", val, NULL);
        g_free(val);
    }

    if (virt_viewer_file_is_set(file, "tls-ciphers")) {
        gchar *val = virt_viewer_file_get_tls_ciphers(file);
        g_object_set(G_OBJECT(session), "ciphers", val, NULL);
        g_free(val);
    }

    if (virt_viewer_file_is_set(file, "ca")) {
        gchar *ca = virt_viewer_file_get_ca(file);
        g_return_if_fail(ca != NULL);

        GByteArray *ba = g_byte_array_new_take((guint8 *)ca, strlen(ca) + 1);
        g_object_set(G_OBJECT(session), "ca", ba, NULL);
        g_byte_array_unref(ba);
    }

    if (virt_viewer_file_is_set(file, "host-subject")) {
        gchar *val = virt_viewer_file_get_host_subject(file);
        g_object_set(G_OBJECT(session), "cert-subject", val, NULL);
        g_free(val);
    }

    if (virt_viewer_file_is_set(file, "proxy")) {
        gchar *val = virt_viewer_file_get_proxy(file);
        g_object_set(G_OBJECT(session), "proxy", val, NULL);
        g_free(val);
    }

    if (virt_viewer_file_is_set(file, "enable-smartcard")) {
        g_object_set(G_OBJECT(session),
                     "enable-smartcard", virt_viewer_file_get_enable_smartcard(file), NULL);
    }

    if (virt_viewer_file_is_set(file, "enable-usbredir")) {
        g_object_set(G_OBJECT(session),
                     "enable-usbredir", virt_viewer_file_get_enable_usbredir(file), NULL);
    }

    if (virt_viewer_file_is_set(file, "color-depth")) {
        g_object_set(G_OBJECT(session),
                     "color-depth", virt_viewer_file_get_color_depth(file), NULL);
    }

    if (virt_viewer_file_is_set(file, "disable-effects")) {
        gchar **disabled = virt_viewer_file_get_disable_effects(file, NULL);
        g_object_set(G_OBJECT(session), "disable-effects", disabled, NULL);
        g_strfreev(disabled);
    }

    if (virt_viewer_file_is_set(file, "enable-usb-autoshare")) {
        //gboolean enabled = virt_viewer_file_get_enable_usb_autoshare(file);
        //SpiceGtkSession *gtk = spice_gtk_session_get(session);
        //g_object_set(G_OBJECT(gtk), "auto-usbredir", enabled, NULL);
    }

    if (virt_viewer_file_is_set(file, "secure-channels")) {
        gchar **channels = virt_viewer_file_get_secure_channels(file, NULL);
        g_object_set(G_OBJECT(session), "secure-channels", channels, NULL);
        g_strfreev(channels);
    }

    if (virt_viewer_file_is_set(file, "disable-channels")) {
        //DEBUG_LOG("FIXME: disable-channels is not supported atm");
    }
}



int spiceClientConnectVv (spice_connection *conn,VirtViewerFile *vv_file, const gboolean sound)
{
    /*
    spice_connection *conn;
    soundEnabled = sound;
    conn = connection_new();*/
    spice_session_setup_from_vv(vv_file, conn->session);
     return connectSession(conn);
}


int connectSession (spice_connection *conn)
{
    connection_connect(conn);
    return 1;
}


#if 0
int SpiceClientConnect(const gchar *h, const gchar *p,
        const gchar *tp, const gchar *pw, const gchar *cf, GByteArray *cc, const gchar *cs,
        const gboolean sound, VirtViewerFile *vv_file)
{
    if(vv_file)
    {
        return spiceClientConnectVv(vv_file,sound);
    
    }
    else
    {
        return spiceClientConnect(h,p,tp,pw,cf,cc,cs,sound);
    
    }

}
#endif


int _CreateOvirtSession(const gchar *uri, const gchar *user, const gchar *password,
        const gchar *ovirt_ca_file, const gboolean sound, const gboolean sslStrict)
{

}

