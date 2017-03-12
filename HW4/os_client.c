#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <errno.h>
#include <fcntl.h>

#define BUFF_SIZE 1024
#define report_error(str) {printf("Error: %s - %s, line %d\n",(str), strerror(errno), __LINE__); exit(errno);}

void print_buff(char buff[BUFF_SIZE],char *string);

int main(int argc, char const *argv[])
{
	if (argc != 5)
	{
		printf("usage: client <IP> <PORT> <IN> <OUT>\n");
		exit(-1);
	}
	char *temp;
	int port = (int)strtol(argv[2], &temp, 10);
	if (*temp || port > 65535 || port < 0)
	{
		printf("invalid port specified, %s\n" ,argv[2]);
		exit(-1);
	}

	int Ifd, Ofd;

	struct sockaddr_in serv_addr = {0};
	socklen_t addrsize = sizeof(struct sockaddr_in);

	char buff[BUFF_SIZE];
	int byte_read_in, byte_wrote_sock, byte_wrote_out;

	int sockfd = 0;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        report_error("can't create socket\n") 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port); 
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

    printf("connecting...\n"); 
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
       report_error("can't connect to the socket")

   	printf("Clinet: conncted to server, %s, pid=%d\n", inet_ntoa(serv_addr.sin_addr), getpid());

	if((Ifd = open(argv[3],O_RDONLY)) < 0)
		report_error("can't open input file")
	if((Ofd = open(argv[4], O_WRONLY | O_TRUNC | O_CREAT, 0600)) < 0) //credit to man page
		report_error("problem with creating output file or openning it")

	while((byte_read_in = read(Ifd, buff, BUFF_SIZE)) > 0)
	{
		if((byte_wrote_sock = send(sockfd,buff,byte_read_in,0)) != byte_read_in)
			report_error("can't write to socket file");

		if((byte_wrote_out = recv(sockfd, buff, byte_read_in, 0)) != byte_read_in)
			report_error("can't reat from socket file")

		if((byte_wrote_out = write(Ofd, buff, byte_read_in)) != byte_read_in)
			report_error("can't write to output file")
	}
	if(byte_read_in < 0)
		report_error("can't read from input file")
	
	printf("%s\n","Clinet: All data had been sent to server");

	close(Ifd);
	close(Ofd);
	close(sockfd);
	printf("%s\n", "Clinet: Done");
	
	return 0;
}
