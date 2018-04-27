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
    char *buffer;

    printf("\n ---> CHILD A START | pid: %d | argc: %d <---\n", getpid(), argc);
    if (argc < 2) {
        printf("I need name and genoma input from argv. \n");
        for(int i = 0; i<argc;i++){
            printf("arg[%d]: %s \n ",i,argv[i]);
        }
        exit(EXIT_FAILURE);
    }

    my_info.nome = argv[0];
    my_info.tipo = 'A';
    my_info.genoma = (unsigned long) strtol(argv[1], &buffer, 10);

    printf("my_info.nome: %s \n", my_info.nome);
    printf("my_info.tipo: %c \n", my_info.tipo);
    printf("my_info.genoma: %lu \n", my_info.genoma);

    sleep(1);
    printf("\n ---> CHILD A END | pid: %d <---\n", getpid());

    exit(EXIT_SUCCESS);
}
