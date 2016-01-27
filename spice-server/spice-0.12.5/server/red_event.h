#ifndef RED_EVENT_H
#define RED_EVENT_H 

#include <pthread.h>
#include <stdbool.h>

typedef struct Event Event;

struct Event
{
   pthread_cond_t _cond;
   pthread_mutex_t _lock;
   bool flag;
};


Event * event_new();
void event_delete(Event * ev);

void init_event(Event * ev);

void uninit_event(Event * ev);

int set_event(Event * ev);

int set_event_all(Event * ev);

int event_timedwait(Event * ev,const struct timespec *restrict abstime);

int event_wait(Event * ev);

bool is_set_event(Event * ev);

void clear_event(Event * ev);



#endif
