#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>

extern char **environ;
#define NUM_PROC 5
#define TEST_ERROR                                     \
		if (errno)                                     \
		{                                              \
			printf("%s:%d: PID=%5d: Error %d (%s)\n",  \
					__FILE__,                          \
					__LINE__,                          \
					getpid(),                          \
					errno,                             \
					strerror(errno));                  \
		}

struct individuo {
    char tipo;
    char *nome;
    unsigned long genoma;
};

struct shared_data {
	unsigned long cur_idx;
	struct individuo individui[NUM_PROC];
};

int main(int argc, char * argv[]) {
	unsigned int i, shid;
	char * my_val, nome;
	struct shared_data *my_data;

	printf("\n ---> CHILD A | pid: %d <---\n", getpid());
	if (argc == 1) {
		shid =  atoi(argv[0]);
		printf("argv[0]: %d \n",shid);
	}

	if ((my_data = shmat(shid, NULL, 0)) == (void *)-1) {
		perror("cannot attach shared memory to address\n");
		exit(1);
    }else{
		printf("attach shared memory: ok\n");
	}

	for (int i = 0; i < my_data->cur_idx; i++) {
		printf("shm pid: %d | [%d] genoma: %lu \n", getpid(), i, my_data->individui[i].genoma);
	}

	exit(EXIT_SUCCESS);
}
