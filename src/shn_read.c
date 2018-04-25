#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>

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
    struct individuo *figli;
    struct shared_data* shmaddr, *ptr;

  /* Allocate a shared memory segment. */
  shm_id = shmget (shm_key, shm_size, S_IRUSR | S_IWUSR);

  /* Attach the shared memory segment. */
  shmaddr = shmat (shm_id, 0, 0);

  printf ("shared memory attached at address %p\n", shmaddr);

  /* Start to read data. */
    ptr = shmaddr ;
    figli = ptr->individui;
    figlio = figli[0];
    printf ("1=%c\n", figlio.tipo);
    printf ("2=%lu\n", figlio.genoma);
    printf ("0=%s\n", figlio.nome);

//  shared_memory[0] = ptr;
//  ptr += *p++;
//  shared_memory[1] = ptr;
//  ptr += *p;
//  shared_memory[2] = ptr;
//  printf ("0=%s\n", shared_memory[0]);
//  printf ("1=%s\n", shared_memory[1]);
//  printf ("2=%s\n", shared_memory[2]);

  /* Detach the shared memory segment. */
  shmdt (shmaddr);
  return 0;
}