#ifndef FILE_UTIL_H
#define FILE_UTIL_H
#include <stdio.h>
#include <stdbool.h>

int get_file_length(FILE * fp);

bool file_get_contents(const char *filename,char **contents,int *length);

int file_readn(FILE * fp , int size,char *contents);

int trim(char s[]);


#endif
