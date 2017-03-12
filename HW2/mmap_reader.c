#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <assert.h>
#include <errno.h> 
#include <string.h>
#include <unistd.h>
#include <time.h> // for time measurements
#include <sys/time.h>
#include <limits.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <signal.h>
#include <sys/mman.h>

#define report_error(str) {printf("Error: %s - %s, line %d\n",(str), strerror(errno), __LINE__); exit(errno);}

struct sigaction oldaction;
int error = 0;

void sig_handler(int signum)
{
	int i = 0, count = 0;
	int fsize = 0;
	char *arr;
	int mfd;
	char *filename = "/tmp/mmapped.bin";

	if (signum == SIGUSR1)
	{
		// printf("%s\n", "got signal");
		if((mfd = open(filename, O_RDWR)) < 0)
			report_error("can't open mmapped file")

		struct stat sb;
		if ((stat(filename, &sb)) == -1)
			report_error("can't use stat function")
		fsize = sb.st_size;
		
		struct timeval t1,t2;

		double elapsed_millisec;

		if(gettimeofday(&t1, NULL) < 0) //start time measurement
			report_error("can't get time of day")

		arr = (char*) mmap(	NULL, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, mfd, 0);

		if(arr == MAP_FAILED)
			report_error("can't mmap the file")

		// printf("%s\n", "before while");
		for(i = 0; i < fsize && arr[i] != '\0'; i++)
		{
			// printf("%d\n", i);
			if(arr[i] == 'a')
				++count;
			else
			{
				error = 1;
				printf("char read is not 'a' and num read != %d\n", fsize);
				break;
			}
		}
		// printf("%s\n", "after while 1");
		if (!error)
		{
			// printf("%s\n", "in if error");
			if (arr[fsize - 1] != '\0')
			{
				printf("%s\n", "last char of the file is not \\0");
				error = 1;
			} 
			else
				count++;
			// printf("%s\n", "after chack");
		}

		// printf("%s\n", "after while");
		if(gettimeofday(&t2, NULL) < 0) //end time measurement
			report_error("can't get time of day")
	
		elapsed_millisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
  		elapsed_millisec += (t2.tv_usec - t1.tv_usec) / 1000.0;

  		printf("%f milliseconds passed\n", elapsed_millisec);
  		printf("%s%d\n","the size read from file is ", count);

  		if(munmap(arr,fsize) < 0)
  			report_error("can't munmap the file")
  		close(mfd);
  		if(unlink(filename) < 0)
  			report_error("can't unlink the file")

  		if (sigaction(SIGTERM, &oldaction, NULL) < 0)
			report_error("can't restore SIGTERM signal")

		if (error)
			exit(-1);
		if(count != fsize)
		{
			printf("number of chars read != %d\n", count);
			exit(-1);
		}

  		exit(0);
	}
}

int main(void)
{
	struct sigaction action;
	struct sigaction noaction;

	action.sa_handler = sig_handler;
	action.sa_flags = 0;

	if (sigaction(SIGTERM, &noaction, &oldaction) < 0)
		report_error("can't ignore SIGTERM signal")

	if (sigaction(SIGUSR1, &action, NULL) < 0)
		report_error("can't catch SIGUSR1 signal")

	while(1)
		sleep(2);
	return 0;
}