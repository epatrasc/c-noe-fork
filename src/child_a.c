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

int main(int argc, char *argv[]) {
    struct individuo my_info;

    printf("\n ---> CHILD A START | pid: %d | argc: %d <---\n", getpid(), argc);
    if (argc < 3) {
        printf("A | I need name and genoma input from argv. \n");
        for (int i = 0; i < argc; i++) {
            printf("A | arg[%d]: %s \n ", i, argv[i]);
        }
        exit(EXIT_FAILURE);
    }
    //creat sem 
    pid_t sem_id = semget(getpid(), 1, IPC_CREAT | 0666);
    printf("A | pid %d | sem_id: %d\n", getpid(),sem_id);
    semctl(sem_id, 0, SETVAL, 1);
    TEST_ERROR;


    my_info.nome = argv[1];
    my_info.tipo = 'A';
    my_info.genoma = (unsigned long) strtol(argv[2], NULL, 10);

    printf("A | my_info.nome: %s \n", my_info.nome);
    printf("A | my_info.tipo: %c \n", my_info.tipo);
    printf("A | my_info.genoma: %lu \n", my_info.genoma);

    //create fifo
    char *pid_s = calloc(sizeof(char), 6);
    sprintf(pid_s, "%d", getpid());
    mkfifo(pid_s, S_IRUSR | S_IWUSR);

    int flg_continua = 1, BUF_SIZE = 1024;
    ssize_t num_bytes;
    char *readbuf = calloc(sizeof(char), BUF_SIZE);
    int fifo_a;
    char *pid_b = calloc(sizeof(char), 6);

    bool continua = true;
    while (flg_continua <= 3 && continua) {
        printf("A | waiting for type b request...\n");
        TEST_ERROR;

        fifo_a = open(pid_s, O_RDONLY);

        if ((num_bytes = read(fifo_a, readbuf, BUF_SIZE))> 0) {
                printf("A | recieved request: %s\n", readbuf);
                close(fifo_a);
                char *nome_b;
                unsigned long genoma_b;
                char *token;

                token = strsep(&readbuf, ",");
                sprintf(pid_b, "%s", token);
                token = strsep(&readbuf, ",");
                nome_b = calloc(sizeof(char), strlen(token));
                sprintf(nome_b, "%s", token);
                genoma_b = (unsigned long) strtol(strsep(&readbuf, ","), NULL, 10);

                printf("A | pid_b:%s\n", pid_b);
                printf("A | nome_b:%s\n", nome_b);
                printf("A | genoma_b:%lu\n", genoma_b);

                //evaluate candidate
                int answer = (int) isGood(my_info.genoma, genoma_b);
                if (answer || flg_continua==2) {
                    //TODO dangerous can end in loop
                    continua = false;
                    answer=1;
                }


                printf("A | accept pid %s? %d\n", pid_b, answer);
                sleep(1);
                // send response
                int fifo_b = open(pid_b, O_WRONLY);
                if(fifo_a < 0){
                    sleep(1);
                    fifo_b = open(pid_b, O_WRONLY);
                }
                char *my_msg = calloc(sizeof(char), 2);
                int str_len = sprintf(my_msg, "%d",answer);
                printf("A | fifo_b: %d\n", fifo_b);
                write(fifo_b, my_msg, str_len);
                close(fifo_b);
        }else{
            close(fifo_a);
        }
        
        flg_continua ++;
    }

    free(readbuf);
    remove(pid_s);

    printf("A | PID: %d, contacting parent...\n",getpid());

    // send info to gestore
    int key, mask, msgid;

    key = getppid();
    mask = 0666;
    msgid = msgget(key, mask);

    if (msgid == -1) {
        msgid = msgget(key, mask | IPC_CREAT);
        if (msgid == -1) {
            fprintf(stderr, "A | Could not create message queue.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Send messages ...
    int pid_b_int = (int) strtol(pid_b, (char **) NULL, 10);
    int ret, msg[2] = {getpid(), pid_b_int};
    ret = msgsnd(msgid, msg, sizeof(int), IPC_NOWAIT);

    printf("A | PID: %d, Send message ...\n",getpid());

    if (ret == -1) {
        if (errno != EAGAIN) {
            fprintf(stderr, "A |Message could not be sended.\n");
            exit(EXIT_FAILURE);
        }
        usleep(50000);//50 ms
        //try again
        if (msgsnd(msgid, msg, sizeof(int), 0) == -1) {
            fprintf(stderr, "A | Message could not be sended.\n");
            exit(EXIT_FAILURE);
        }
    }

    //semctl(sem_id, 0, IPC_RMID,0);


    printf("A | PID: %d, Message sent \n",getpid());
    printf(" ---> CHILD A END | pid: %d <---\n", getpid());

    exit(EXIT_SUCCESS);
}
