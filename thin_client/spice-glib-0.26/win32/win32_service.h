#ifndef win32_service_h
#define win32_service_h

#include <glib.h>
#include <glib-object.h>
#include "win32-spicy.h"
#include "virt-viewer-file.h"
#include "virt-viewer-util.h"



# ifdef __cplusplus
extern "C"
{
#endif



   int spiceClientConnect (spice_connection *conn,const gchar *h, const gchar *p, const gchar *tp,
         const gchar *pw, const gchar *cf, GByteArray *cc,
         const gchar *cs, const gboolean sound);
   int spiceClientConnectVv (spice_connection *conn,VirtViewerFile *vv_file, const gboolean sound);
   int connectSession (spice_connection *conn);
   
#if 0
   int SpiceClientConnect( const gchar *h, const gchar *p,
         const gchar *tp, const gchar *pw, const gchar *cf, GByteArray *cc, const gchar *cs,
         const gboolean sound, VirtViewerFile *vv_file);
#endif
   int _CreateOvirtSession(const gchar *uri, const gchar *user, const gchar *password,
         const gchar *ovirt_ca_file, const gboolean sound, const gboolean sslStrict);

#ifdef __cplusplus
}
#endif



#endif
