#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <time.h> // for time measurement
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096
#define report_error(str) {printf("Error: %s - %s , line %d\n",(str), strerror(errno), __LINE__); exit(1);}

void do_enc_dec(char *,char *,char *);
void endec_array(char arr[BUF_SIZE], char[BUF_SIZE]);

int main(int argc, char const *argv[])
{
	if (argc != 4)
	{
		printf("%s\n", "usage: cipher <input_directory> <encryption_key> <output_directory>");
		exit(1);
	}
    
	DIR *input_dir, *output_dir;
	char input_file[1024], output_file[1024];

	if((input_dir = opendir(argv[1])) == NULL)
		report_error("the input directory cant be open")

	if((output_dir = opendir(argv[3])) == NULL)
	{
		if(mkdir(argv[3],0777)<0)
			report_error("the output directory cant be created")

		printf("%s\n", "output directory had been created");
		if((output_dir = opendir(argv[3])) == NULL)
			report_error("the output directory cant be open")
	}

	struct dirent *dp;
	struct stat sb;

	while((dp = readdir(input_dir)) != NULL) // credit to recitations
	{
		sprintf(input_file,"%s/%s",argv[1],dp->d_name);
		sprintf(output_file,"%s/%s",argv[3],dp->d_name);

		// if ((stat(input_file, &sb)) == -1)
		// 	report_error("cant use stat function")
		// // skip directories
		// if ((sb.st_mode & S_IFMT) == S_IFDIR) {
		// 	continue;
		// } // credit to man page
		if((strcmp(dp->d_name,".") == 0) || (strcmp(dp->d_name,"..") == 0)){
			continue;
		}

		do_enc_dec(input_file,(char *)argv[2],output_file);
	}
	closedir(input_dir); // check fault
	closedir(output_dir);

	return 0;
}

void do_enc_dec(char *input_file,char *cipher_file,char *output_file)
{
	int Ifd,Ofd,Cfd;
    int first = 0;

	if((Ifd = open(input_file,O_RDONLY)) < 0)
		report_error("cant open input file");
	if((Cfd = open(cipher_file,O_RDONLY)) < 0)
		report_error("cant open key file")
	if((Ofd = open(output_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) //creadit to man page
		report_error("problem with creating output file or openning it")

	char buf_in[BUF_SIZE], buf_ci[BUF_SIZE];
	int byte_read_ci , byte_read_in ,byte_wrote_out;
    
    byte_read_ci = read(Cfd ,buf_ci, BUF_SIZE);
    if(byte_read_ci < 0)
		report_error("cant read from key file")
    if(byte_read_ci == 0)
        report_error("key file is empty")
    
    //printf("byte read key - %d\n", byte_read_ci);
    
	while(byte_read_ci != -1)
	{
        if(first != 0){
            byte_read_ci = read(Cfd ,buf_ci, BUF_SIZE);
            if(byte_read_ci < 0)
				report_error("cant read from key file")
            
            //printf("byte read key - %d\n", byte_read_ci);
            
            if(byte_read_ci == 0)
            {
                close(Cfd);
                Cfd = open(cipher_file,O_RDONLY);
                if(Cfd < 0)
                    report_error("cant open key file")
                continue;
            }
        }
        
		byte_read_in = read(Ifd ,buf_in, byte_read_ci);
		
		//printf("byte read in - %d\n", byte_read_ci);

		if(byte_read_in < 0)
			report_error("cant read from input file")
		if(byte_read_in == 0) break;
        //printf("%s\n", buf_in);
        endec_array(buf_in, buf_ci);

		if((byte_wrote_out = write(Ofd,buf_in,byte_read_in)) != byte_read_in)
			report_error("problem while writing to output file");
        
        //printf("byte wrote out- %d\n", byte_wrote_out);
        
        first = 1;
		
	}
	if (byte_read_in < 0)
		report_error("problem reading from key file");

	close(Ifd); // check fault
	close(Ofd);
	close(Cfd);
}

void endec_array(char arr[BUF_SIZE], char key[BUF_SIZE])
{
	int i;
	for (i = 0; i < BUF_SIZE; ++i)
		arr[i] ^= key[i];
}

