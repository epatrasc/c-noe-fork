#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE    100

/*
 * (1) gcc test-fifo-server.c -o test-fifo-server
 * (2) ./test-fifo-server fifo_file server.log  &
 *   - argv[1] is the name of the FIFO to be created
 *   - argv[2] is the log file where to write all data read from FIFO
 *   - "&" is used to launch a process on background
 */

/* Creates a server that reads from a FIFO named argv[1] and writes
   over argv[2] */
int main(int argc, char * argv[])
{
	int fifo_a, fifo_b, str_len;
	char * readbuf=calloc(sizeof(char), BUF_SIZE);
	ssize_t num_bytes;
	char my_msg[BUF_SIZE];
    int flg_continua=5;
	/* Create the FIFO if it does not exist */
	mkfifo("chil_a", S_IRUSR | S_IWUSR);
	
	/*
	 * Next code will keep reading forever (server needs to be
	 * kill/term by signal or else)
	 *
	 * The FIFO must be opened and closed everytime. Otherwise the
	 * server will exit as soon as the client closes its write
	 * file descriptor
	 */
	printf("start\n");
	while(flg_continua) {
		fifo_a = open("chil_a", O_RDONLY);
        printf("giro \n");
		while (num_bytes = read(fifo_a, readbuf, BUF_SIZE)) {
            if(num_bytes>0){
			    printf("out: %s\n",readbuf);
                int i=0;
                char *token = calloc(sizeof(char),strlen(readbuf)+1);
                char *array[2];
                while ((token = strsep(&readbuf, ","))) {
                    //strcpy(array[i], token);
                    printf("token[%d]:%s\n",i++,token);
                }
                free(token);

                fifo_b = open("chil_b", O_WRONLY);
                str_len = sprintf(my_msg,"PID_A=%d", getpid());
                write(fifo_b, my_msg, str_len);
                close(fifo_b);
            } 
		}
        flg_continua--;
		close(fifo_a);
	}
    remove("chil_a");
	free(readbuf);
	return(0);
}
