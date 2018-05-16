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
#include <sys/file.h>
#include <sys/sem.h>

#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));}

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

bool isForMe(unsigned long gen_a, unsigned long gen_b) {
    if (gen_a % gen_b == 0) {
        return true;
    }

    if (mcd(gen_a, gen_b) >= 1) {
        return true;
    }

    return false;
}

void init_shmemory();

void send_msg_to_gestore(int pid_a);

const int SHMFLG = IPC_CREAT | 0666;
const key_t key = 1060;
unsigned int shmid;
struct shared_data *shdata;

int main(int argc, char *argv[]) {
    char *my_val, nome;
    struct individuo my_info;
    struct sembuf sem_lock={0, -1, 0};
    struct sembuf sem_unlock={0, 1, 0};

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

    int foundMate = 0;

    char *pid_s = calloc(sizeof(char), 6);
    char *pida_s = calloc(sizeof(char), 6);
    int pid_a_winner;
    sprintf(pid_s, "%d", getpid());
    mkfifo(pid_s, S_IRUSR | S_IWUSR);
    TEST_ERROR;
    for (int i = 0; i < shdata->cur_idx && !foundMate; i++) {
        struct child_a child = shdata->children_a[i];
        printf("B | pid: %d | shdata[%d]: genoma: %lu | child_a pid: %d | alive: %d \n", getpid(), i, child.genoma, child.pid, child.alive);

        // skip death child
        if (shdata->children_a[i].alive == 0) {
            printf("B | shdata->children_a[%d] NOT alive \n", i);
            continue;
        }

        //evaluate candidate
        int answer = (int) isForMe(child.genoma, my_info.genoma);
        printf("B | GEN_A: %lu, GEN_B: %lu are compatible? %d\n",child.genoma, my_info.genoma, answer);
        if (answer == 0) continue;

        // access to the child semaphore
        int pida = child.pid;
        pid_t sem_id = semget(pida, 1 ,IPC_CREAT | 0666);

        // print semaphore status
        int sem_val = semctl (sem_id, 0, GETVAL);
        printf("B | pid %d | sem_id: %d | sem_val: %d\n", getpid(), sem_id, sem_val);

        // lock resource
        semop(sem_id,&sem_lock,1);
        TEST_ERROR;
        printf("B1 | pid %d | took control of %d semaphore\n", getpid(),sem_id);
        sleep(2);
        printf("B2 | pid %d | took control of %d semaphore\n", getpid(),sem_id);
        if(child.alive == 0){
            printf("B | shdata->children_a[%d] NOT alive \n", i);
            // Release the resource
            semop(sem_id, &sem_unlock, 1);
            TEST_ERROR;
            continue;
        }

        // Write message to A
        sprintf(pida_s, "%d", child.pid);
        TEST_ERROR;
        printf("B | pid: %d, shdata->children_a[i]: %s\n", getpid(), pida_s);
        printf("B | before open | shdata->children_a[i].alive: %d\n", child.alive);

        int fifo_a = open(pida_s, O_WRONLY);
        TEST_ERROR;

        if (fifo_a == -1) {
            printf("B | error fifo_a returned: %d\n", fifo_a);
        }

        printf("B | PID: %d | writing to A: start\n", getpid());
        char *my_msg = calloc(sizeof(char), 1024);
        int str_len = sprintf(my_msg, "%d,%s,%lu", getpid(), my_info.nome, my_info.genoma);
        write(fifo_a, my_msg, str_len);
        free(my_msg);
        TEST_ERROR;
        close(fifo_a);
        TEST_ERROR;
        printf("B | PID: %d | writing to A: end\n", getpid());

        // read answer
        printf("B | PID: %d | reading response from A..\n", getpid());
        int BUF_SIZE = 1024;
        char *readbuf = calloc(sizeof(char), BUF_SIZE);
        int fifo_b = open(pid_s, O_RDONLY);
        ssize_t num_bytes = read(fifo_b, readbuf, BUF_SIZE);

        printf("B | num_bytes: %li\n", num_bytes);
        printf("B | PID: %d | A with pid %s has response: %s\n", getpid(), pida_s, readbuf);

        close(fifo_b);
        free(readbuf);
        free(pid_s);
        TEST_ERROR;

        printf("B | readbuf[0] == '1': %li\n", strtol(readbuf, NULL, 10));
        int result;
        if ((result = strtol(readbuf, NULL, 10)) == 1) {
            printf("B | pid %d | foundMate \n", getpid());
            foundMate = 1;
            pid_a_winner = child.pid;
            child.alive = 0;
            TEST_ERROR;
        }

        // Release the resource
        semop(sem_id, &sem_unlock, 1);
        TEST_ERROR;
    }

    if (foundMate == 0) {
        printf("B | I'm sad, no mate found. Now I will die\n");
        free(pida_s);
        remove(pid_s);
        exit(EXIT_FAILURE);
    }

    send_msg_to_gestore(pid_a_winner);
    TEST_ERROR;
    printf(" ---> CHILD B END | pid: %d <---\n", getpid());

    free(pida_s);
    remove(pid_s);

    exit(EXIT_SUCCESS);
}

void send_msg_to_gestore(int pid_a) {
    printf("B | PID: %d, contacting parent..\n", getpid());
    // send info to gestore
    int key = getppid();
    int msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1) {
        msgid = msgget(key, 0666 | IPC_CREAT);
        if (msgid == -1) {
            fprintf(stderr, "B | Could not create message queue.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Send messages ..
    int ret, msg[2] = {getpid(), pid_a};
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
    printf("B | PID: %d, message to the parent sent.\n", getpid());
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