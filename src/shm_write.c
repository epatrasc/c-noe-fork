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
    char* nome;
    struct individuo figlio;
    struct individuo *figli;
    struct shared_data* shmaddr, *ptr;
    int offset;

    figlio.nome = "mandy";
    figlio.tipo = 'A';
    figlio.genoma = 333333;
    figli = malloc(sizeof(figlio));
    figli[0] = figlio;

    printf ("writer started.\n");

    /* Allocate a shared memory segment. */
    shm_id = shmget (shm_key, shm_size, IPC_CREAT | S_IRUSR | S_IWUSR);

    /* Attach the shared memory segment. */
    shmaddr = shmat (shm_id, 0, 0);

    printf ("shared memory attached at address %p\n", shmaddr);

    /* Start to write data. */
    ptr = shmaddr + sizeof (figli);
    ptr->cur_idx = 1;
    ptr->individui = figli;
    shmaddr->cur_idx = ptr->cur_idx;
    //memcpy(shmaddr->individui, &ptr->individui, sizeof (ptr->individui));
    printf ("writer ended.\n");

    /*calling the other process*/
    system("./exec/shn_read.exe");

    /* Detach the shared memory segment. */
   // shmdt (shmaddr);
    /* Deallocate the shared memory segment.*/
    //shmctl (shm_id, IPC_RMID, 0);

    return 0;
}