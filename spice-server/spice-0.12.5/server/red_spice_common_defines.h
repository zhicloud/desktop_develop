#ifndef RED_SPICE_COMMON_DEFINES_H
#define RED_SPICE_COMMON_DEFINES_H

typedef struct LCXCALLBACK
{   
   void* (*_cb)(void* ctx,void * ud);    
   void* _ctx;   
   void* _ud;
}LCXCALLBACK;

#endif 
