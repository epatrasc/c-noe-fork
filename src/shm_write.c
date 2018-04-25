#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdlib.h>

struct individuo
{
    char tipo;
    char *nome;
    unsigned long genoma;
};

struct shared_data
{
    unsigned long cur_idx;
    struct individuo *individui;
};

int main ()
{
    key_t shm_key = 6166529;
    const int shm_size = 1024;

    int shm_id;
    struct individuo figlio;
    char* nome;
    struct individuo* shmaddr, *ptr;
    int x;

    figlio.nome = "mandy";
    figlio.tipo = 'A';
    figlio.genoma = 333333;

    printf ("writer started.\n");

    /* Allocate a shared memory segment. */
    shm_id = shmget (shm_key, shm_size, IPC_CREAT | S_IRUSR | S_IWUSR);

    /* Attach the shared memory segment. */
    shmaddr = (char*) shmat (shm_id, 0, 0);

    printf ("shared memory attached at address %p\n", shmaddr);

    /* Start to write data. */
    ptr = shmaddr + sizeof (figlio);
    ptr += sizeof(figlio.nome);
    ptr->tipo = figlio.tipo;
    ptr->genoma = figlio.genoma;
    ptr->nome = figlio.nome;
    memcpy(shmaddr, &ptr, sizeof (ptr));
    printf ("writer ended.\n");

    /*calling the other process*/
    system("./exec/shn_read.exe");

    /* Detach the shared memory segment. */
    shmdt (shmaddr);
    /* Deallocate the shared memory segment.*/
    shmctl (shm_id, IPC_RMID, 0);

    return 0;
}