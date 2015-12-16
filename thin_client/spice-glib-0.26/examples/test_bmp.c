#include <stdio.h>
#include "win32_spice_interface.h"
#include "win32_spice_interface_types.h"
#include "spice_log.h"
#include "create_bmp.h"
#include <unistd.h>


int width ;
int height;
void* settingchanges(void * ctx,void* ud)
{
    SPICE_SettringChange *sc = (SPICE_SettringChange*)ud;
    SPICE_LOG("%d , %d , %d\n",sc->width,sc->height,sc->bpp);
    width = sc->width;
    height = sc->height;
    return NULL;
}
void * invalidates(void * ctx,void *ud)
{
    SPICE_LOG("\n");
    printf("?????????????");
    /*
    static int i  = 0;
    static int b = 100;
    char filename[100] = "";
    SPICE_Invalidate * bmp_info = (SPICE_Invalidate *)ud;
    sprintf(filename,"%s%x-%d.bmp","./",b,i);
    bmp_generator(filename,width,height,bmp_info->bitmap);
    i++;
    b+=100;
    */


}
void *main_event(void * ctx,void *ud)
{

    SPICE_MainChannelEvent * event = (SPICE_MainChannelEvent*)ud;
    switch (event->et) {
    case LCX_SPICE_CHANNEL_OPENED:
        SPICE_LOG("main channel: opened\n");
        break;
    case LCX_SPICE_CHANNEL_SWITCHING:
        SPICE_LOG("main channel: switching host\n");
        break;
    case LCX_SPICE_CHANNEL_CLOSED:
        SPICE_LOG("main channel: closed\n");
        break;
    case LCX_SPICE_CHANNEL_ERROR_IO:
        SPICE_LOG("main channel: closed\n");
        break;
    case LCX_SPICE_CHANNEL_ERROR_TLS:
    case LCX_SPICE_CHANNEL_ERROR_LINK:
    case LCX_SPICE_CHANNEL_ERROR_CONNECT:
        SPICE_LOG("main channel: failed to connect\n");
        break;
    case LCX_SPICE_CHANNEL_ERROR_AUTH:
        SPICE_LOG("main channel: auth failure (wrong password?\n");
        break;
        SPICE_LOG("unknown main channel event: %d\n",event);
        break;
    }
}


int main(int argc,char **argv)
{
        SPICE_HANDLE handle = SpiceInit();
        int hmain = SpiceOpenMain(handle);
        SpiceRunMainLoop(handle,hmain,true);
        int hspice = SpiceOpen(handle);
        SpiceCallBacks scbs[CB_NULL];


        scbs[0].type = CB_SETTING_CHANGE;
        scbs[0].lcb._cb = settingchanges;
        scbs[0].lcb._ctx = 0;

        scbs[1].type = CB_INVALIDATE;
        scbs[1].lcb._cb = invalidates;
        scbs[1].lcb._ctx = 0;

        scbs[2].type = CB_MAIN_CHANNEL_EVENT; 
        scbs[2].lcb._cb = main_event;
        scbs[2].lcb._ctx = 0;



        SpiceSetCalls(handle,hspice,scbs,3);
        SpiceConnect(handle,hspice,argv[1],argv[2],NULL,"",NULL,NULL,1);

        SPICE_LOG("lcx ===================================== \n");
        sleep(2);
        SPICE_LOG("lcx2 ===================================== \n");

        SpiceDisconnect(handle,hspice);
        SpiceClose(handle, hspice);
        SpiceStopMainLoop(handle,hmain);
        SpiceCloseMain(handle,hmain);
        SpiceUninit(handle);
//    getchar();

    return 0;
}


