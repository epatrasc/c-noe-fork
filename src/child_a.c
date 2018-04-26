#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

extern char **environ;
#define NUM_PROC 5
#define TEST_ERROR                                \
	if (errno)                                    \
	{                                             \
		printf("%s:%d: PID=%5d: Error %d (%s)\n", \
			   __FILE__,                          \
			   __LINE__,                          \
			   getpid(),                          \
			   errno,                             \
			   strerror(errno));                  \
	}

const int SHMFLG = IPC_CREAT | 0666;

struct individuo
{
	char tipo;
	char *nome;
	unsigned long genoma;
	pid_t pid;
	bool alive;
};

struct child_a
{
	unsigned long genoma;
	pid_t pid;
	bool alive;
};

struct shared_data
{
	unsigned long cur_idx;
	struct child_a children_a[100];
};

key_t key = 1060;

int main(int argc, char *argv[])
{
	unsigned int i, shmid;
	char *my_val, nome;
	struct shared_data *shdata;

	printf("\n ---> CHILD A | pid: %d <---\n", getpid());
	// if (argc == 1)
	// {
	// 	shmid = atoi(argv[0]);
	// 	printf("argv[0]: %d \n", shmid);
	// }

	if ((shmid = shmget(key, sizeof(shdata), SHMFLG)) == 0)
	{
		perror("cannot get shared memory id | shdata\n");
		exit(EXIT_FAILURE);
	}

	if ((shdata = shmat(shmid, NULL, 0)) == (void *)-1)
	{
		perror("cannot attach shared memory to address \n");
		exit(EXIT_FAILURE);
	}

    // if ((shmid = shmget(key+ 1, shdata->cur_idx * sizeof(shdata->individui),SHMFLG)) < 0)
    // {
    //     perror("cannot get shared memory id | shdata->individui \n");
    //     exit(EXIT_FAILURE);
    // }

    // if ((shdata->individui = shmat(shmid, NULL, 0)) < 0)
    // {
    //     perror("cannot attach shared memory to address shdata->individui \n");
    //     exit(EXIT_FAILURE);
    // }

    // if ((shmid = shmget(key+ 2, sizeof(char *),SHMFLG)) < 0)
    // {
    //     perror("cannot get shared memory id | shdata->individui->nome\n");
    //     exit(EXIT_FAILURE);
    // }

    // if ((shdata->individui[0].nome = shmat(shmid, NULL, 0)) < 0)
    // {
    //     perror("cannot attach shared memory to address shdata->individui->nom \n");
    //     exit(EXIT_FAILURE);
    // }


    for (int i = 0; i < shdata->cur_idx; i++)
	{
		struct child_a child = shdata->children_a[i];
		printf("current pid: %d | [%d] genoma: %lu | child_a pid: %d \n", getpid(), i, child.genoma, child.pid);

	}

	exit(EXIT_SUCCESS);
}
