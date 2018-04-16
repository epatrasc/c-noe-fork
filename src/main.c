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

#define die(e)                      \
    do                              \
    {                               \
        fprintf(stderr, "%s\n", e); \
        exit(EXIT_FAILURE);         \
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
#define NUM_PROC 5
// Input arguments;
const int INIT_PEOPLE = 5; // number of inital children
const unsigned long GENES = 1000000;
const unsigned int BIRTH_DEATH = 1;   //seconds
const unsigned int SIM_TIME = 1 * 60; //seconds

const int MAX_POPOLATION = 20;
const int MIN_CHAR = 65; // A
const int MAX_CHAR = 90; // Z

struct individuo {
    char tipo;
    char *nome;
    unsigned long genoma;
};

struct shared_data {
    unsigned long cur_idx;
    struct individuo individui[NUM_PROC];
};

unsigned long gen_genoma();

char *gen_name();

char gen_type();

struct individuo *gen_individuo();

void run_parent(pid_t gestore_pid, pid_t pg_pid);

void run_child(int shid, struct individuo *figlio);

unsigned int rand_interval(unsigned int min, unsigned int max);

static void wake_up_process(int signo);

static void shm_print_stats(int fd, int m_id);

// Global variables
int m_id, s_id, num_errors;

int main() {
    int i, cur_i, status;
    pid_t gestore_pid, pg_pid, child_pid;
    struct individuo *figlio;
    struct shared_data *my_data;
    int population = INIT_PEOPLE;
    pid_t pid_array[population];

    // randomize
    srand(time(NULL));

    // check popolation limit
    if (population > 20) {
        printf("Warning: Popolazione iniziale maggiore di 20, il massimo consentito.");
        exit(EXIT_FAILURE);
    }

    // pid gestore, pid padre gestore
    gestore_pid = getpid();
    pg_pid = getppid();
    my_data = malloc(sizeof(*my_data));
    my_data->cur_idx = 0;

    run_parent(gestore_pid, pg_pid);

    // ** SHARED MEMORY
    // Create a shared memory area and attach it to the pointer
    m_id = shmget(IPC_PRIVATE, sizeof(*my_data), 0600);
    my_data = shmat(m_id, NULL, 0);

    //generate population
    for (i = 0; i < INIT_PEOPLE; i++) {
        printf("\nchild number: %d", i + 1);

        // generate child
        fflush(stdout);
        figlio = gen_individuo();
        child_pid = fork();

        /* Handle error create child*/
        if (child_pid < 0) {
            die(strerror(errno));
        }

        /* Perform actions specific to child */
        if (child_pid == 0) {
            signal(SIGUSR1, wake_up_process);
            pause();
            run_child(m_id, figlio);
            exit(EXIT_SUCCESS);
        }

        /* Perform actions specific to parent */
        pid_array[i] = child_pid;

        // add children in memory
        cur_i = my_data->cur_idx;
        my_data->individui[cur_i] = *figlio;
        my_data->cur_idx++;

        printf("\nRANDOM FOR: %d", rand());

    }

    shm_print_stats(2, m_id);
    // make sure child set the signal handler
    sleep(1);

    // send signal to wake up all the children
    for (int i = 0; i < population; i++) {
        printf("sending the signal SIGUSR1 to the child %d...\n", pid_array[i]);
        kill(pid_array[i], SIGUSR1);
    }
    sleep(1);

    while ((wait(&status)) > 0);
    printf("end wait(&status) ...");

    //the shared memory can be marked for deletion.
    // Remember: it will be deleted only when all processes
    // are detached from it
    while (shmctl(m_id, IPC_RMID, NULL)) {
        TEST_ERROR;
    }

    exit(EXIT_SUCCESS);
}

unsigned long gen_genoma() {
    return (unsigned long) rand_interval(2, GENES);
}

char *gen_name() {
    char *name;
    char ran_c;

    name = calloc(sizeof(char), 2);
    ran_c  = (char)rand_interval(MIN_CHAR, MAX_CHAR);
    *name = ran_c;
    printf("gen_name ran_c: %c\n", ran_c);
    printf("gen_name name: %s\n", name);

    return name;
}

char gen_type() {
    srand(time(0));
    return rand() % 2 ? 'A' : 'B';
}

struct individuo *gen_individuo() {
    struct individuo *figlio;

    figlio = malloc(sizeof(*figlio));

    figlio->tipo = gen_type();
    figlio->nome = gen_name();
    figlio->genoma = gen_genoma();

    return figlio;
}

void run_parent(pid_t gestore_pid, pid_t pg_pid) {
    printf("* PARENT: PID=%d, PPID=%d\n", gestore_pid, pg_pid);
}

void run_child(int shid, struct individuo *figlio) {
    char *arg[] = {"1", NULL};
    struct shared_data *my_data;
    int error = 0;

    my_data = shmat(shid, NULL, 0);

    fflush(stdout);
    printf("Hi, I'm the child with the name %s \n", figlio->nome);
    printf("My type is:  %c\n", figlio->tipo);
    printf("My genoma is:  %lu \n", figlio->genoma);

    // print shared memory
    printf("Shared Memory\n");
    printf("last index: %lu \n", my_data->cur_idx);
    for (int i = 0; i < my_data->cur_idx; i++) {
        printf("[%d] nome: %s \n", i, my_data->individui[i].nome);
    }

    error = execve(figlio->tipo == 'A' ? "./exec/child_a.exe" : "./exec/child_b.exe", arg, arg);
    if (error == -1) {
        die(strcat("Errore execve child ", figlio->nome));
    }
}

/****** UTILS ******/
static void wake_up_process(int signo) {
    printf("wake_up_process signo: %d\n", signo);
    if (signo == SIGUSR1) {
        printf("process %d as received SIGUSR1\n", getpid());
    }
}

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

    printf("\n\n #random : %d\n ",  min + (r / buckets));
    return min + (r / buckets);
}

static void shm_print_stats(int fd, int m_id) {
    struct shmid_ds my_m_data;

    while (shmctl(m_id, IPC_STAT, &my_m_data)) {
        TEST_ERROR;
    }

    dprintf(fd, "\n--- IPC Shared Memory ID: %8d, \nSTART ---\n", m_id);
    dprintf(fd, "---------------------- Memory size: %ld\n",
            my_m_data.shm_segsz);
    dprintf(fd, "---------------------- Time of last attach: %ld\n",
            my_m_data.shm_atime);
    dprintf(fd, "---------------------- Time of last detach: %ld\n",
            my_m_data.shm_dtime);
    dprintf(fd, "---------------------- Time of last change: %ld\n",
            my_m_data.shm_ctime);
    dprintf(fd, "---------- Number of attached processes: %ld\n",
            my_m_data.shm_nattch);
    dprintf(fd, "----------------------- PID of creator: %d\n",
            my_m_data.shm_cpid);
    dprintf(fd, "----------------------- PID of last shmat shmdt : %d\n",
            my_m_data.shm_lpid);
    dprintf(fd, "--- IPC Shared Memory ID: %8d \n", m_id);
    dprintf(fd, " END -----\n");
}
