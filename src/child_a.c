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

    if (mcd(gen_a, gen_b) >= gen_a / 3) {
        return true;
    }

    return false;
}

int main(int argc, char *argv[]) {
    struct individuo my_info;

    printf("\n ---> CHILD A START | pid: %d | argc: %d <---\n", getpid(), argc);
    if (argc < 3) {
        printf("I need name and genoma input from argv. \n");
        for (int i = 0; i < argc; i++) {
            printf("arg[%d]: %s \n ", i, argv[i]);
        }
        exit(EXIT_FAILURE);
    }

    my_info.nome = argv[1];
    my_info.tipo = 'A';
    my_info.genoma = (unsigned long) strtol(argv[2], NULL, 10);

    printf("my_info.nome: %s \n", my_info.nome);
    printf("my_info.tipo: %c \n", my_info.tipo);
    printf("my_info.genoma: %lu \n", my_info.genoma);

    //create fifo
    char *pid_s = calloc(sizeof(char), 6);
    sprintf(pid_s, "%d", getpid());
    mkfifo(pid_s, S_IRUSR | S_IWUSR);

    int flg_continua = 1, BUF_SIZE = 1024;
    ssize_t num_bytes;
    char *readbuf = calloc(sizeof(char), BUF_SIZE);
    int fifo_a;
    char *pid_b = calloc(sizeof(char), 6);

    while (flg_continua >= 1) {
        printf("waiting for type b request\n");
        fifo_a = open(pid_s, O_RDONLY);
        while (num_bytes = read(fifo_a, readbuf, BUF_SIZE)) {
            if (num_bytes > 0) {
                printf("out: %s\n", readbuf);

                char *nome_b;
                unsigned long genoma_b;
                char *token;
                token = strsep(&readbuf, ",");
                sprintf(pid_b, "%s", token);
                token = strsep(&readbuf, ",");
                nome_b = calloc(sizeof(char), strlen(token));
                sprintf(nome_b, "%s", token);
                genoma_b = (unsigned long) strtol(strsep(&readbuf, ","), NULL, 10);

                printf("pid_b:%s\n", pid_b);
                printf("nome_b:%s\n", nome_b);
                printf("genoma_b:%lu\n", genoma_b);

                //evaluate candidate
                int answer = (int) isGood(my_info.genoma, genoma_b);
                if (answer) flg_continua = 0;//TODO dangerus can end in loop

                printf("accept pid %s? %d\n", pid_b, answer);

                // send response
                int fifo_b = open(pid_b, O_WRONLY);
                write(fifo_b, "%d", answer);
                close(fifo_b);
            }
        }
        close(fifo_a);
    }

    free(readbuf);
    remove(pid_s);

    // send info to gestore
    int key, mask, msgid;

    key = getppid();
    mask = 0666;
    msgid = msgget(key, mask);

    if (msgid == -1) {
        msgid = msgget(key, mask | IPC_CREAT);
        if (msgid == -1) {
            fprintf(stderr, "Could not create message queue.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Send messages ...
    int pid_b_int = (int) strtol(pid_b, (char **) NULL, 10);
    int ret, msg[2] = {getpid(), pid_b_int};
    ret = msgsnd(msgid, msg, sizeof(int), IPC_NOWAIT);
    if (ret == -1) {
        if (errno != EAGAIN) {
            fprintf(stderr, "Message could not be sended.\n");
            exit(EXIT_FAILURE);
        }
        usleep(50000);//5 sec
        //try again
        if (msgsnd(msgid, msg, sizeof(int), 0) == -1) {
            fprintf(stderr, "Message could not be sended.\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("\n ---> CHILD A END | pid: %d <---\n", getpid());

    exit(EXIT_SUCCESS);
}
