#include "win32_io.h"
#include <stdlib.h>
#include <stdio.h>

#include "spice_log.h"
#include "win32_spice_interface_priv.h"
#include "win32keymap.h"

//#include "create_bmp.h"

extern struct Win32SpiceGloabInfo * global_wsgi;
void UpdateBitmap (SpiceDisplay* display,char* bitmap, gint x, gint y, gint width, gint height) 
{ 

	uchar* pixels;
	SpiceDisplayPrivate *d = SPICE_DISPLAY_GET_PRIVATE(display);
	if(!bitmap)
	{
		return;
	}
	pixels = bitmap;

	int slen = d->width * 4;
	int offset = (slen * y) + (x * 4);
	uchar *source = d->data;
	uchar *sourcepix = (uchar*) &source[offset];
	uchar *destpix   = (uchar*) &pixels[offset];

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width * 4; j += 4) {
			destpix[j + 0] = sourcepix[j + 0];
			destpix[j + 1] = sourcepix[j + 1];
			destpix[j + 2] = sourcepix[j + 2];
		}
		sourcepix = sourcepix + slen;
		destpix   = destpix + slen;
	}
}

void smallUpdateBitmap (SpiceDisplay* display,char* bitmap, gint x, gint y, gint width, gint height)
{

	uchar* pixels;
	SpiceDisplayPrivate *d = SPICE_DISPLAY_GET_PRIVATE(display);
	if(!bitmap)
	{
		return;
	}                                                                                                                                   
	pixels = bitmap;

	int slen = d->width * 4;
	int slen1 = width * 4;                                                                                                              
	int offset = (slen * y) + (x * 4);
	uchar *source = d->data;
	uchar *sourcepix = (uchar*) &source[offset];
	uchar *destpix   = (uchar*) pixels;

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width * 4; j += 4) {
			destpix[j + 0] = sourcepix[j + 0];
			destpix[j + 1] = sourcepix[j + 1];
			destpix[j + 2] = sourcepix[j + 2];
		}
		sourcepix = sourcepix + slen;
		destpix   = destpix + slen1;
	}
}




int win32key2spice (int keycode)
{                                                                                                                                                                     
	int newKeyCode = keymap_win322xtkbd[keycode];
	/*
	 *      char buf[100];
	 *           snprintf (buf, 100, "Converted win32 key: %d to linux key: %d", keycode, newKeyCode);
	 *                __android_log_write(6, "android-io", buf);
	 *                     */
	return newKeyCode;
}

int button_update_mask (int button, gboolean down) {                                                                                                                  
	static int mask;
	int update = 0;
	if (button == SPICE_MOUSE_BUTTON_LEFT)
		update = SPICE_MOUSE_BUTTON_MASK_LEFT;
	else if (button == SPICE_MOUSE_BUTTON_MIDDLE)
		update = SPICE_MOUSE_BUTTON_MASK_MIDDLE;
	else if (button == SPICE_MOUSE_BUTTON_RIGHT)
		update = SPICE_MOUSE_BUTTON_MASK_RIGHT;
	if (down) {
		mask |= update;
	} else {
		mask &= ~update;
	}
	return mask;
}
extern bool display2conn(spice_connection** tmp_conn,SpiceDisplay *display);

void free_bitmap(void* p)
{
    free(p);
}

void uiCallbackInvalidate (SpiceDisplay *display, gint x, gint y, gint w, gint h)
{
	g_rec_mutex_lock(&global_wsgi->rec_mutex_conn);
	int i = 0;

	struct spice_connection* tmp_conn = NULL;

	if(display2conn(&tmp_conn,display))
	{ 
		SpiceDisplayPrivate *d = SPICE_DISPLAY_GET_PRIVATE(display);

		if(tmp_conn->callback)
		{
			//UpdateBitmap(display,d->screem.bitmap,x,y,w,h);
			//SPICE_Invalidate pic = {x,y,w,h,d->screem.bitmap};
			//SPICE_Invalidate pic = {x,y,w,h,d->data};
                        char * tmp = (char*)malloc(w * h * 4);
                        memset(tmp,0,w * h * 4);
                        smallUpdateBitmap(display,tmp,x,y,w,h);
                        SPICE_Invalidate pic = {x,y,w,h,tmp,free_bitmap};
			tmp_conn->callback[CB_INVALIDATE].lcb._ud = &pic;
			if(tmp_conn->callback[CB_INVALIDATE].lcb._cb)
			{
				tmp_conn->callback[CB_INVALIDATE].lcb._cb(
						tmp_conn->callback[CB_INVALIDATE].lcb._ctx,
						tmp_conn->callback[CB_INVALIDATE].lcb._ud);
			}
         else
         {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! memory leak   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1\n");
            free(tmp);
         }
			//free(tmp);
		}
	}
	g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);
}

void uiCallbackSettingsChanged (SpiceDisplay *display,gint instance, gint width, gint height, gint bpp)
{
	//global_wsgi
	g_rec_mutex_lock(&global_wsgi->rec_mutex_conn);
	int i = 0;

	struct spice_connection* tmp_conn = NULL;

	if(display2conn(&tmp_conn,display))
	{ 
		SpiceDisplayPrivate *d = SPICE_DISPLAY_GET_PRIVATE(display);
		d->screem.x = 0;
		d->screem.y = 0;
		d->screem.w = width;
		d->screem.h = height;
		if(d->screem.bitmap)
		{
			g_free(d->screem.bitmap);
		}
		//d->screem.bitmap = g_new0(,width * height * bpp * 2);
		d->screem.bitmap = (unsigned char *)malloc(sizeof(char) * width * height * bpp * 2);
		if(tmp_conn->callback)
		{
			SPICE_SettringChange change ={width,height,bpp};
			tmp_conn->callback[CB_SETTING_CHANGE].lcb._ud = &change;
			if(tmp_conn->callback[CB_SETTING_CHANGE].lcb._cb)
			{
				tmp_conn->callback[CB_SETTING_CHANGE].lcb._cb(
						tmp_conn->callback[CB_SETTING_CHANGE].lcb._ctx,
						tmp_conn->callback[CB_SETTING_CHANGE].lcb._ud);
			}
		}
	}

	g_rec_mutex_unlock(&global_wsgi->rec_mutex_conn);


}

void SpiceRequestResolution_io(gint x, gint y,SpiceDisplay* display)
{
	SpiceDisplayPrivate *d = SPICE_DISPLAY_GET_PRIVATE(display);
        spice_main_set_display_enabled(d->main, -1, FALSE);
	spice_main_update_display(d->main, get_display_id(display), 0, 0, x, y, TRUE);
	spice_main_set_display_enabled(d->main, -1, TRUE);
        spice_main_send_monitor_config(d->main);

}
