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


int main(int argc, char const *argv[])
{
	struct sigaction oldaction;
	struct sigaction noaction;

	if (sigaction(SIGTERM, &noaction, &oldaction) < 0)
		report_error("can't ignore SIGTERM signal")

	if(argc != 3)
	{
		printf("%s\n", "usage: mwriter <NUM> <RPID>");
		exit(-1);
	}

	int i;

	char *temp;
	int fsize = (int) strtol(argv[1], &temp, 10);
	if (*temp)
	{
		printf("can't convert NUM to int using strtol, %s\n" ,argv[1]);
		exit(-1);
	}
	int PRID = (int) strtol(argv[2], &temp, 10);
	if (*temp)
	{
		printf("can't convert RPID to int using strtol, %s\n" ,argv[2]);
		exit(-1);
	}

	int mfd;
	char *arr;
	int swriten = 0;
	char *filename = "/tmp/mmapped.bin";

	if((mfd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR))< 0)
	{	
		if(errno != EEXIST)
			report_error("can't open/creat mmapped file")
		if(chmod(filename, 0600) < 0)
			report_error("can't change existing file permission")
	}

  	if (lseek(mfd, fsize-1, SEEK_SET) < 0)
    	report_error("problem calling lseek function")

  	if (write(mfd, "", 1) < 0) //make the file size automaticly fsize
	  	report_error("can't write last byte of the file")

	arr = (char*) mmap(	NULL, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, mfd, 0);

	if(arr == MAP_FAILED)
		report_error("can't mmap the file")

	struct timeval t1,t2;

	double elapsed_millisec;

	if(gettimeofday(&t1, NULL) < 0) //start time measurement
		report_error("can't get time of day")

	for (i = 0; i < fsize-1; ++i)
	{
		arr[i] = 'a';
	}
	arr[fsize-1] = '\0';

	if(munmap(arr,fsize) < 0)
  		report_error("can't munmap the file")
  	close(mfd);

	if(kill(PRID,SIGUSR1) < 0)
		report_error("can't send signal to PRID")

	if(gettimeofday(&t2, NULL) < 0) //end time measurement
		report_error("can't get time of day")
	
	elapsed_millisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
  	elapsed_millisec += (t2.tv_usec - t1.tv_usec) / 1000.0;

  	printf("%f milliseconds passed\n", elapsed_millisec);
  	printf("%s%d\n","the size written to the file is ",fsize);

  	if (sigaction(SIGTERM, &oldaction, NULL) < 0)
		report_error("can't recover SIGTERM signal")
	return 0;
}