#ifndef RED_THREAD_H
#define RED_THREAD_H
/*********************************************************************************************************
 * 
 * 说明：线程基类
 *
 * 创建人：李陈祥
 *
 * 创建时间： 2015-12-01
 *
 * 修改及修改时间：
 *                2015-12-01 :初次创建文件，并实现基本代码
 *
 *
 ********************************************************************************************************/



#include <pthread.h>
#include <stdbool.h>

#include "common/log.h"


#define WITRE_CLOSE_MSG "close"

#define RED_SPICE_BASE_THREADING_INIT(threading) do{\
   threading->init_spice_base_threading = init_spice_base_threading;\
   threading->init_spice_base_threading_data = init_spice_base_threading_data;\
   threading->set_spice_base_threading_status = set_spice_base_threading_status;\
   threading->set_spice_base_threading_run = set_spice_base_threading_run;\
   threading->start_spice_base_threading = start_spice_base_threading;\
   threading->stop_wait_spice_base_threading = stop_wait_spice_base_threading;\
   threading->set_spice_base_threading_close = set_spice_base_threading_close;\
   threading->get_spice_base_threading_close = get_spice_base_threading_close;\
   threading->get_spice_base_threading_status = get_spice_base_threading_status;\
}while(0)




typedef enum {
   RED_THREADING_STOPPING = 0,
   RED_THREADING_RUNNING  = 1,
   RED_THREADING_PAUSING  = 2
}RedBaseThreadingStatus;

typedef struct RedBaseThreading RedBaseThreading;

typedef struct RedBaseThreading
{
   RedBaseThreadingStatus status;
   bool bClose;
   pthread_t g_base_thread;
   void * thread_data;
   void* (*start_rtn)(void*);

   void (*init_spice_base_threading)(RedBaseThreading * basethreading,void* (*start_rtn)(void*));
   void (*init_spice_base_threading_data)(RedBaseThreading* basethreading,void* thread_data);
   void (*set_spice_base_threading_status)(RedBaseThreading * basethreading,RedBaseThreadingStatus status);
   RedBaseThreadingStatus (*get_spice_base_threading_status)(RedBaseThreading * basethreading);
   void (*set_spice_base_threading_run)(RedBaseThreading* basethreading,void* (*start_rtn)(void*));
   int (*start_spice_base_threading)(RedBaseThreading* basethreading);
   void (*stop_wait_spice_base_threading)(RedBaseThreading * basethreading);
   void (*set_spice_base_threading_close)(RedBaseThreading * basethreading,bool bClose);
   int (*get_spice_base_threading_close)(RedBaseThreading * basethreading);
};


//
RedBaseThreading * spice_base_threading_new();
void spice_base_threading_delete(RedBaseThreading * basethreading);

void init_spice_base_threading(RedBaseThreading * basethreading,void* (*start_rtn)(void*));
void init_spice_base_threading_data(RedBaseThreading* basethreading,void* thread_data);
void set_spice_base_threading_status(RedBaseThreading * basethreading,RedBaseThreadingStatus status);
RedBaseThreadingStatus get_spice_base_threading_status(RedBaseThreading * basethreading);
void set_spice_base_threading_run(RedBaseThreading* basethreading,void* (*start_rtn)(void*));
int start_spice_base_threading(RedBaseThreading* basethreading);
void stop_wait_spice_base_threading(RedBaseThreading * basethreading);
void set_spice_base_threading_close(RedBaseThreading * basethreading,bool bClose);
int get_spice_base_threading_close(RedBaseThreading * basethreading);

#endif 
