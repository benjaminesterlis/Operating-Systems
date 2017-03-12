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
int ffd;
int sig = 1;

void handler(int signum) //https://www.linuxquestions.org/questions/programming-9/how-to-handle-a-broken-pipe-exception-sigpipe-in-fifo-pipe-866132/
{
	if (signum == SIGPIPE)
	{
		sig = 0;

		// if (signal(SIGINT, SIG_DFL) == SIG_ERR)
		// 	report_error("can't restore SIGINT signal")
	}
}

int main(int argc, char const *argv[])
{
	if(argc != 2)
	{
		printf("%s\n", "usage: fwriter <NUM>");
		exit(-1);
	}

	// printf("%s\n", "before sigaction");

	struct sigaction action;
	struct sigaction noaction;
	struct sigaction oldaction;

	// printf("%s\n", "after init sigaction");
	action.sa_handler = handler;
	action.sa_flags = 0;

	// printf("%s\n", "set sig handler for sigaction");

	if (sigaction(SIGINT, &noaction, &oldaction) < 0)
		report_error("can't ignore SIGINT signal")

	if (sigaction(SIGPIPE, &action, NULL) < 0)
		report_error("can't handel SIGPIPE signal")

	int i;
	int writen = 0;
	char *temp;
	int fsize = (int)strtol(argv[1], &temp, 10);
	if (*temp)
	{
		printf("can't convert NUM to int using strtol, %s\n" ,argv[1]);
		exit(-1);
	}
	char outBUFF[BUFF_SIZE];
	char *filename = "/tmp/osfifo";
	struct stat sb;

	// printf("%s\n", "hello");

	if (mkfifo(filename, 0600) < 0) //https: //stackoverflow.com/questions/2784500/how-to-send-a-simple-string-between-two-programs-using-pipes
	{	
		if(errno != EEXIST)
			report_error("can't create fifo file")
		if(stat(filename,&sb) < 0)
			report_error("can't use stat function")
		if(!S_ISFIFO(sb.st_mode))
			report_error("the file is not fifo file")
		if(chmod(filename, 0600) < 0)
			report_error("can't change existing file permission")
	}

	if((ffd = open(filename,O_WRONLY)) < 0)
		report_error("can't open the fifo file")

	struct timeval t1,t2;

	double elapsed_microsec;

	if(gettimeofday(&t1, NULL) < 0) //start time measurement
		report_error("can't get time of day")

	for (i = 0; i < BUFF_SIZE; ++i)
		outBUFF[i]='a';

	i = 0;
	while(writen + BUFF_SIZE < fsize && sig)
	{
		if((i = write(ffd, outBUFF, BUFF_SIZE)) < 0)
		{
			if(errno != EPIPE)
				report_error("can't write to the file")
		}
		else
			writen += i;
	}

	if(sig)
	{
		if((i = write(ffd, outBUFF, fsize - writen)) < 0)
		{
			if(errno != EPIPE)
				report_error("can't write to the file")	
		}
		else
			writen += i;
	}
	close(ffd);

	if(gettimeofday(&t2, NULL) < 0) //end time measurement
		report_error("can't get time of day")
	
	elapsed_microsec = (t2.tv_sec - t1.tv_sec) * 1000.0;
  	elapsed_microsec += (t2.tv_usec - t1.tv_usec) / 1000.0;

  	printf("%f milliseconds passed\n", elapsed_microsec);
  	printf("%s%d\n","the size written to the file is ",writen);

  	if(unlink(filename) < 0)
  		report_error("can't unlink the file")
  	
  	if (sigaction(SIGINT, &oldaction, NULL) < 0)
			report_error("can't restore SIGINT signal")

    if (!sig)         
    	report_error("Program Got SIGPIPE signal")     
    return 0;
}
