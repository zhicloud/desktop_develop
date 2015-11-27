#ifndef win32_spice_interface_priv_h
#define win32_spice_interface_priv_h


#include "win32_spice_interface_types.h"
#include "win32-spicy.h"
extern int connections;
extern GMainLoop            *mainloop;
extern spice_connection*  global_conn;

#define MAX_MAIN_LOOP 1
#define MAX_SPICE_CONNECT 10


typedef struct main_loop_context
{
   GMainLoop* mainloop;
   GMainContext *context;
   GThread * thread;
}main_loop_context;

typedef struct priv_spice_connection
{
   //priv_bmp bmp_info;
   //SPICE_Invalidate invalidate_info;
   struct spice_connection* conn;
   SpiceCallBacks callbacks[CB_NULL];
   GThread * thread;
   char default_usb_on_connect[1024];
   char default_usb_connect[1024];
   int default_usb_enable;
}priv_spice_connection;


typedef struct Win32SpiceGloabInfo
{
   struct main_loop_context* MainloopList[MAX_MAIN_LOOP];
   priv_spice_connection* SpiceConnList[MAX_SPICE_CONNECT]; 
   int connections;
   GRecMutex rec_mutex_conn;
   GRecMutex rec_mutex_loop;
}Win32SpiceGloabInfo;

#endif
