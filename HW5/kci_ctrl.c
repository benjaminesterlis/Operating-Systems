#include "kci.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#define BUF_SIZE 1024

#define INIT "-init"
#define PID "-pid"
#define FD "-fd"
#define START "-start"
#define STOP "-stop"
#define RM "-rm"

// #define CONCAT(STR1, STR2, ST) STR1 STR2 
#define DEV "/dev/" DEVICE_FILE_NAME
// #define DEV_S "/dev/" DEVICE_R_NAME
#define CALLS "/sys/kernel/debug/kcikmod/calls" // maybe kci_kmod
#define cwdCALLS "./calls"


#define report_error(str) {printf("Error: %s - %s, line %d\n",(str), strerror(errno), __LINE__); exit(errno);} 
#define print_usage() \
	do { \
		printf("usage: <CMD> [arg]\n%s\n%s\n%s\n%s\n%s\n%s\n", \
			"-init <KO>", \
			"-pid <PID>", \
			"-fd <FD>", \
			"-start", \
			"-stop", \
			"-rm"); \
		exit(-1); \
	} while (0)

// credit: https://stackoverflow.com/questions/5947286/how-can-linux-kernel-modules-be-loaded-from-c-code
// or shorten path : https://goo.gl/bHGVjA
#define init_module(mod, uargs, flags) syscall(__NR_finit_module, mod, uargs, flags)
#define delete_module(name, flags) syscall(__NR_delete_module, name, flags)

int main(int argc, char const *argv[])
{
	if (argc == 3)
	{
		if (!strcmp(argv[1],INIT))
		{
			int fd;

			if ((fd = open(argv[2], O_RDONLY)) < 0)
				report_error("can't open given file")

			if(init_module(fd, "", 0) != 0)
				report_error("can't init module")

			close(fd);

			if (mknod(DEV, S_IFCHR | 0777 , makedev(MAJOR_NUM, 0)) < 0)
			{
				if (errno != EEXIST)
				{
					report_error("can't mknod with given device: "DEV)	
				}
			}

			return 0;
		}
		if(!strcmp(argv[1],PID))
		{
			int dev_fd, ret_val;
			pid_t pid;

			char *temp;
			pid = (pid_t)strtol(argv[2], &temp, 10);

			if ((dev_fd = open(DEV, O_WRONLY)) < 0)
				report_error("can't open device file: "DEVICE_FILE_NAME)

			if ((ret_val = ioctl(dev_fd, IOCTL_SET_PID, pid)) < 0)
				report_error("ioctl failed")

			close(dev_fd);
			return 0;
		}
		if (!strcmp(argv[1],FD))
		{
			int dev_fd, ret_val;
			int fd;

			char *temp;
			fd = (int)strtol(argv[2], &temp, 10);

			if ((dev_fd = open(DEV, 0)) < 0)
				report_error("can't open device file: "DEVICE_FILE_NAME)

			if ((ret_val = ioctl(dev_fd, IOCTL_SET_FD, fd)) < 0)
				report_error("ioctl failed")

			close(dev_fd);
			return 0;
		}
		else
		{
			print_usage();
		}
	} 
	else if (argc == 2)
	{
		if (!strcmp(argv[1],START))
		{
			int dev_fd, ret_val;

			if ((dev_fd = open(DEV, 0)) < 0)
				report_error("can't open device file: "DEVICE_FILE_NAME)

			if ((ret_val = ioctl(dev_fd, IOCTL_CHIPER, 1)) < 0)
				report_error("ioctl failed")

			close(dev_fd);
			return 0;
		}
		if (!strcmp(argv[1],STOP))
		{
			int dev_fd, ret_val;

			if ((dev_fd = open(DEV, 0)) < 0)
				report_error("can't open device file: "DEVICE_FILE_NAME)

			if ((ret_val = ioctl(dev_fd, IOCTL_CHIPER, 0)) < 0)
				report_error("ioctl failed")

			close(dev_fd);
			return 0;
		}
		if (!strcmp(argv[1],RM))
		{	
		    int iter = 0;
			int Ifd, Ofd;
			char buff[BUF_SIZE];
			int n_read, n_wrote;
			
			if ((Ifd = open(CALLS, O_RDONLY)) < 0)
				report_error("can't open "CALLS" for reading")
			if ((Ofd = open(cwdCALLS, O_WRONLY | O_TRUNC | O_CREAT,
									S_IRWXU | S_IRWXG | S_IRWXO)) < 0)
			{
				report_error("can't open/create calls file in cwd")
			}

			while((n_read = read(Ifd, buff, BUF_SIZE)) > 0)
			{
			    iter++;
				if ((n_wrote = write(Ofd, buff, n_read)) != n_read)
					report_error("can't write into \'calls\' file in cwd")
			}
			if (n_read == -1)
			{
				report_error("can't read from "CALLS)
			}
			close(Ofd);
			close(Ifd);

			if ((delete_module("kci_kmod", O_NONBLOCK)) != 0)
				report_error("can't delete the module")

			if(remove(DEV) != 0)
				report_error("can't remove the dev file")

			return 0;
		}
		else
			print_usage();
	} 
	else 
	{
		print_usage();
	}
	return 0;
}
