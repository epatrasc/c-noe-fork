#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE    100

/*
 * (1) gcc test-fifo-client.c -o test-fifo-client
 * (2) ./test-fifo-client fifo_file mio_messaggio  &
 */

/* Creates a client that writes to a FIFO named argv[1] the message
   argv[2] plus other client info */
int main(int argc, char * argv[])
{
	int fifo_a,fifo_b, str_len;
	char my_msg[BUF_SIZE];
	ssize_t num_bytes;
	char * readbuf=calloc(sizeof(char), BUF_SIZE);

    //create fifo
    mkfifo("chil_b", S_IRUSR | S_IWUSR);

	// Open FIFO in write mode
	fifo_a = open("chil_a", O_WRONLY);
	
	// Assemble the message
	str_len = sprintf(my_msg,"%d,%s",
			  getpid(),
			  "1234567890");

	// Write message to FIFO
	write(fifo_a, my_msg, str_len);

    fifo_b = open("chil_b", O_RDONLY);
    num_bytes = read(fifo_b, readbuf, BUF_SIZE);
    printf("num_bytes: %li\n",num_bytes);
	printf("out: %s\n",readbuf);

    close(fifo_a);
	close(fifo_b);
    free(readbuf);
    remove("chil_b");
	return(0);
}
