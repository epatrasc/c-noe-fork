#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

const int MIN_CHAR = 65; // A
const int MAX_CHAR = 90; // Z

unsigned int rand_interval(unsigned int min, unsigned int max) {
    // reference https://stackoverflow.com/a/17554531
    int r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do {
        r = lrand48();
    } while (r >= limit);

    return min + (r / buckets);
}

int len_of(unsigned long value)
{
    int l = 1;

    while (value > 9)
    {
        l++;
        value /= 10;
    }

    return l;
}

char* gen_name(char* name) {
    int new_length = (int) (strlen(name) + 2);
    char* new_name = (char*) calloc(sizeof(char), new_length);
    char* buffer = calloc(sizeof(char),2);

    memset(new_name, '\0', new_length);

    sprintf(buffer, "%c",(char)rand_interval(MIN_CHAR, MAX_CHAR));
    strcat(new_name, name);
    strcat(new_name, buffer);

    return new_name;
}

struct individuo
{
    char tipo;
    char *nome;
    unsigned long genoma;
    pid_t pid;
    bool alive;
};

int main()
{
    int error = 0;
    struct individuo figlio;
    char *argv[] = {NULL, NULL, NULL, NULL};

    figlio.nome="ABCFD";
    figlio.tipo='A';
    figlio.genoma=20202;

    printf("CHILD -> NAME: %s | TYPE: %c | GENOMA: %lu \n", figlio.nome, figlio.tipo, figlio.genoma);

    // run execve
    char *buffer;

    argv[0] = calloc(strlen(figlio.nome) + 1, sizeof(char));
    strcat(argv[0], figlio.nome);

    argv[1] = calloc(2, sizeof(char));
    buffer = calloc(2,sizeof(char));
    sprintf(buffer,"%c", figlio.tipo);
    strcat(argv[1], buffer);
    free(buffer);

    buffer = calloc(len_of(figlio.genoma),sizeof(char));
    argv[2] = calloc(len_of(figlio.genoma) + 1, sizeof(char));
    sprintf(buffer, "%lu", figlio.genoma);
    strcat(argv[2], buffer);
    free(buffer);

    error = execv(figlio.tipo == 'A' ? "./exec/child_a.exe" : "./exec/child_b.exe", argv);

    // if here i'm in error
    if (error < 0)
    {
        free(argv[0]);
        free(argv[1]);
        free(argv[2]);
        free(argv);
        exit(EXIT_FAILURE);
    }
}