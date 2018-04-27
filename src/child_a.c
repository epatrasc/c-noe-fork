#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

struct individuo {
    char tipo;
    char *nome;
    unsigned long genoma;
};


int main(int argc, char *argv[]) {
    struct individuo my_info;

    printf("\n ---> CHILD A START | pid: %d <---\n", getpid());
    if (argc > 1) {
        char *buffer;

        my_info.nome = argv[0];
        my_info.tipo = 'A';
        my_info.genoma = (unsigned long) strtol(argv[2], &buffer, 10);

        printf("my_info.nome: %s \n", my_info.nome);
        printf("my_info.tipo: %c \n", my_info.tipo);
        printf("my_info.genoma: %lu \n", my_info.genoma);
    }

    sleep(1);
    printf("\n ---> CHILD A END | pid: %d <---\n", getpid());

    exit(EXIT_SUCCESS);
}
