#ifndef create_bmp_h
#define create_bmp_h




#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef long LONG;
typedef unsigned char BYTE;
typedef unsigned long DWORD;
typedef unsigned short WORD;

#pragma  pack (push,1)
typedef struct 
{    
   WORD    bfType;//2
   DWORD   bfSize;//4    
   WORD    bfReserved1;//2 
   WORD    bfReserved2;//2 
   DWORD   bfOffBits;//4
}FileHead;

typedef struct
{    
   DWORD      biSize;//4 
   LONG       biWidth;//4  
   LONG       biHeight;//4 
   WORD       biPlanes;//2  
   WORD       biBitCount;//2 
   DWORD      biCompress;//4 
   DWORD      biSizeImage;//4  
   LONG       biXPelsPerMeter;//4  
   LONG       biYPelsPerMeter;//4  
   DWORD      biClrUsed;//4 
   DWORD      biClrImportant;//4
}Infohead;

typedef struct 
{    
   BYTE b;    
   BYTE g;    
   BYTE r;
}RGB_data;//RGB TYPE
#pragma pack(pop)

# ifdef __cplusplus
extern "C"
{
#endif

int bmp_generator(char * filename,int width,int height,unsigned char *data);
# ifdef __cplusplus
}
#endif

#endif 

