
#ifndef spice_log_h
#define spice_log_h

//\033[0;32;31m
#if defined SPICE_LOG_INFO
#define SPICE_LOG(format, ...)     fprintf(stdout,"[FILE:%s][FUNCTION:%s][LINE:%d] : "format,__FILE__,__FUNCTION__,__LINE__, ##__VA_ARGS__)
#define SPICE_ERR(format, ...)     fprintf(stdout,"\033[1;31m [FILE:%s][FUNCTION:%s][LINE:%d] : "format"\033[m",__FILE__,__FUNCTION__,__LINE__, ##__VA_ARGS__)
#define SPICE_WARRING(format, ...)     fprintf(stdout,"\033[0;37m [FILE:%s][FUNCTION:%s][LINE:%d] : "format"\033[m",__FILE__,__FUNCTION__,__LINE__, ##__VA_ARGS__)
#else
#define SPICE_LOG(format, ...)     
#define SPICE_ERR(format, ...)     
#define SPICE_WARRING(format, ...)     
#endif

#endif
