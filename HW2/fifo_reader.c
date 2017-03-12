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

#define BUFF_SIZE 4096
#define report_error(str) {printf("Error: %s - %s, line %d\n",(str), strerror(errno), __LINE__); exit(errno);}

int main(void)
{	
	struct sigaction oldaction;
	struct sigaction noaction;

	if (sigaction(SIGINT, &noaction, &oldaction) < 0)
		report_error("can't ignore SIGINT signal")

	sleep(1);

	int i;
	int num_read, b_read = 0;
	char *filename = "/tmp/osfifo";
	int ffd;
	char inBUFF[BUFF_SIZE];

	if((ffd = open(filename, O_RDONLY)) < 0)
		report_error("can't opne the fifo file for reading")

	struct timeval t1,t2;

	double elapsed_microsec;

	if(gettimeofday(&t1, NULL) < 0) //start time measurement
		report_error("can't get time of day")

	while((num_read = read(ffd, inBUFF, BUFF_SIZE)) >= 0)
	{
		if(num_read == 0)
			break;
		for (i = 0; i < num_read; ++i)
		{
			if(inBUFF[i] == 'a')
				++b_read;
		}
	}
	if(num_read < 0)
		report_error("can't read from fifo file")
	close(ffd);

	if(gettimeofday(&t2, NULL) < 0) //end time measurement
		report_error("can't get time of day")
	
	elapsed_microsec = (t2.tv_sec - t1.tv_sec) * 1000.0;
  	elapsed_microsec += (t2.tv_usec - t1.tv_usec) / 1000.0;

  	printf("%f milliseconds passed\n", elapsed_microsec);
  	printf("%s%d\n","the size read from file is ",b_read);

  	if (sigaction(SIGINT, &oldaction, NULL) < 0)
			report_error("can't restore SIGINT signal")
	return 0;
}