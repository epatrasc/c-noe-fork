#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>

struct individuo {
    char tipo;
    char *nome;
    unsigned long genoma;
    pid_t pid;
    bool alive;
};

struct child_a {
    unsigned long genoma;
    pid_t pid;
    bool alive;
};

struct shared_data {
    unsigned long cur_idx;
    struct child_a *children_a;
};

void init_shmemory();

const int SHMFLG = IPC_CREAT | 0666;
const key_t key = 1060;
unsigned int shmid;
struct shared_data *shdata;

int main(int argc, char *argv[]) {
    char *my_val, nome;
    struct individuo my_info;

    printf("\n ---> CHILD B START | pid: %d <---\n", getpid());

    if (argc < 2) {
        printf("I need name and genoma input from argv. \n");
        for (int i = 0; i < argc; i++) {
            printf("arg[%d]: %s \n ", i, argv[i]);
        }
        exit(EXIT_FAILURE);
    }

    my_info.nome = argv[1];
    my_info.tipo = 'B';
    my_info.genoma = (unsigned long) strtol(argv[2], NULL, 10);

    printf("my_info.nome: %s \n", my_info.nome);
    printf("my_info.tipo: %c \n", my_info.tipo);
    printf("my_info.genoma: %lu \n", my_info.genoma);

    init_shmemory();

    for (int i = 0; i < shdata->cur_idx; i++) {
        if (shdata->children_a[i].alive == 0) {
            printf("shdata->children_a[%d] it's NULL \n", i);
            continue;
        }

        struct child_a child = shdata->children_a[i];
        printf("my_pid: %d | [%d] genoma: %lu | child_a pid: %d \n", getpid(), i, child.genoma, child.pid);

    }

    // TODO ho scelto A

    // Write message to A
    char *pida_s =calloc(sizeof(char), 6);
    sprintf(pida_s, "%d", 32583);

    int fifo_a = open(pida_s, O_WRONLY);
    char *my_msg = calloc(sizeof(char), 1024);

    int str_len = sprintf(my_msg, "%d,%s,%lu", getpid(), my_info.nome, my_info.genoma);
    write(fifo_a, my_msg, str_len);
    free(my_msg);
    free(pida_s);
    close(fifo_a);

    //create fifo
    int BUF_SIZE = 1024;
    char *pid_s = calloc(sizeof(char), 6);
    char *readbuf = calloc(sizeof(char), BUF_SIZE);
    sprintf(pid_s, "%d", getpid());
    mkfifo(pid_s, S_IRUSR | S_IWUSR);

    // read answer
    int fifo_b = open(pid_s, O_RDONLY);
    ssize_t num_bytes = read(fifo_b, readbuf, BUF_SIZE);
    printf("num_bytes: %li\n", num_bytes);
    printf("out: %s\n", readbuf);

    close(fifo_a);
    close(fifo_b);
    remove(pid_s);
    free(readbuf);
    free(pid_s);

    printf("\n ---> CHILD B END | pid: %d <---\n", getpid());

    exit(EXIT_SUCCESS);
}


void init_shmemory() {
    if ((shmid = shmget(key, sizeof(shdata), SHMFLG)) == 0) {
        perror("cannot get shared memory id | shdata\n");
        exit(EXIT_FAILURE);
    }

    if ((shdata = shmat(shmid, NULL, 0)) == (void *) -1) {
        perror("cannot attach shared memory to address \n");
        exit(EXIT_FAILURE);
    }

    if ((shmid = shmget(key + 1, getpagesize(), SHMFLG)) == 0) {
        perror("cannot get shared memory id | shdata->children_a \n");
        exit(EXIT_FAILURE);
    }

    if ((shdata->children_a = shmat(shmid, NULL, 0)) < 0) {
        perror("cannot attach shared memory to address shdata->children_a \n");
        exit(EXIT_FAILURE);
    }
}