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
#include <stdbool.h>

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

struct individuo {
    char tipo;
    char *nome;
    unsigned long genoma;
    pid_t pid;
    bool alive;
};

struct child_a {
    unsigned long genoma;
    pid_t pid;
    bool alive;
};

struct shared_data {
    unsigned long cur_idx;
    struct child_a *children_a;
};

struct stats {
    int num_type_a;
    int num_type_b;
    int cnt_total_pop;
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

unsigned int compile_child_code(char type);

void init_shmemory();

void free_shmemory();

void publish_shared_data(struct individuo figlio);

// Global variables
int shmid[2];
struct shared_data *shdata;
key_t key = 1060;

int main() {
    struct individuo figli[INIT_PEOPLE];
    struct individuo figlio;
    pid_t child_pid, pid_array[INIT_PEOPLE];

    //init steps
    srand(time(NULL));
    init_shmemory();
    compile_child_code('A');
    compile_child_code('B');

    run_parent(getpid(), getppid());

    //init population
    for (int i = 0; i < INIT_PEOPLE; i++) {
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
            run_child(figlio);
            exit(EXIT_FAILURE);
        }

        /* Perform actions specific to parent */
        pid_array[i] = child_pid;
        figlio.pid = child_pid;
        figli[i] = figlio;
        publish_shared_data(figlio);
    }

    for (int i = 0; i < shdata->cur_idx; i++) {
        struct child_a child = shdata->children_a[i];
        printf("GESTORE pid: %d | [%d] child_a pid: %d \n", getpid(), i, child.pid);
    }

    // make sure child set the signal handler
    sleep(1);

    // send signal to wake up all the children
    for (int i = 0; i < INIT_PEOPLE; i++) {
        printf("Sending SIGUSR1 to the child %d...\n", pid_array[i]);
        kill(pid_array[i], SIGUSR1);
    }

    int status;
    while ((child_pid = wait(&status)) > 0) {
        printf("Ended child : %d | status: %d \n", child_pid, status);
    }

    free_shmemory();

    exit(EXIT_SUCCESS);
}

unsigned long gen_genoma() {
    return (unsigned long) rand_interval(2, GENES);
}

char *gen_name(char *name) {
    int new_length = (int) (strlen(name) + 2);
    char *new_name = calloc(sizeof(char), new_length);
    char *buffer = calloc(sizeof(char), 2);

    sprintf(buffer, "%c", (char) rand_interval(MIN_CHAR, MAX_CHAR));
    strcpy(new_name, name);
    strcat(new_name, buffer);

    free(buffer);

    return new_name;
}

char gen_type() {
    srand(time(0));
    return rand_interval(1,10) %2 == 0 ? 'A' : 'B';
}

struct individuo gen_individuo() {
    struct individuo figlio;

    figlio.tipo = gen_type();
    figlio.nome = gen_name("");
    figlio.genoma = gen_genoma();
    figlio.alive = true;

    return figlio;
}

void run_parent(pid_t gestore_pid, pid_t pg_pid) {
    printf("* PARENT: PID=%d, PPID=%d\n", gestore_pid, pg_pid);
}

void run_child(struct individuo figlio) {
    int error = 0;
    char *buffer;
    char *argv[] = {NULL, NULL, NULL};

    printf("CHILD -> NAME: %s | TYPE: %c | GENOMA: %lu \n", figlio.nome, figlio.tipo, figlio.genoma);

    // run execve
    argv[0] = calloc(strlen(figlio.nome) + 1, sizeof(char));
    strcat(argv[0], figlio.nome);

    buffer = calloc(len_of(figlio.genoma), sizeof(char));
    argv[1] = calloc(len_of(figlio.genoma) + 1, sizeof(char));
    sprintf(buffer, "%lu", figlio.genoma);
    strcat(argv[1], buffer);
    free(buffer);

    error = execv(figlio.tipo == 'A' ? "./exec/child_a.exe" : "./exec/child_b.exe", argv);

    // if here i'm in error
    if (error < 0) {
        free(argv[0]);
        free(argv[1]);
        die("Errore execve child");
    }
}

// ** SHARED MEMORY
void init_shmemory() {
    if ((shmid[0] = shmget(key, sizeof(shdata), SHMFLG)) == 0) die("parent shmget");
    if ((shdata = shmat(shmid[0], NULL, 0)) == 0) die("parent shmat shdata");

    if ((shmid[1] = shmget(++key, getpagesize(), SHMFLG)) == 0) die("parent shmget");
    if ((shdata->children_a = shmat(shmid[1], NULL, 0)) == 0) die("parent shmat shdata->children_a");

    shdata->cur_idx = 0;
}

void free_shmemory() {
    shmdt(shdata->children_a);
    shmdt(shdata);
    while (shmctl(shmid[0], IPC_RMID, NULL)) TEST_ERROR;
    while (shmctl(shmid[1], IPC_RMID, NULL)) TEST_ERROR;
}

void publish_shared_data(struct individuo figlio) {
    if (figlio.tipo == 'B') return;

    struct child_a child;
    child.pid = figlio.pid;
    child.genoma = figlio.genoma;
    child.alive = figlio.alive;

    shdata->children_a[shdata->cur_idx] = child;
    shdata->cur_idx++;
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

unsigned int compile_child_code(char type) {
    char *file[] = {"gcc ./src/child_a.c -o ./exec/child_a.exe", "gcc ./src/child_b.c -o ./exec/child_b.exe"};
    int status = system(type == 'A' ? file[0] : file[1]);

    printf("COMPILE TYPE FILE: %c | terminated with exit status %d\n", type, status);

    return status;
}

int len_of(int value) {
    int l = 1;

    while (value > 9) {
        l++;
        value /= 10;
    }

    return l;
}
