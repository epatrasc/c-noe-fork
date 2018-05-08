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
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/msg.h>


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

int mcd(unsigned long a, unsigned long b) {
    int remainder;
    while (b != 0) {
        remainder = a % b;

        a = b;
        b = remainder;
    }
    return a;
}

bool isGood(unsigned long gen_a, unsigned long gen_b) {
    if (gen_a % gen_b == 0) {
        return true;
    }

    if (mcd(gen_a, gen_b) >= 1) {
        return true;
    }

    return false;
}

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
        printf("B | I need name and genoma input from argv. \n");
        for (int i = 0; i < argc; i++) {
            printf("B | arg[%d]: %s \n ", i, argv[i]);
        }
        exit(EXIT_FAILURE);
    }

    my_info.nome = argv[1];
    my_info.tipo = 'B';
    my_info.genoma = (unsigned long) strtol(argv[2], NULL, 10);

    printf("B | my_info.nome: %s \n", my_info.nome);
    printf("B | my_info.tipo: %c \n", my_info.tipo);
    printf("B | my_info.genoma: %lu \n", my_info.genoma);

    init_shmemory();

    struct child_a child;
    int choice_id, foundMate=0;
    int num_of_death=0;

    char *pid_s = calloc(sizeof(char), 6);
    sprintf(pid_s, "%d", getpid());
    mkfifo(pid_s, S_IRUSR | S_IWUSR);

    for (int i = 0; i < shdata->cur_idx && !foundMate; i++) {
        child = shdata->children_a[i];
        printf("B | my_pid: %d | shdata -> [%d] genoma: %lu | child_a pid: %d | alive: %d \n", getpid(), i, child.genoma, child.pid,
               child.alive);
        if (shdata->children_a[i].alive == 0) {
            printf("B | shdata->children_a[%d] NOT alive \n", i);
            num_of_death++;
            if(num_of_death==shdata->cur_idx){
                printf("B | PID %d: GAME OVER FOR ME\n", getpid());
                exit(EXIT_FAILURE);
            }

            continue;
        }

        //evaluate candidate
        int answer = (int) isGood(child.genoma, my_info.genoma);
        printf("B | GEN_A: %lu, GEN_B: %lu are compatible? %d\n",child.genoma, my_info.genoma,answer);
        if (answer) {
            choice_id = i;
            // Write message to A
            char *pida_s = calloc(sizeof(char), 6);
            sprintf(pida_s, "%d", child.pid);

            int fifo_a = open(pida_s, O_WRONLY);
            if(fifo_a < 0){
                sleep(1);
                fifo_a = open(pida_s, O_WRONLY);
            }

            char *my_msg = calloc(sizeof(char), 1024);

            printf("B | PID: %d | writing to A\n", getpid());
            int str_len = sprintf(my_msg, "%d,%s,%lu", getpid(), my_info.nome, my_info.genoma);
            write(fifo_a, my_msg, str_len);
            free(my_msg);
            free(pida_s);
            close(fifo_a);

            //create fifo
            int BUF_SIZE = 1024;
            char *readbuf = calloc(sizeof(char), BUF_SIZE);

            // read answer
            printf("B | PID: %d | reading response from A...\n", getpid());
            int fifo_b = open(pid_s, O_RDONLY);
            ssize_t num_bytes = read(fifo_b, readbuf, BUF_SIZE);
            printf("B | num_bytes: %li\n", num_bytes);
            printf("B | PID: %d | A with pid %s has response: %s\n", getpid(), pid_s, readbuf);

            close(fifo_b);
            free(readbuf);
            free(pid_s);

            if(readbuf[0]=='1'){
                foundMate=1;
                shdata->children_a[choice_id].alive = 0;
            }else if(i == shdata->cur_idx -1 ){
                i=0;
            }
        }
    }

    remove(pid_s);

    // TODO ho scelto A
//    if (child.alive) {
//        printf("No type A found\n");
//        printf("\n ---> CHILD B END | pid: %d <---\n", getpid());
//
//        exit(EXIT_FAILURE);
//    }


    if(foundMate==1) {
        printf("B | PID: %d, contacting parent...\n",getpid());
        // send info to gestore
        int key, mask, msgid;
        key = getppid();
        mask = 0666;
        msgid = msgget(key, mask);

        if (msgid == -1) {
            msgid = msgget(key, mask | IPC_CREAT);
            if (msgid == -1) {
                fprintf(stderr, "B | Could not create message queue.\n");
                exit(EXIT_FAILURE);
            }
        }

        // Send messages ...
        int pid_a_int = shdata->children_a[choice_id].pid;
        int ret, msg[2] = {getpid(), pid_a_int};
        ret = msgsnd(msgid, msg, sizeof(int), IPC_NOWAIT);
        if (ret == -1) {
            if (errno != EAGAIN) {
                fprintf(stderr, "B | Message could not be sended.\n");
                exit(EXIT_FAILURE);
            }
            usleep(50000);//50ms
            //try again
            if (msgsnd(msgid, msg, sizeof(int), 0) == -1) {
                fprintf(stderr, "B | Message could not be sended.\n");
                exit(EXIT_FAILURE);
            }
        }
        printf("B | PID: %d, message to the parent sent.\n",getpid());
    } else{
        printf("B | I'm sad, no mate found");
    }

    printf(" ---> CHILD B END | pid: %d <---\n", getpid());

    exit(EXIT_SUCCESS);
}


void init_shmemory() {
    if ((shmid = shmget(key, getpagesize(), SHMFLG)) == 0) {
        perror("B | cannot get shared memory id | shdata\n");
        exit(EXIT_FAILURE);
    }

    if ((shdata = shmat(shmid, NULL, 0)) == (void *) -1) {
        perror("B | cannot attach shared memory to address \n");
        exit(EXIT_FAILURE);
    }

    if ((shmid = shmget(key + 1, getpagesize() * 10, SHMFLG)) == 0) {
        perror("B | cannot get shared memory id | shdata->children_a \n");
        exit(EXIT_FAILURE);
    }

    if ((shdata->children_a = shmat(shmid, NULL, 0)) < 0) {
        perror("B | cannot attach shared memory to address shdata->children_a \n");
        exit(EXIT_FAILURE);
    }
}