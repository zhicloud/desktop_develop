#include "fileutil.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>


int get_file_length(FILE * fp)
{
   int size = 0;
   int pos = 0;
   pos = ftell(fp);
   fseek (fp, 0, SEEK_END);
   size=ftell(fp);
   fseek (fp, pos, SEEK_SET);
   return size;
}

bool file_get_contents(const char *filename,char **contents,int *length)
{
   bool flag = false;
   FILE *fp = NULL;
   int size = 0;
   *contents = NULL;
   do
   {
      fp = fopen(filename,"r");
      if (!fp) break;
      size = get_file_length(fp);
      if (size <= 0) break;
      //printf("size = %d\n",size);

      *contents = (char *)malloc(size * sizeof(char));
      if (! *contents) break;
      int ret = file_readn(fp,size,*contents);
      if (ret <=  0) break; 

      if (length) *length = ret;
      flag = true;
   }while(0);

   if(fp) fclose(fp);
   if(!flag && *contents) free(*contents);

   return flag;
}

int file_readn(FILE * fp , int size,char *contents)
{
   int left = size;
   int ret = 0;

   while(left > 0)
   {
      ret = fread(contents,1,left,fp);
      if (ret < 0)
      {
         if (errno == EAGAIN || errno == EINTR)
         {
            continue;
         }
         else
         {
            return ret ; 
         }
      }
      else if(ret == 0)
      {
         return size - left;
      }
      left -= ret;
   }
   return size;
}

int trim(char s[])  
{  
   int n;
   char *p = strstr(s,"\n");
   if(p){
		s[p-s] =  '\0';
   }
	
   for (n = strlen(s)-1; n >= 0; n--)
		if (s[n] != ' ' && s[n] != '\t' && s[n] != '\n')
			break;
    s[n+1] = '\0';

	
    return n;
}  

