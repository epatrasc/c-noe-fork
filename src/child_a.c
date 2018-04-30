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
};

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
    while (flg_continua) {
        printf("waiting for type b request\n");
        int fifo_a = open(pid_s, O_RDONLY);
        while (num_bytes = read(fifo_a, readbuf, BUF_SIZE)) {
            if (num_bytes > 0) {
                printf("out: %s\n", readbuf);
                char *pid_b = calloc(sizeof(char), 6);
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

                int fifo_b = open(pid_b, O_WRONLY);
                //TODO implementare logica accetta o no
                printf("accept pid %s", pid_b);
                write(fifo_b, "1", 1);
                close(fifo_b);
            }
        }
        flg_continua--;
        close(fifo_a);
    }

    free(readbuf);
    remove(pid_s);

    printf("\n ---> CHILD A END | pid: %d <---\n", getpid());

    exit(EXIT_SUCCESS);
}
