#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define die(msg_error)      \
    do                      \
    {                       \
        fprintf(stderr, "\n%s: %d. Error #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));\
    } while (0);
#define TEST_ERROR                                 \
    if (errno)                                     \
    {                                              \
        dprintf(STDERR_FILENO,                     \
                "%s:%d: PID=%5d: Error %d (%s)\n", \
                __FILE__,                          \
                __LINE__,                          \
                getpid(),                          \
                errno,                             \
                strerror(errno));                  \
    }

// Input arguments;
const int INIT_PEOPLE = 5; // number of inital children
const unsigned long GENES = 1000000;
const unsigned int BIRTH_DEATH = 5;   //seconds
const unsigned int SIM_TIME = 1 * 60; //seconds

const int MAX_POPOLATION = 20;
const int MIN_CHAR = 65; // A
const int MAX_CHAR = 90; // Z
const int SHMFLG = IPC_CREAT | 0666;

struct individuo
{
    char tipo;
    char *nome;
    unsigned long genoma;
};

struct child_a
{
    unsigned long genoma;
    pid_t pid;
};

struct shared_data
{
    unsigned long cur_idx;
    struct child_a *children_a;
};

unsigned long gen_genoma();

char *gen_name(char *name);

char gen_type();

struct individuo gen_individuo();

void run_parent(pid_t gestore_pid, pid_t pg_pid);

void run_child(struct individuo figlio);

unsigned int rand_interval(unsigned int min, unsigned int max);

int len_of(int x);

static void wake_up_process(int signo);

static void shm_print_stats(int shmid);

unsigned int compile_child_code(char type);

void publish_shared_data(struct individuo figlio);

// Global variables
int shmid, s_id, num_errors;
struct shared_data *shdata;
key_t key = 1060;

int main()
{
    int i, cur_i, status;
    struct individuo figli[INIT_PEOPLE];
    struct individuo figlio;
    pid_t gestore_pid, pg_pid, child_pid, pid_array[INIT_PEOPLE];
    size_t psize = getpagesize();

    // randomize
    srand(time(NULL));

    // pid gestore, pid padre gestore
    gestore_pid = getpid();
    pg_pid = getppid();
    run_parent(getpid(), pg_pid);

    // compile child code
    compile_child_code('A');
    compile_child_code('B');

    //Create shm segment
    if ((shmid = shmget(key, psize, SHMFLG)) < 0)
    {
        die("parent shmget");
    }

    //Attach segment to data space
    if ((shdata = shmat(shmid, NULL, 0)) == (void *)-1)
    {
        die("parent shmat");
    }

    //generate population
    for (i = 0; i < INIT_PEOPLE; i++)
    {
        printf("child sequence: %d \n", i + 1);

        // generate child
        figlio = gen_individuo();
        child_pid = fork();

        /* Handle error create child*/
        if (child_pid < 0)
        {
            die(strerror(errno));
        }

        /* Perform actions specific to child */
        if (child_pid == 0)
        {
            signal(SIGUSR1, wake_up_process);
            pause();
            run_child(figlio);
            exit(EXIT_FAILURE);
        }

        /* Perform actions specific to parent */
        pid_array[i] = child_pid;
        figli[i] = figlio;
        publish_shared_data(figlio);
    }

    shm_print_stats(shmid);
    for (int i = 0; i < shdata->cur_idx; i++)
    {
        struct individuo figlio = shdata->individui[i];
        printf("GESTORE pid: %d | [%d] nome: %s \n", getpid(), i, figlio.nome);
    }

    // make sure child set the signal handler
    sleep(1);
    if(1 == 1){
        exit(EXIT_FAILURE);
    }
    //exit(EXIT_FAILURE);
    // send signal to wake up all the children
    for (int i = 0; i < INIT_PEOPLE; i++)
    {
        printf("Sending SIGUSR1 to the child %d...\n", pid_array[i]);
        kill(pid_array[i], SIGUSR1);
    }

    while ((child_pid = wait(&status)) > 0)
    {
        printf("Ended child : %d | status: %d \n", child_pid, status);
    }

    // The shared memory can be marked for deletion.
    // Remember: it will be deleted only when all processes
    // are detached from it
    // while (shmctl(shmid, IPC_RMID, NULL)) TEST_ERROR;
    exit(EXIT_SUCCESS);
}

unsigned long gen_genoma()
{
    return (unsigned long)rand_interval(2, GENES);
}

char *gen_name(char *name)
{
    int new_length = (int)(strlen(name) + 2);
    char *new_name = calloc(sizeof(char), new_length);
    char *buffer = calloc(sizeof(char), 2);

    sprintf(buffer, "%c", (char)rand_interval(MIN_CHAR, MAX_CHAR));
    strcpy(new_name, name);
    strcat(new_name, buffer);

    free(buffer);

    return new_name;
}

char gen_type()
{
    srand(time(0));
    return rand() % 2 ? 'A' : 'A';
}

struct individuo gen_individuo()
{
    struct individuo figlio;

    figlio.tipo = gen_type();
    figlio.nome = gen_name("");
    figlio.genoma = gen_genoma();

    return figlio;
}

void run_parent(pid_t gestore_pid, pid_t pg_pid)
{
    printf("* PARENT: PID=%d, PPID=%d\n", gestore_pid, pg_pid);
}

void run_child(struct individuo figlio)
{
    int error = 0;
    char *envp[] = {NULL};
    char *buffer = calloc(sizeof(char), len_of(shmid));
    char *argv[] = {calloc(sizeof(char), len_of(shmid) * sizeof(char)), NULL};

    printf("CHILD -> NAME: %s | TYPE: %c | GENOMA: %lu \n", figlio.nome, figlio.tipo, figlio.genoma);

    // run execve
    sprintf(buffer, "%d", shmid);
    strcat(argv[0], buffer);
    free(buffer);

    error = execv(figlio.tipo == 'A' ? "./exec/child_a.exe" : "./exec/child_b.exe", argv);

    // if here i'm in error
    if (error < 0)
    {
        free(argv[0]);
        die("Errore execve child");
    }
}

// ** SHARED MEMORY
void publish_shared_data(struct individuo figlio)
{   
    if(figlio.tipo == 'B') return;

    if(shdata->individui == NULL){
        shdata->cur_idx = 0;
        shmid = shmget(++key, (INIT_PEOPLE * sizeof(shdata->individui)), 0666 | IPC_CREAT);
        shdata->individui = shmat(shmid,NULL, 0);

        if(shdata->individui == NULL){
            die("error malloc shdata->individui");
        }
    }

    char *nome;
    shdata->individui[shdata->cur_idx].tipo = figlio.tipo;
    shdata->individui[shdata->cur_idx].genoma = figlio.genoma;

    int offset = strcpy(nome, figlio.nome);
    memcpy(shdata->individui[shdata->cur_idx].nome, &offset, sizeof(offset));



    shdata->cur_idx++;
}

/****** UTILS ******/
static void wake_up_process(int signo)
{
    if (signo == SIGUSR1)
    {
        printf("WAKEUP  process %d | received signal SIGUSR1\n", getpid());
    }
}

unsigned int rand_interval(unsigned int min, unsigned int max)
{
    // reference https://stackoverflow.com/a/17554531
    int r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do
    {
        r = lrand48();
    } while (r >= limit);

    return min + (r / buckets);
}

unsigned int compile_child_code(char type)
{
    int status = 0;

    status = system(type == 'A' ? "gcc ./src/child_a.c -o ./exec/child_a.exe" : "gcc ./src/child_b.c -o ./exec/child_b.exe");
    printf("COMPILE TYPE FILE: %c | terminated with exit status %d\n",
           type, status);

    return status;
}

static void shm_print_stats(int shmid)
{
    struct shmid_ds my_m_data;

    while (shmctl(shmid, IPC_STAT, &my_m_data))
    {
        TEST_ERROR;
    }

    printf("\n--- IPC Shared Memory ID: %8d, START ---\n", shmid);
    printf("---------------------- Memory size: %ld\n",
           my_m_data.shm_segsz);
    printf("---------------------- Time of last attach: %ld\n",
           my_m_data.shm_atime);
    printf("---------------------- Time of last detach: %ld\n",
           my_m_data.shm_dtime);
    printf("---------------------- Time of last change: %ld\n",
           my_m_data.shm_ctime);
    printf("---------- Number of attached processes: %lu \n",
           my_m_data.shm_nattch);
    printf("----------------------- PID of creator: %d\n",
           my_m_data.shm_cpid);
    printf("----------------------- PID of last shmat shmdt : %d\n",
           my_m_data.shm_lpid);
    printf("--- IPC Shared Memory ID: %8d \n", shmid);
    printf("----- END -----\n");
}

int len_of(int value)
{
    int l = 1;

    while (value > 9)
    {
        l++;
        value /= 10;
    }

    return l;
}
