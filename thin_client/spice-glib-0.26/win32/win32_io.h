#ifndef win32_io_h
#define win32_io_h


#include <glib.h>
#include <glib-object.h>
#include "win32-spice-widget-priv.h"

# ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned char uchar;

void uiCallbackInvalidate (SpiceDisplay *d, gint x, gint y, gint w, gint h);
void uiCallbackSettingsChanged (SpiceDisplay *display,gint instance, gint width, gint height, gint bpp);
void SpiceRequestResolution_io(gint x, gint y,SpiceDisplay* display);
gint get_display_id(SpiceDisplay *display);
int win32key2spice (int keycode);
int button_update_mask (int button, gboolean down); 
#ifdef __cplusplus
}
#endif

#endif
