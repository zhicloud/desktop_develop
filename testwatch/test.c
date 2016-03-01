#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timeb.h>
#include <time.h>
int main()
{
	int rtn; /*子进程的返回数值*/
	time_t time1,time2;
	int sec1,sec2;
	char* command = "/home/cloud_teminal/cloud_teminal";
	while(1) 
	{
		if ( fork() == 0 ) 
		{/* 子进程执行此命令 */
			execlp( command,0);
			/* 如果exec函数返回，表明没有正常执行命令，打印错误信息*/
			perror( command );
			exit( errno );
		}
		else 
		{/* 父进程， 等待子进程结束，并打印子进程的返回值 */
			time(&time1);
			struct tm* tm1 = gmtime(&time1);
			printf("before wait h1 = %d,m1=%d,s1=%d\n",tm1->tm_hour,tm1->tm_min,tm1->tm_sec);
			sec1 = tm1->tm_sec;
			wait ( &rtn );
			time(&time2);
			struct tm* tm2 = gmtime(&time2);	
			printf("after wait h2=%d,m2=%d,s2=%d\n",tm2->tm_hour,tm2->tm_min,tm2->tm_sec);
			sec2 = tm2->tm_sec;

			printf("sec = %d\n",sec2 - sec1);
			if(sec2 - sec1 == 0)
			{
			  printf("wocaoooooooooo\n");
			  system("cp /home/backup/cloud_teminal /home/cloud_teminal");
			}
			printf( " child process return %d\n", rtn );
		}
	}
}
