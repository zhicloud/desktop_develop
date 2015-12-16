#include "red_threading.h"



#include <stdio.h>
#include <common/log.h>


void init_spice_base_threading(RedBaseThreading * basethreading,void* (*start_rtn)(void*))
{
    set_spice_base_threading_status(basethreading,RED_THREADING_STOPPING);
    basethreading->start_rtn = start_rtn;
    basethreading->g_base_thread = NULL;
}

void set_spice_base_threading_status(RedBaseThreading * basethreading,RedBaseThreadingStatus status)
{
    basethreading->status = RED_THREADING_STOPPING;
}

void set_spice_base_threading_run(RedBaseThreading* basethreading,void* (*start_rtn)(void*))
{
    basethreading->start_rtn = start_rtn;
}

int start_spice_base_threading(RedBaseThreading* basethreading)
{
    int r;
    if ((r = pthread_create(&basethreading->g_base_thread, NULL, basethreading->start_rtn, basethreading->thread_data))) {
        spice_printerr("create thread base failed %d", r);
        return r;
    }
    set_spice_base_threading_status(basethreading,RED_THREADING_RUNNING);
    return r;
}

void init_spice_base_threading_data(RedBaseThreading* basethreading,void* thread_data)
{
    basethreading->thread_data = thread_data;
}

void stop_wait_spice_base_threading(RedBaseThreading * basethreading)
{
    pthread_join(basethreading->g_base_thread,NULL);
    set_spice_base_threading_status(basethreading,RED_THREADING_STOPPING);
}

RedBaseThreading * spice_base_threading_new()
{
    RedBaseThreading *basethreading = (RedBaseThreading *)malloc(sizeof(RedBaseThreading));
    if(!basethreading)
    {
        return NULL;
    }
    return basethreading;

}
void spice_base_threading_delete(RedBaseThreading * basethreading)
{
    if(basethreading)
    {
        free(basethreading);
    }
}


