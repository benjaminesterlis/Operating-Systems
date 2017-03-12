#include <time.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#define PATH "/home/benjamin/Desktop/output"
#define CWD "./output"

int main(void)
{
	int fd, wrote;
	char buff = 'a';
	char buffer[1];
	if ((fd=open(CWD, O_RDWR | O_TRUNC | O_CREAT, 0777)) < 0)
	{
		printf("%s\n", "cant open the file");
		return errno;
	}

	printf("pid=%d, fd=%d\n", getpid(), fd);
	sleep(30);
	printf("woke up from sleep\n");

	for (int i = 0; i < 2000; ++i)
	{
		if((wrote=write(fd, &buff, 1))<0)
		{
			printf("%s %d\n", "error writing", wrote);
			return errno;
		}
	}
	if(lseek(fd, SEEK_SET, 0) < 0)
	{
		printf("cant lseek to the start");
		return errno;
	}
	printf("a=%c\n", buff);
	printf("Done writing to the file\n");
	for (int i = 0; i < 2000; ++i)
	{	
		//printf("in the %d iter, ", i);
		if((wrote=read(fd, buffer, 1))<0)
		{
			printf("%s %d\n", "error writing", wrote);
			printf("errno set to=:%d, filedes=%d\n", errno, fd);
			return errno;
		}
		//printf("wrote %d\n", wrote);
		//printf("%c",buffer[0]);
	}
	printf("Done reading from the file\n");
	close(fd);
	return 0;
}
