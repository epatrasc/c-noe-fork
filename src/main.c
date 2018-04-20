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

// Input arguments;
const int INIT_PEOPLE = 5; // number of inital children
const unsigned long GENES = 1000000;
const unsigned int BIRTH_DEATH = 5;   //seconds
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
    struct individuo* individui;
};

unsigned long gen_genoma();

char *gen_name(char* name);

char gen_type();

struct individuo gen_individuo();

void run_parent(pid_t gestore_pid, pid_t pg_pid);

void run_child(int shid, struct individuo figlio);

unsigned int rand_interval(unsigned int min, unsigned int max);

static void wake_up_process(int signo);

static void shm_print_stats(int fd, int m_id);

unsigned int compile_child_code(char type);

// Global variables
int m_id, s_id, num_errors;

int main() {
    int i, cur_i, status;
    struct individuo* figli[INIT_PEOPLE];
    struct individuo figlio;
    struct shared_data* my_data;
    pid_t gestore_pid, pg_pid, child_pid, pid_array[INIT_PEOPLE];

    // randomize
    srand(time(NULL));

    // pid gestore, pid padre gestore
    gestore_pid = getpid();
    pg_pid = getppid();
    run_parent(gestore_pid, pg_pid);

    // compile child code
    compile_child_code('A');
    compile_child_code('B');

    //generate population
    for (i = 0; i < INIT_PEOPLE; i++) {
        printf("child sequence: %d \n", i + 1);

        // generate child
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
        }

        /* Perform actions specific to parent */
        pid_array[i] = child_pid;
    }

    // ** SHARED MEMORY
    // Create a shared memory area and attach it to the pointer
    //init shared data
//    my_data = malloc(sizeof(*my_data));
//    my_data->individui = malloc(sizeof(*figlio));
//    my_data->cur_idx = 0;

    // add children in memory
//    cur_i = my_data->cur_idx;
//    my_data->individui[cur_i] = *figlio;
//    my_data->cur_idx++;

    m_id = shmget(IPC_PRIVATE, sizeof(*my_data), 0666);
    my_data = shmat(m_id, NULL, 0);
    shm_print_stats(2, m_id);

    for (int i = 0; i < my_data->cur_idx; i++) {
        printf("SHM pid gestore: %d | [%d] nome: %s \n", getpid(), i, my_data->individui[i].nome);
    }

    // make sure child set the signal handler
    sleep(1);

    // send signal to wake up all the children
    for (int i = 0; i < INIT_PEOPLE; i++) {
        printf("Sending SIGUSR1 to the child %d...\n", pid_array[i]);
        kill(pid_array[i], SIGUSR1);
    }

    while ((child_pid = wait(&status)) > 0){
        printf("Ended child : %d | status: %d \n", child_pid, status);
    }

    // The shared memory can be marked for deletion.
    // Remember: it will be deleted only when all processes
    // are detached from it
    while (shmctl(m_id, IPC_RMID, NULL)) TEST_ERROR;

    exit(EXIT_SUCCESS);
}

unsigned long gen_genoma() {
    return (unsigned long) rand_interval(2, GENES);
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

char gen_type() {
    srand(time(0));
    return rand() % 2 ? 'A' : 'A';
}

struct individuo gen_individuo() {
    struct individuo figlio;

    figlio.tipo = gen_type();
    figlio.nome = gen_name("");
    figlio.genoma = gen_genoma();

    return figlio;
}

void run_parent(pid_t gestore_pid, pid_t pg_pid) {
    printf("* PARENT: PID=%d, PPID=%d\n", gestore_pid, pg_pid);
}

void run_child(int shid, struct individuo figlio) {
    int error = 0;
    char* envp[] = {NULL};
    char* argv[] = {NULL, NULL};
	char* buffer = malloc(sizeof(shid));

    printf("CHILD -> NAME: %s | TYPE: %c | GENOMA: %lu \n", figlio.nome, figlio.tipo, figlio.genoma);


	// run execve
	sprintf(buffer, "%d", shid);
	argv[0] = buffer;

    error = execv(figlio.tipo == 'A' ? "./exec/child_a.exe" : "./exec/child_b.exe", argv);

	// if here i'm in error
    if (error == -1) {
        fprintf(stderr, "\n%s: %d. Error #%03d: %s\n", __FILE__, __LINE__, errno, strerror(errno));
        die("Errore execve child");
    }
}

/****** UTILS ******/
static void wake_up_process(int signo) {
    if (signo == SIGUSR1) {
        printf("WAKEUP  process %d | received signal SIGUSR1\n", getpid());
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

    return min + (r / buckets);
}

unsigned int compile_child_code(char type){
    int status =0;

    status = system(type == 'A' ? "gcc ./src/child_a.c -o ./exec/child_a.exe" : "gcc ./src/child_b.c -o ./exec/child_b.exe");
    printf("COMPILE TYPE FILE: %c | terminated with exit status %d\n",
           type, status);

    return status;
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
    dprintf(fd, "---------- Number of attached processes: %lu \n",
            my_m_data.shm_nattch);
    dprintf(fd, "----------------------- PID of creator: %d\n",
            my_m_data.shm_cpid);
    dprintf(fd, "----------------------- PID of last shmat shmdt : %d\n",
            my_m_data.shm_lpid);
    dprintf(fd, "--- IPC Shared Memory ID: %8d \n", m_id);
    dprintf(fd, " END -----\n");
}
