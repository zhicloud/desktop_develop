
#include "red_event.h"
#include "common/rect.h"
#include "common/region.h"
#include "common/ring.h"


Event * event_new()
{
    Event * ev = (Event *)malloc(sizeof(Event));
    if (ev)
    {
        init_event(ev);
    }
    return ev;
}

void event_delete(Event * ev)
{
    if(ev)
    {
        uninit_event(ev);
        free(ev);
    }

}

void init_event(Event * ev)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&ev->_lock,&attr);
    pthread_mutexattr_destroy(&attr);
    
    pthread_cond_init(&ev->_cond,NULL);
    ev->flag = false;
}

void uninit_event(Event * ev)
{
    pthread_cond_destroy(&ev->_cond);
    pthread_mutex_destroy(&ev->_lock);
    ev->flag = false;
}

int set_event(Event * ev)
{
    int ret = 0;
    pthread_mutex_lock(&ev->_lock);
    ev->flag = true;
    ret = pthread_cond_signal(&ev->_cond);  
    pthread_mutex_unlock(&ev->_lock);
    return ret;
}

int set_event_all(Event * ev)
{
    int ret = 0;
    pthread_mutex_lock(&ev->_lock);
    ev->flag = true;
    ret = pthread_cond_broadcast(&ev->_cond);  
    pthread_mutex_unlock(&ev->_lock);
    return ret;
}

int event_timedwait(Event * ev,const struct timespec *restrict abstime)
{
    int ret = 0;
    pthread_mutex_lock(&ev->_lock);
    if(ev->flag == false)
    {
        ret = pthread_cond_timedwait(&ev->_cond, &ev->_lock,abstime);
    }
    ev->flag = false;
    pthread_mutex_unlock(&ev->_lock);
    return ret;
}

int event_wait(Event * ev)
{
    int ret = 0;
    pthread_mutex_lock(&ev->_lock);
    while(ev->flag == false)
    {
        ret = pthread_cond_wait(&ev->_cond, &ev->_lock);
    }
    ev->flag = false;
    pthread_mutex_unlock(&ev->_lock);
    return ret;
}

bool is_set_event(Event * ev)
{
    return ev->flag;

}

void clear_event(Event * ev)
{
    pthread_mutex_lock(&ev->_lock);
    ev->flag = false;
    pthread_mutex_unlock(&ev->_lock);
}


