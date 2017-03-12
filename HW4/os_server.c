#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

#define BUFF_SIZE 1024
#define RANDOM "/dev/urandom"

#define report_error(str) \
	do {\
		if(errno != EINTR){\
			printf("Error: %s - %s, line %d\n",(str), strerror(errno), __LINE__);\
			exit(errno);\
		}\
	} while(0);

void endec_array(char [BUFF_SIZE], char[BUFF_SIZE]);
void print_buff(char buff[BUFF_SIZE],char *string);

int stop = 1;

void handler(int signum)
{
	if (signum == SIGINT)
	{
		stop = 0;
		printf("\nSIGINT signal received , pid=%d\n",getpid());
	}
}

int main(int argc, char const *argv[])
{
	if (argc != 3 && argc != 4)
	{
		printf("usage: server <PORT> <KEY> [<KEYLEN>]\n");
		exit(-1);
	}
	char *temp;
	int port = (int)strtol(argv[1], &temp, 10);
	if (*temp || port > 65535 || port < 0)
	{
		printf("invalid port specified, %s\n" ,argv[1]);
		exit(-1);
	}
	int pid;
	int key,num=0, wrote=0, randfd;
	char buff[BUFF_SIZE], buf_key[BUFF_SIZE];

	if (argc == 4)
	{
		int keylen = (int)strtol(argv[3], &temp, 10);
		if (*temp || keylen <= 0)
		{
			printf("KEYLEN must be positive integer, %s\n" ,argv[3]);
			exit(-1);
		}
		if((key = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) //credit to man page
			report_error("can't create/open key file")

		if ((randfd = open(RANDOM, O_RDONLY)) < 0)
			report_error("can't open the /dev/urandom file")

		/* can be done by buff[keylen] but this solution is bad if the keylen is to long */
		while((num = read(randfd, buff, BUFF_SIZE)) > 0 && wrote < keylen) 
		{
			if (wrote + num < keylen)
			{
				if((num = write(key, buff, num)) < 0)
					report_error("can't write to the key file")
			}
			else{
				if((num = write(key, buff, keylen - wrote)) < 0)
					report_error("can't write to the key file")
			}
			wrote += num;
		}
		if (num < 0)
			report_error("can't read from /dev/urando")
		close(key);
		close(randfd);
	}
	else 
	{
		// if((key = open(argv[2], O_RDONLY)) < 0) //credit to man page
		// 	report_error("can't open key file")
		struct stat st;

		if(stat(argv[2], &st))
			report_error("can't stat the file key")

		if (st.st_size == 0)
			report_error("the key file is empty")
	}

	struct sockaddr_in serv_addr = {0} ,cli_addr;
	socklen_t addrsize = sizeof(struct sockaddr_in);

	int listenfd, connfd;
	int byte_read_in, byte_read_key = 0, byte_wrote_out;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any local machine address
    serv_addr.sin_port = htons(port);

    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
       report_error("can't bind the socket")

    if(listen(listenfd, 10))
		report_error("can't listen on the socket")

	struct sigaction action;

	action.sa_handler = handler;
	action.sa_flags = 0;

	if (sigaction(SIGINT, &action, NULL) < 0)
		report_error("can't handle SIGINT signal")

	printf("Server is up...\n");
    while(stop)
    {
	    if((connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &addrsize)) < 0)
			report_error("can't accept the socket")

		if(!stop)
		{
			printf("%s\n", "you shall not pass!!!");
			break;
		}

       	if((pid = fork()) == 0)
       	{	
			printf("Server: conncted to client, %s, pid=%d\n", inet_ntoa(cli_addr.sin_addr), getpid());

       		close(listenfd);

       		if((key = open(argv[2], O_RDONLY)) < 0) //credit to man page
				report_error("can't open key file")

			if(lseek(key, SEEK_SET, 0) < 0)
				report_error("can't lseek to the beggining of the key file")

       		while((byte_read_in = recv(connfd, buff, BUFF_SIZE,0)) > 0 && stop) //credit to solution given in class to cipher
			{
				while(byte_read_key < byte_read_in && stop)
				{
					if((num = read(key, buf_key+byte_read_key,byte_read_in - byte_read_key)) < 0)
						report_error("can't read from key file")
					else if(!num)
					{
						if(lseek(key, SEEK_SET, 0) < 0)
							report_error("can't lseek to the beggining of the key file")
					}
					else
						byte_read_key += num;
				}

				endec_array(buff, buf_key); 

				if((byte_wrote_out = send(connfd, buff, byte_read_in,0)) != byte_read_in) // if i dont send the client will wait forever
					report_error("can't write to output file")
			}
			if(byte_read_in < 0)
				report_error("can't read from client socket")
			// printf("%s\n", "Server: Data had been sent to the client");
			close(key);
			close(connfd);
			printf("Server - SubProcess: Done\n");
			exit(0);
    	}
       	else if(pid < 0)
       		report_error("can't fork new process")
		close(connfd);
    }
    close(listenfd);
    printf("Server: Done\n");
	return 0;
}

void endec_array(char arr[BUFF_SIZE], char key[BUFF_SIZE])
{
	int i;
	for (i = 0; i < BUFF_SIZE; ++i)
		arr[i] ^= key[i];
}
