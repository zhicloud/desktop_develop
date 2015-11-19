#include "create_bmp.h"

int bmp_generator(char * filename,int width,int height,unsigned char *data)
{    
    FileHead bmp_head;    
    Infohead bmp_info; 
    int size = width*height*4; 
    bmp_head.bfType=0x4d42; 
    bmp_head.bfSize=size+sizeof(FileHead)+sizeof(Infohead);//24+head+info no quad
    bmp_head.bfReserved1=bmp_head.bfReserved2=0;
    bmp_head.bfOffBits=bmp_head.bfSize-size;  
    bmp_info.biSize=40; 
    bmp_info.biWidth=width; 
    bmp_info.biHeight=-height;
    bmp_info.biPlanes=1;
    bmp_info.biBitCount = 32;  
    bmp_info.biCompress=0; 
    bmp_info.biSizeImage=size; 
    bmp_info.biXPelsPerMeter=0; 
    bmp_info.biYPelsPerMeter=0; 
    bmp_info.biClrUsed=0;  
    bmp_info.biClrImportant=0; 
    FILE *fp;  
    if(!(fp=fopen(filename,"wb"))) 
        return 0;   
    fwrite(&bmp_head,1,sizeof(FileHead),fp);   
    fwrite(&bmp_info,1,sizeof(Infohead),fp);  
    fwrite(data,1,size,fp);    
    /*    int i = 0;    for(; i < height ; i++)    {        fwrite(data + i * width * 3,1,width * 3,fp);    }*/ 
    fclose(fp);    
    return 1;
}
