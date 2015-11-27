
#include <glib.h>
#include <glib-object.h>

#include <stdlib.h>
#include <stdio.h>

#ifndef DSPICE_LOG_INFO
#define NDEBUG
#endif
#include <assert.h>



#include "win32_spice_interface_priv.h"
#include "win32-spicy.h"
#include "virt-viewer-file.h"
#include "virt-viewer-util.h"
#include "spice_log.h"
#include "win32_spice_interface.h"
#include "win32-spice-widget-priv.h"
#include "win32_io.h"
#include "win32_service.h"

static bool init_flag = false;
struct Win32SpiceGloabInfo * global_wsgi = NULL;
SPICE_HANDLE SpiceInit()
{
    SPICE_LOG("\n");
    if(init_flag == true)
    {
        SPICE_LOG("you have init it yet\n");
        return NULL;
    }
    init_flag = true;

    struct Win32SpiceGloabInfo * wsgi = g_new0(struct Win32SpiceGloabInfo,1);
    if(wsgi == NULL)
    {
        SPICE_LOG("SpiceInit error with malloc memory\n");
        return NULL;
    }
    global_wsgi = wsgi;

    g_rec_mutex_init(&wsgi->rec_mutex_conn);
    g_rec_mutex_init(&wsgi->rec_mutex_loop);
    return (SPICE_HANDLE)wsgi;
}


void SpiceUninit(IN_OUT SPICE_HANDLE sh)
{
    SPICE_LOG("\n");

    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;
    assert(wsgi);

    if(init_flag == false)
    {
        return;
    }
    init_flag = false;
    
    g_rec_mutex_clear(&wsgi->rec_mutex_conn);
    g_rec_mutex_clear(&wsgi->rec_mutex_loop);

    g_free(wsgi);
}

static int find_free_mainloop(SPICE_HANDLE sh)
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    assert(wsgi);

    g_rec_mutex_lock(&wsgi->rec_mutex_loop);
    int loop;
    for(loop = 0; loop < MAX_MAIN_LOOP; loop++)
    {
        if(!wsgi->MainloopList[loop])
        {
            g_rec_mutex_unlock(&wsgi->rec_mutex_loop);
            return loop;
        }

    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_loop);
    return ERROR_MAX;
}

int SpiceOpenMain(IN_OUT SPICE_HANDLE sh)
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;
    int ret = ERROR_MAX;
    assert(wsgi);
    g_rec_mutex_lock(&wsgi->rec_mutex_loop);
    do 
    {
        ret = find_free_mainloop(sh);
        if(ret < 0)
        {
            ret = ERROR_MAX;
            break;
        }
        wsgi->MainloopList[ret] = g_new0(struct main_loop_context,1);
        if(!wsgi->MainloopList[ret])
        {
            ret = ERROR_NO_MEMORY; 
            break;
        }
        
    }while(0);
    g_rec_mutex_unlock(&wsgi->rec_mutex_loop);
    return ret;
}


int SpiceCloseMain(IN_OUT SPICE_HANDLE sh,IN int hmain)
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;
    assert(wsgi);
    assert(hmain >= 0 && hmain < MAX_MAIN_LOOP);

    g_rec_mutex_lock(&wsgi->rec_mutex_loop);
    if(wsgi->MainloopList[hmain])
    {
        g_free(wsgi->MainloopList[hmain]);
        wsgi->MainloopList[hmain] = NULL;
    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_loop);
    
    return 0;
}

struct smain_thread
{
    SPICE_HANDLE sh;
    int hmain;
};

static gpointer  main_loop_run_thread(gpointer data)
{
    SPICE_LOG("\n");
    assert(data);
    
    struct smain_thread * pThis = (struct smain_thread *)data;

    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)pThis->sh;

    SPICE_LOG("wocscacacaca\n");
    g_rec_mutex_lock(&wsgi->rec_mutex_loop);
    GMainLoop *mainloop = wsgi->MainloopList[pThis->hmain]->mainloop = g_main_loop_new(NULL, FALSE);
    GMainContext *context = wsgi->MainloopList[pThis->hmain]->context;
    g_rec_mutex_unlock(&wsgi->rec_mutex_loop);

    SPICE_LOG("?????????????????????????????????\n");
    if(mainloop)
    {
    SPICE_LOG("################################################\n");
    
        g_main_loop_run(mainloop);
    }

    SPICE_LOG("::::::::::::::::::::::::::::::::::::::::::\n");
    return (gpointer)0xdead;
}

int SpiceRunMainLoop(IN_OUT SPICE_HANDLE sh,IN int hmain,bool default_ctx)
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;
    assert(wsgi);
    assert(hmain >= 0 && hmain < MAX_MAIN_LOOP);
    int ret = ERROR_HANDLE;
    struct smain_thread * smt = g_new0(struct smain_thread,1);
    g_rec_mutex_lock(&wsgi->rec_mutex_loop);
    smt->sh = sh;
    smt->hmain = hmain;
    if(wsgi->MainloopList[hmain])
    {
        GError*  error = NULL;
        /*
        if(!default_ctx)
        {
            wsgi->MainloopList[hmain]->context = g_main_context_new();
        }
        else
        {
            wsgi->MainloopList[hmain]->context = NULL;
        }*/
        wsgi->MainloopList[hmain]->context = NULL;
        //wsgi->MainloopList[hmain]->mainloop = g_main_loop_new(wsgi->MainloopList[hmain]->context, FALSE);

        wsgi->MainloopList[hmain]->thread = g_thread_try_new("SPICE_MAIN_LOOP",(GThreadFunc)main_loop_run_thread, 
                (void*)smt, &error);
        ret = (wsgi->MainloopList[hmain]->thread ? 0 : ERROR_RESOURCE);
        
        if (error != NULL)
        {
            g_object_unref(error);
        }


        /*
        if(!ret)
        {
            if(wsgi->MainloopList[hmain]->mainloop)
            {
                g_main_loop_unref(wsgi->MainloopList[hmain]->mainloop);

            }
            if(wsgi->MainloopList[hmain]->context)
            {
                g_main_context_unref(wsgi->MainloopList[hmain]->context);
            }
        }*/
    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_loop);
    SPICE_LOG("end\n");

    return ret;
}


int SpiceStopMainLoop(IN_OUT SPICE_HANDLE sh,IN int hmain)
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;
    assert(wsgi);
    assert(hmain >= 0 && hmain < MAX_MAIN_LOOP);
    int ret = ERROR_HANDLE;

    GThread *tmpthread = NULL;
    g_rec_mutex_lock(&wsgi->rec_mutex_loop);
    if(wsgi->MainloopList[hmain])
    {
        if(wsgi->MainloopList[hmain]->mainloop)
            g_main_loop_quit(wsgi->MainloopList[hmain]->mainloop);
        tmpthread = wsgi->MainloopList[hmain]->thread;
        wsgi->MainloopList[hmain]->thread = NULL;
    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_loop);

    if(tmpthread)
        g_thread_join(tmpthread);

    g_rec_mutex_lock(&wsgi->rec_mutex_loop);
    if(wsgi->MainloopList[hmain])
    {
        if(wsgi->MainloopList[hmain]->mainloop)
            g_main_loop_unref(wsgi->MainloopList[hmain]->mainloop);
        if(wsgi->MainloopList[hmain]->context)
            g_main_context_unref(wsgi->MainloopList[hmain]->context);
        wsgi->MainloopList[hmain]->mainloop = NULL;
        wsgi->MainloopList[hmain]->context = NULL;
    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_loop); 
    return 0;
}


static int find_free_spiceconn(SPICE_HANDLE sh)
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    assert(wsgi);

    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    int loop;
    for(loop = 0; loop < MAX_SPICE_CONNECT; loop++)
    {
        if(!wsgi->SpiceConnList[loop])
        {
            g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
            return loop;
        }
    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
    return ERROR_MAX;
}

int SpiceOpen(IN_OUT SPICE_HANDLE sh)
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    assert(wsgi);

    int ret = ERROR_MAX;

    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    do
    {
        ret = find_free_spiceconn(sh);
        if(ret < 0)
        {
            ret = ERROR_MAX;
            break;
        }
        wsgi->SpiceConnList[ret] = g_new0(struct priv_spice_connection,1);
        if(!wsgi->SpiceConnList[ret])
        {
            ret = ERROR_NO_MEMORY; 
            break;
        }
    }while(0);
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
    return ret;
}


void SpiceSetCalls(IN_OUT SPICE_HANDLE sh,IN int hspice,SpiceCallBacks *scbs,int cc)
{
    SPICE_LOG("hspice = %d\n",hspice);
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    assert(wsgi);
    assert(hspice >= 0 && hspice < MAX_SPICE_CONNECT);
    assert(scbs);
    assert(cc > 0 && cc <= CB_NULL);

    CBTypes type = CB_NULL;
    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    for(int i = 0 ; i < cc; i++)
    {
        type = scbs[i].type;
        memcpy(&wsgi->SpiceConnList[hspice]->callbacks[type],&scbs[i],sizeof(SpiceCallBacks));
    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
    
}

void SpiceSetUSBRedirOnConnectFilterBefore(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char * filter) 
{
    SPICE_LOG("hspice = %d\n",hspice);
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    assert(wsgi);
    assert(hspice >= 0 && hspice < MAX_SPICE_CONNECT);

    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    memset(wsgi->SpiceConnList[hspice]->default_usb_on_connect,0,sizeof(wsgi->SpiceConnList[hspice]->default_usb_on_connect));
    if(filter == NULL)
    {
        g_debug("SpiceSetUSBRedirOnConnectFilterBefore============================== %s ==============2","default_usb_on_connect, is null");
        strcpy(wsgi->SpiceConnList[hspice]->default_usb_on_connect,"-1,7104,32787,-1,1|0x07,-1,-1,-1,1|0x08,-1,-1,-1,1|0x02,-1,-1,-1,1|-1,-1,-1,-1,0");
    }
    else
    {
        g_debug("SpiceSetUSBRedirOnConnectFilterBefore============================== %s ==============2",filter);
        strcpy(wsgi->SpiceConnList[hspice]->default_usb_on_connect,filter);
    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
}

void SpiceSetUSBFilterBefore(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char * filter)
{
    SPICE_LOG("hspice = %d\n",hspice);
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    assert(wsgi);
    assert(hspice >= 0 && hspice < MAX_SPICE_CONNECT);

    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    memset(wsgi->SpiceConnList[hspice]->default_usb_on_connect,0,sizeof(wsgi->SpiceConnList[hspice]->default_usb_on_connect));
    if(filter == NULL)
    {
        g_debug("SpiceSetUSBFilterBefore============================== %s ==============2","default_usb_connect, is null");
        strcpy(wsgi->SpiceConnList[hspice]->default_usb_connect,"-1,7104,32787,-1,1|0x07,-1,-1,-1,1|0x08,-1,-1,-1,1|0x02,-1,-1,-1,1|-1,-1,-1,-1,0");
    }
    else
    {
        g_debug("SpiceSetUSBFilterBefore============================== %s ==============2",filter);
        strcpy(wsgi->SpiceConnList[hspice]->default_usb_connect,filter);
    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);

}
//default_usb_enable
void SpiceEnableAutoUSBRedirectBefore(IN_OUT SPICE_HANDLE sh,IN int hspice,IN int enable) 
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    assert(wsgi);
    assert(hspice >= 0 && hspice < MAX_SPICE_CONNECT);

    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    wsgi->SpiceConnList[hspice]->default_usb_enable = enable;
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
}




int SpiceClose(IN_OUT SPICE_HANDLE sh,IN int hspice)
{
    SPICE_LOG("hspice = %d\n",hspice);
//    printf("2222222222222222==================22222222222222222222222\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    assert(wsgi);
    assert(hspice >= 0 && hspice < MAX_SPICE_CONNECT);

    int ret = ERROR_HANDLE;
    g_rec_mutex_lock(&wsgi->rec_mutex_conn);

    if(wsgi->SpiceConnList[hspice])
    {
        g_free(wsgi->SpiceConnList[hspice]);
        wsgi->SpiceConnList[hspice] = NULL;
    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
    return ret;
}

struct SpiceConnectARGS
{
    SPICE_HANDLE sh;
    gint hspice;
    gchar  host[20];
    gchar port[10];
    gchar tp[10];
    gchar pw[100];
    gchar cf[1024 * 2];
    gchar cs[1024 * 2];
    gchar vvFileName[1024*2];
    bool sound;
};

//static gpointer  SpiceConnect_thread(gpointer data)
static gboolean  SpiceConnect_thread(gpointer data)
{
    SPICE_LOG("\n");
    assert(data);
    struct SpiceConnectARGS * args = (struct SpiceConnectARGS *)data;

    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)args->sh;  

    g_rec_mutex_lock(&wsgi->rec_mutex_conn);

    SPICE_LOG("args->hspice = %d\n",args->hspice);
    wsgi->SpiceConnList[args->hspice]->conn = connection_new();//important port 1
    wsgi->SpiceConnList[args->hspice]->conn->callback = wsgi->SpiceConnList[args->hspice]->callbacks;

    g_debug("SpiceConnect_thread============================== %s ==============2",wsgi->SpiceConnList[args->hspice]->default_usb_on_connect);
    g_debug("SpiceConnect_thread============================== %s ==============1",wsgi->SpiceConnList[args->hspice]->default_usb_connect);
    
    SpiceEnableAutoUSBRedirect(wsgi,args->hspice,wsgi->SpiceConnList[args->hspice]->default_usb_enable); 
    SpiceSetUSBRedirOnConnectFilter(wsgi,args->hspice,wsgi->SpiceConnList[args->hspice]->default_usb_on_connect); 
    SpiceSetUSBFilter(wsgi,args->hspice,wsgi->SpiceConnList[args->hspice]->default_usb_connect);
    
    if(!strcmp(args->vvFileName,""))
    {
        spiceClientConnect (wsgi->SpiceConnList[args->hspice]->conn,args->host, args->port, args->tp,args->pw,args->cf,
                NULL,args->cs, args->sound);//important port 2
    }
    else
    {
        GError *error = NULL;
        VirtViewerFile *vv_file = NULL;
        vv_file = virt_viewer_file_new(args->vvFileName, &error);
        spiceClientConnectVv(wsgi->SpiceConnList[args->hspice]->conn,vv_file,args->sound);
    
        if (vv_file != NULL)
        {
            g_object_unref(vv_file);
        }
        if (error != NULL)
        {
            g_object_unref(error);
        }

    }
    wsgi->connections++;

    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
    g_free(args);
    return FALSE;
    //return (gpointer)0xdead;
}

static void init_spiceconn_atgs(struct SpiceConnectARGS * args,SPICE_HANDLE sh,int hspice,const char * host,const char * port,
        const char *tp, const char *pw,const char* cf,/*IN_OUT SByteArray *cc,*/const char*cs,bool sound,const gchar *vvFileName)
{
    args->sh = sh;
    args->hspice = hspice;
    if(host)
        strncpy(args->host,host,19);
    if(port)
        strncpy(args->port,port,9);
    if(tp)
        strncpy(args->tp,tp,9);
    if(pw)
        strncpy(args->pw,pw,99);
    if(cf)
        strncpy(args->cf,cf,1024 * 2 - 1 );
    if(cs)
        strncpy(args->cs,cs,1024 * 2 - 1);
    if(vvFileName)
        strncpy(args->vvFileName,vvFileName,1024 * 2 - 1);
    args->sound = sound;
}

int SpiceConnect(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char * host,IN const char * port,IN const char *tp,
        IN const char *pw,IN const char* cf,/*IN_OUT SByteArray *cc,*/IN const char*cs,IN  bool sound)
{
    SPICE_LOG("hspice = %d\n",hspice);

    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    assert(wsgi);
    assert(hspice >= 0 && hspice < MAX_SPICE_CONNECT);
    struct SpiceConnectARGS * args = NULL ;

    GError*  error = NULL;
    int ret = ERROR_OK;
    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    do
    {
        args = g_new0(struct SpiceConnectARGS,1);
        if(!args)
        {
            ret = ERROR_NO_MEMORY;
            break;
        }
        init_spiceconn_atgs(args,sh,hspice,host,port,tp,pw,cf,cs,sound,NULL);
#if 0
        wsgi->SpiceConnList[hspice]->thread = g_thread_try_new("SpiceConnect",(GThreadFunc)SpiceConnect_thread, 
                (void*)args, &error);
        if(!wsgi->SpiceConnList[hspice]->thread)
        {
            ret = ERROR_RESOURCE; 
                break;
        }
#endif
        g_main_context_invoke (NULL, SpiceConnect_thread, (gpointer)args);

    }while(0);
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
    if(ret != ERROR_OK && args)
    {
        g_free(args);
    }
#if 0
    if (error != NULL)
    {
        g_object_unref(error);
    }
    if(wsgi->SpiceConnList[hspice]->thread)
    {
        g_thread_join(wsgi->SpiceConnList[hspice]->thread);
        wsgi->SpiceConnList[hspice]->thread = NULL;
    }
#endif
    return ret;
}

int SpiceStartSessionFromVvFile(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char* vvFileName,IN  bool sound)
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    assert(wsgi);
    assert(hspice >= 0 && hspice < MAX_SPICE_CONNECT);
    struct SpiceConnectARGS * args = NULL ;

    int ret = ERROR_OK;
    GError*  error = NULL;
    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    do
    {
        args = g_new0(struct SpiceConnectARGS,1);
        if(!args)
        {
            ret = ERROR_NO_MEMORY;
            break;
        }
        init_spiceconn_atgs(args,sh,hspice,NULL,NULL,NULL,NULL,NULL,NULL,sound,vvFileName);

#if 0
        wsgi->SpiceConnList[hspice]->thread = g_thread_try_new("SpiceConnect",(GThreadFunc)SpiceConnect_thread, 
                (void*)args, &error);
        if(!wsgi->SpiceConnList[hspice]->thread)
        {
            ret = ERROR_RESOURCE; 
            break;
        }
#endif
        g_main_context_invoke (NULL, SpiceConnect_thread, (gpointer)args);
    }while(0);
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
    if(ret != ERROR_OK && args)
    {
        g_free(args);
    }
#if 0
    if (error != NULL)
    {
        g_object_unref(error);
    }
    if(wsgi->SpiceConnList[hspice]->thread)
    {
        g_thread_join(wsgi->SpiceConnList[hspice]->thread);
        wsgi->SpiceConnList[hspice]->thread = NULL;
    }
#endif
    return 0;
}

static gboolean disconnect(gpointer user_data) {                                                                                                                     
    SPICE_LOG("\n");
    assert(user_data);
    struct spice_connection* spice_conn = (struct spice_connection*)user_data;
    SPICE_LOG("=========\n");
    connection_disconnect(spice_conn);
    SPICE_LOG("==================end\n");
    return FALSE;            
}

void SpiceDisconnect(IN_OUT SPICE_HANDLE sh,IN int hspice)
{
    SPICE_LOG("hspice ==== %d\n",hspice);
 //   printf("11111111111111==================111111111111111111111111\n");
    assert(sh);
    assert(hspice >= 0 && hspice < MAX_SPICE_CONNECT);
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    //struct SpiceConnectARGS * args = g_new0(struct SpiceConnectARGS,1);
    
    struct spice_connection* spice_conn = NULL;
    
    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    if(!wsgi->SpiceConnList[hspice])
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return;
    }
    if(!wsgi->SpiceConnList[hspice]->conn)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return;
    }
    
    wsgi->SpiceConnList[hspice]->conn->callback = NULL;
    spice_conn = wsgi->SpiceConnList[hspice]->conn;
    wsgi->SpiceConnList[hspice]->conn = NULL;
    wsgi->connections--;
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
    if(spice_conn != NULL)
    {
        g_main_context_invoke (NULL, disconnect, (gpointer)spice_conn);
    }
    SPICE_LOG("hspice ==== end %d\n",hspice);
}


int SpiceCreateOvirtSession(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char* URI, IN const char * user, IN const char *  pass,
        IN const char * sslCaFile,IN bool sound, IN bool sslStrict)
{
    SPICE_LOG("\n");
    //暂时不实现
    return 0;
}

int SpiceFetchVmNames(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char *URI, IN const char* user, IN const char* password,
        IN const char * sslCaFile,IN bool sslStrict)
{
    SPICE_LOG("\n");
    //暂时不实现
    return 0;

}

void SpiceRequestResolution(IN_OUT SPICE_HANDLE sh,IN int hspice,IN int x, IN int y)
{
    SPICE_LOG("\n");
    assert(sh);
    assert(hspice >= 0 && hspice < MAX_SPICE_CONNECT);
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    SpiceDisplay* display = NULL;

    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    if(!wsgi->SpiceConnList[hspice])
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if(!wsgi->SpiceConnList[hspice]->conn)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    display = wsgi->SpiceConnList[hspice]->conn->global_display;
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
    if(!display)
    {
        return ;
    }
    SpiceRequestResolution_io(x,y,display);
}

void SpiceKeyEvent(IN_OUT SPICE_HANDLE sh,IN int hspice,IN bool down, IN int hardware_keycode)
{
    SPICE_LOG("\n");
    
//    printf("[File:%s] [Line:%d] [Function:%s] msg = \"%s\"\n"
//            , __FILE__
//            , __LINE__
//            , __FUNCTION__
//            , "The function has been invoked!");

    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  

    SpiceDisplay* display = NULL;

    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    if(!wsgi->SpiceConnList[hspice])
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if(!wsgi->SpiceConnList[hspice]->conn)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    display = wsgi->SpiceConnList[hspice]->conn->global_display;
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
    if(!display)
    {
        return ;
    }
    SpiceDisplayPrivate* d = SPICE_DISPLAY_GET_PRIVATE(display);
    int scancode;            

    SPICE_DEBUG("%s %s: keycode: %d", __FUNCTION__, "Key", hardware_keycode);

    if (!d->inputs)          
        return;                

    scancode = win32key2spice(hardware_keycode);
    scancode = hardware_keycode;                                                                                                                                    
    if (down) {
        send_key(display, hardware_keycode, 1);
    } else {
        send_key(display, hardware_keycode, 0);
    }
    return ;

}


void ButtonLCX2Spice(int *button)
{
    if (*button == LCX_SPICE_MOUSE_BUTTON_LEFT)
        *button = SPICE_MOUSE_BUTTON_LEFT;
    else if (*button == LCX_SPICE_MOUSE_BUTTON_MIDDLE)
        *button = SPICE_MOUSE_BUTTON_MIDDLE;
    else if (*button == LCX_SPICE_MOUSE_BUTTON_RIGHT)
        *button = SPICE_MOUSE_BUTTON_RIGHT;
    else if (*button == LCX_SPICE_MOUSE_BUTTON_INVALID)
        *button = SPICE_MOUSE_BUTTON_INVALID;
    else if (*button == LCX_SPICE_MOUSE_BUTTON_UP)
        *button = SPICE_MOUSE_BUTTON_UP;
    else if (*button == LCX_SPICE_MOUSE_BUTTON_DOWN)
        *button = SPICE_MOUSE_BUTTON_DOWN;
    else
        *button = SPICE_MOUSE_BUTTON_ENUM_END;
}


void SpiceButtonEvent(IN_OUT SPICE_HANDLE sh,IN int hspice,IN int  x, IN int y, IN int metaState, IN int type)
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  

    SpiceDisplay* display = NULL;

    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    if(!wsgi->SpiceConnList[hspice])
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if(!wsgi->SpiceConnList[hspice]->conn)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    display = wsgi->SpiceConnList[hspice]->conn->global_display;
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);

    SpiceDisplayPrivate *d = SPICE_DISPLAY_GET_PRIVATE(display);


    if (!d->inputs || (x >= 0 && x < d->width && y >= 0 && y < d->height)) {

        gboolean down = (type & PTRFLAGS_DOWN) != 0;
        int mouseButton = type &~ PTRFLAGS_DOWN;
        ButtonLCX2Spice(&mouseButton);
        int newMask = button_update_mask (mouseButton, down);

        gint dx;
        gint dy;
        switch (d->mouse_mode) {
            case SPICE_MOUSE_MODE_CLIENT:
                spice_inputs_position(d->inputs, x, y, d->channel_id, newMask);
                break;
            case SPICE_MOUSE_MODE_SERVER:
                dx = d->mouse_last_x != -1 ? x - d->mouse_last_x : 0;
                dy = d->mouse_last_y != -1 ? y - d->mouse_last_y : 0;
                spice_inputs_motion(d->inputs, dx, dy, newMask);
                d->mouse_last_x = x;
                d->mouse_last_y = y;
                break;
            default:
                g_warn_if_reached();
                break;
        }

        if (mouseButton != SPICE_MOUSE_BUTTON_INVALID) {
            if (down) {
                spice_inputs_button_press(d->inputs, mouseButton, newMask);
            } else {
                //usleep(50000);
                spice_inputs_button_release(d->inputs, mouseButton, newMask);
            }
        }
    }
    return ;

}


void SpiceEnableAutoUSBRedirect(IN_OUT SPICE_HANDLE sh,IN int hspice,IN int enable) 
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    if(!wsgi->SpiceConnList[hspice])
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if(!wsgi->SpiceConnList[hspice]->conn)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    SpiceUsbDeviceManager *manager;    
    struct spice_connection* conn = wsgi->SpiceConnList[hspice]->conn;
    manager = spice_usb_device_manager_get(conn->session, NULL);
    if (manager)
    {
        g_object_set(manager,"auto-connect",enable,NULL);

    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);

}


void SpiceSetUSBFilter(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char * filter)
{

    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    if(!wsgi->SpiceConnList[hspice])
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if(!wsgi->SpiceConnList[hspice]->conn)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if (!filter)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return;
    }
    SpiceUsbDeviceManager *manager;    
    struct spice_connection* conn = wsgi->SpiceConnList[hspice]->conn;
    manager = spice_usb_device_manager_get(conn->session, NULL);
    if (manager)
    {
        g_object_set(manager,"auto-connect-filter",filter,NULL);

    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);

} 


void SpiceSetUSBRedirOnConnectFilter(IN_OUT SPICE_HANDLE sh,IN int hspice,IN const char * filter) 
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    if(!wsgi->SpiceConnList[hspice])
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if(!wsgi->SpiceConnList[hspice]->conn)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if (!filter)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return;
    }
    SpiceUsbDeviceManager *manager;    
    struct spice_connection* conn = wsgi->SpiceConnList[hspice]->conn;
    manager = spice_usb_device_manager_get(conn->session, NULL);
    if (manager)
    {
        g_object_set(manager,"redirect-on-connect",filter,NULL);

    }
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);

}


void SpiceUSBRedirect(IN_OUT SPICE_HANDLE sh,IN int hspice,IN void * dev) 
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    if(!wsgi->SpiceConnList[hspice])
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if(!wsgi->SpiceConnList[hspice]->conn)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if (!dev)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return;
    }
    SpiceUsbDevice *device = (SpiceUsbDevice *)dev;
    SpiceUsbDeviceManager *manager;    
    struct spice_connection* conn = wsgi->SpiceConnList[hspice]->conn;
    manager = spice_usb_device_manager_get(conn->session, NULL);
    if (!manager)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return;
    }
    spice_usb_device_manager_connect_device_async(manager,
                                   device, NULL,
                                   spice_usb_device_manager_auto_connect_cb,
                                   spice_usb_device_ref(device));
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);

}

void SpiceSetUSBDisconnect(IN_OUT SPICE_HANDLE sh,IN int hspice,IN void * dev) 
{
    SPICE_LOG("\n");
    struct Win32SpiceGloabInfo * wsgi = (struct Win32SpiceGloabInfo *)sh;  
    g_rec_mutex_lock(&wsgi->rec_mutex_conn);
    if(!wsgi->SpiceConnList[hspice])
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if(!wsgi->SpiceConnList[hspice]->conn)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return ;
    }
    if (!dev)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return;
    }
    SpiceUsbDevice *device = (SpiceUsbDevice *)dev;
    SpiceUsbDeviceManager *manager;    
    struct spice_connection* conn = wsgi->SpiceConnList[hspice]->conn;
    manager = spice_usb_device_manager_get(conn->session, NULL);
    if (!manager)
    {
        g_rec_mutex_unlock(&wsgi->rec_mutex_conn);
        return;
    }
    spice_usb_device_manager_disconnect_device(manager,device);
    g_rec_mutex_unlock(&wsgi->rec_mutex_conn);

}

/* ------------------------------------------------------------------ */
