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
#include <stdarg.h>
#include <sys/msg.h>
#include <sys/sem.h>

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

struct individuo {
    char tipo;
    char *nome;
    unsigned long genoma;
    pid_t pid;
    bool alive;
};

struct popolazione {
    size_t size;
    unsigned int cur_idx;
    struct individuo *individui;
};

struct child_a {
    unsigned long genoma;
    pid_t pid;
    bool alive;
};

struct shared_data {
    unsigned long cur_idx;
    struct child_a children_a[];
};

struct stats {
    int num_type_a;
    int num_type_b;
    int num_match;
    int num_birth_death;
    struct popolazione *societa;
};

unsigned long gen_genoma(unsigned long min, unsigned long max);

char *gen_name(char *name);

char gen_type();

struct individuo gen_individuo();

struct individuo gen_individuo_erede(struct individuo a, struct individuo b);

void mate_and_fork(struct individuo a, struct individuo b);

void run_child(struct individuo figlio);

unsigned int rand_interval(unsigned int min, unsigned int max);

unsigned long rand_interval_lu(unsigned long min, unsigned long max);

int len_of(int x);

unsigned long mcd(unsigned long a, unsigned long b);

static void wake_up_process(int signo);

unsigned int compile_child_code(char type);

void init_shmemory();

void free_shmemory();

void init_stats();

void publish_shared_data(struct individuo figlio);

char get_type_from_pid(pid_t pid, struct popolazione);

struct individuo get_individuo_by_pid(int pid, struct popolazione);

int get_index_by_pid(int pid, struct popolazione societa);

void init_societa(int init_pop);

void add_to_societa(struct individuo);

void del_societa();

void print_stats();

int pick_random_process();

int * read_queue();

void alarm_handler(int sig);

struct individuo search_longest_name();

struct individuo search_greater_genoma();

//costants
const int MIN_CHAR = 65; // A
const int MAX_CHAR = 90; // Z
const int SHMFLG = IPC_CREAT | 0666;

// handle termination
volatile sig_atomic_t end_simulation = 0;

// Global variables
int shmid;
key_t key = 1060;
struct shared_data *shdata;
struct popolazione societa;
struct stats statistics;
int trigger_birth_death = 0;
int trigger_end_sim = 0;

// Input arguments;
int INIT_PEOPLE = 10; // number of inital children
unsigned long GENES = 10000;
unsigned int BIRTH_DEATH = 5;   //seconds
unsigned int SIM_TIME = 1 * 60; //seconds

int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        printf("P | arg[%d]: %s \n", i, argv[i]);
    }

    if (argc < 2) {
        printf("P | insufficient arguments\n");
        exit(EXIT_FAILURE);
    }

    if (atoi(argv[1]) < 2) {
        printf("P | initial population is less then 2\n");
        exit(EXIT_FAILURE);
    }

    INIT_PEOPLE = atoi(argv[1]);
    printf("P | INIT_PEOPLE: %d \n", INIT_PEOPLE);

    //init steps
    srand48(time(NULL));
    compile_child_code('A');
    compile_child_code('B');

    init_societa(INIT_PEOPLE);
    init_stats();
    init_shmemory();

    //init population
    pid_t child_pid;
    for (int i = 0; i < INIT_PEOPLE; i++) {
        // generate child
        struct individuo figlio = gen_individuo();
        if (i == 0)figlio.tipo = 'A';
        if (i == 1)figlio.tipo = 'B';
        if (i == 2)figlio.tipo = 'B';
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

        printf("P | CHILD BORN[%d]: %d %c\n", i, child_pid, figlio.tipo);

        /* Perform actions specific to parent */
        figlio.pid = child_pid;
        add_to_societa(figlio);
        publish_shared_data(figlio);
    }

    printf("P | CHILD Generation END\n");

    for (int i = 0; i < shdata->cur_idx; i++) {
        struct child_a child = shdata->children_a[i];
        printf("P | GESTORE pid: %d | [%d] child_a pid: %d \n", getpid(), i, child.pid);
    }

    //creat sem to sync shared memory
    pid_t sem_id = semget(getpid(), 1, IPC_CREAT | 0666);
    printf("P | pid %d | sem_a_id: %d\n", getpid(), sem_id);
    semctl(sem_id, 0, SETVAL, 1);
    TEST_ERROR;

    // make sure child set the signal handler
    sleep(1);

    // send signal to wake up all the children
    for (int i = 0; i < INIT_PEOPLE; i++) {
        printf("P | Sending SIGUSR1 to the child %d...\n", societa.individui[i].pid);
        kill(societa.individui[i].pid, SIGUSR1);
    }

    struct sigaction sa, sa_old;
    sa.sa_handler = &alarm_handler;
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, &sa_old) == -1) {  //settaggi handler
        perror("ERRORE SIGACTION");
        exit(EXIT_FAILURE);
    }
    signal(SIGALRM, alarm_handler);
    alarm(1);

    int status, a_death = 0, b_death = 0, e_death = 0;
    while ((child_pid = wait(&status)) > 0) {
        printf("P | Ended child : %d | status: %d \n", child_pid, status);

        char type = get_type_from_pid(child_pid, societa);
        type == 'A' ? a_death++ : b_death++;
        printf("P | TYPE STAT | A: %d, B: %d, E: %d\n", a_death, b_death, e_death);

        // update alive
        int index = get_index_by_pid(child_pid, societa);

        if (index > -1) societa.individui[index].alive = 0;
        if (status != 0) continue;

        // handle queue msg
        int *rcv = read_queue();
        if (rcv[0] > -1 && !end_simulation) {
            struct individuo a = get_individuo_by_pid(rcv[0], societa);
            struct individuo b = get_individuo_by_pid(rcv[1], societa);
            mate_and_fork(a, b);
        }
    }

    free_shmemory();
    // Now the semaphore can be deallocated
    semctl(sem_id, 0, IPC_RMID);
    printf("\n ---> PARENT END | pid: %d <---\n", getpid());
    exit(EXIT_SUCCESS);
}

int * read_queue() {
    int msgid;
    static int received[] = {-1,-1};
    msgid = msgget(getpid(), 0666);

    if (msgrcv(msgid, &received, sizeof(int), 0, IPC_NOWAIT) == -1) {
        if (errno != ENOMSG) {
            fprintf(stderr, "P | Message could not be received.\n");
        }
        usleep(50000);
        if (msgrcv(msgid, &received, sizeof(int), 0, 0) == -1) {
            fprintf(stderr, "P | Message could not be received.\n");
        }
    }

    printf("P | QUEUE MSG[0]:%d | QUEUE MSG[1]:%d\n", received[0], received[1]);

    return received;
}

unsigned long gen_genoma(unsigned long min, unsigned long max) {
    return rand_interval_lu(min, max);
}

char *gen_name(char *name) {
    int new_length = (int) (strlen(name) + 2);
    char *new_name = calloc(sizeof(char), new_length);

    sprintf(new_name, "%s%c", name, (char) rand_interval(MIN_CHAR, MAX_CHAR));
    printf("P | gen_name: %s\n", new_name);
    return new_name;
}

char gen_type() {
    return rand_interval(1, 10) % 2 == 0 ? 'A' : 'B';
}

struct individuo gen_individuo() {
    struct individuo figlio;

    figlio.tipo = gen_type();
    figlio.nome = gen_name("");
    figlio.genoma = gen_genoma(2, 2 + GENES);
    figlio.alive = true;

    return figlio;
}

struct individuo gen_individuo_erede(struct individuo a, struct individuo b) {
    struct individuo new_child;
    unsigned long x = mcd(a.genoma, b.genoma);
    int new_length = (int) (strlen(a.nome) + strlen(b.nome) + 1);
    char *new_name = calloc(sizeof(char), new_length);

    sprintf(new_name, "%s%s", a.nome, b.nome);

    new_child.tipo = gen_type();
    new_child.nome = gen_name(new_name);
    new_child.genoma = gen_genoma(x, x + GENES);
    new_child.alive = true;


    printf("P | gen_individuo_erede a.nome: %s\n", a.nome);
    printf("P | gen_individuo_erede a.genoma: %lu\n", a.genoma);
    printf("P | gen_individuo_erede b.nome: %s\n", b.nome);
    printf("P | gen_individuo_erede b.genoma: %lu\n", b.genoma);
    printf("P | gen_individuo_erede x: %lu\n", x);
    printf("P | new_child.tipo: %c\n", new_child.tipo);
    printf("P | new_child.nome: %s\n", new_child.nome);
    printf("P | new_child.genoma: %lu\n", new_child.genoma);

    return new_child;
}

unsigned long mcd(unsigned long a, unsigned long b) {
    int remainder;
    while (b != 0) {
        remainder = a % b;

        a = b;
        b = remainder;
    }
    return a;
}

void run_child(struct individuo figlio) {
    int error = 0;
    char *buffer;
    char *argv[] = {NULL, NULL, NULL, NULL};

    printf("CHILD -> NAME: %s | TYPE: %c | GENOMA: %lu \n", figlio.nome, figlio.tipo, figlio.genoma);

    // run execve
    // argv[0] = program name
    argv[0] = "c_noe_fork_child.exe"; //TODO

    // argv[1] = child name
    argv[1] = calloc(strlen(figlio.nome) + 1, sizeof(char));
    strcat(argv[1], figlio.nome);

    // argv[2] = child genoma
    buffer = calloc(len_of(figlio.genoma), sizeof(char));
    argv[2] = calloc(len_of(figlio.genoma) + 1, sizeof(char));
    sprintf(buffer, "%lu", figlio.genoma);
    strcat(argv[2], buffer);
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
    key = ftok("shdata", 1);
    if ((shmid = shmget(key, getpagesize() * 1000, SHMFLG)) == 0) die("parent shmget");
    if ((shdata = shmat(shmid, NULL, 0)) == 0) die("parent shmat shdata");

    shdata->cur_idx = 0;
}

void free_shmemory() {
    shmdt(shdata);
    while (shmctl(shmid, IPC_RMID, NULL)) TEST_ERROR;
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
        printf("P | WAKEUP  process %d | received signal SIGUSR1\n", getpid());
    }
}

unsigned int rand_interval(unsigned int min, unsigned int max) {
    // reference https://stackoverflow.com/a/17554531
    unsigned int r;
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

unsigned long rand_interval_lu(unsigned long min, unsigned long max) {
    // reference https://stackoverflow.com/a/17554531
    unsigned long r;
    const unsigned long range = 1 + max - min;
    const unsigned long buckets = RAND_MAX / range;
    const unsigned long limit = buckets * range;

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

    printf("P | COMPILE TYPE FILE: %c | terminated with exit status %d\n", type, status);

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

char get_type_from_pid(pid_t pid, struct popolazione societa) {
    for (int i = 0; i < societa.cur_idx; i++) {
        if (societa.individui[i].pid == pid) {
            return societa.individui[i].tipo;
        }
    }

    return ' ';
}

int get_index_by_pid(int pid, struct popolazione societa) {
    for (int i = 0; i < societa.cur_idx; i++) {
        if (societa.individui[i].pid == pid) {
            return i;
        }
    }

    return -1;
}

struct individuo get_individuo_by_pid(int pid, struct popolazione societa) {
    for (int i = 0; i < societa.cur_idx; i++) {
        if (societa.individui[i].pid == pid) {
            return societa.individui[i];
        }
    }
    struct individuo no_match;

    return no_match;
}

void mate_and_fork(struct individuo a, struct individuo b) {
    struct individuo new_born;
    pid_t child_pid;

    new_born = gen_individuo_erede(a, b);

    child_pid = fork();
    if (child_pid < 0) {
        die(strerror(errno));
    }

    /* Perform actions specific to child */
    if (child_pid == 0) {
        run_child(new_born);
        exit(EXIT_FAILURE);
    }

    printf("P | mate_and_fork CHILD BORN: %d %c\n", child_pid, new_born.tipo);

    /* Perform actions specific to parent */
    new_born.pid = child_pid;
    add_to_societa(new_born);
    publish_shared_data(new_born);
}

void init_societa(int init_pop) {
    size_t size = init_pop * sizeof(struct individuo);
    societa.individui = malloc(size);
    if (societa.individui == NULL) {
        free(societa.individui);
    }

    societa.size = size;
    societa.cur_idx = 0;
}

void add_to_societa(struct individuo child) {
    if (societa.size == societa.cur_idx + 1) {
        size_t new_size = (societa.size + 50) * sizeof(struct individuo);
        societa.individui = realloc(societa.individui, new_size);
        societa.size = new_size;
    }
    societa.individui[societa.cur_idx++] = child;
}

void del_societa() {
    if (societa.individui != NULL) {
        free(societa.individui);
    }
    societa.size = 0;
    societa.cur_idx = 0;
}

void init_stats() {
    statistics.num_match = 0;
    statistics.num_type_a = 0;
    statistics.num_type_b = 0;
    statistics.num_birth_death = 0;
    statistics.societa = &societa;
}

int pick_random_process() {
    int pids_index[societa.cur_idx];
    int num_alive = 0;

    //filter alive
    for (int i = 0; i < societa.cur_idx; i++) {
        if (societa.individui[i].alive) {
            pids_index[num_alive] = i;
            num_alive++;
        }
    }

    // pick one of them
    int index = rand_interval(0, num_alive);

    return pids_index[index];
}

void alarm_handler(int sig) {
    printf("P| alarm_handler: signal%d\n", sig);
    if (trigger_birth_death == BIRTH_DEATH) {
        trigger_birth_death = 0;

        // kill random child
        int index, status;

        index = pick_random_process();
        // TODO add semaphores
        if (societa.individui[index].tipo == 'B') {
            shdata->children_a[index].alive = 0;
        }

        pid_t pid = societa.individui[index].pid;
        kill(pid, SIGTERM);

        while (waitpid(pid, &status, 0) != -1);

        

        // generate new child
        struct individuo figlio = gen_individuo();
        pid_t child_pid = fork();

        /* Handle error create child*/
        if (child_pid < 0) die(strerror(errno));

        /* Perform actions specific to child */
        if (child_pid == 0) {
            run_child(figlio);
            exit(EXIT_FAILURE);
        }

        printf("P | CHILD BORN: %d %c\n", child_pid, figlio.tipo);

        /* Perform actions specific to parent */
        figlio.pid = child_pid;
        add_to_societa(figlio);
        publish_shared_data(figlio);

        // print stats
        print_stats();
    }

    if (trigger_end_sim == SIM_TIME) {
        printf("P| alarm_handler: trigger_end_sim\n");
        for(int i=0; i<societa.cur_idx;i++){
            kill(societa.individui[i].pid, SIGTERM);
        }
    }

    trigger_birth_death++;
    trigger_end_sim++;

    if(!end_simulation) alarm(1);
}

struct individuo search_longest_name() {
    int max_len = 0;
    int index_max_len = 0;
    for (int i = 0; i < societa.cur_idx; i++) {
        if (strlen(societa.individui[i].nome) > max_len) {
            max_len = strlen(societa.individui[i].nome);
            index_max_len = i;
        }
    }
    return societa.individui[index_max_len];
}

struct individuo search_greater_genoma() {
    int max_genoma = 0;
    int index_max_genoma = 0;
    for (int i = 0; i < societa.cur_idx; i++) {
        if (societa.individui[i].genoma > max_genoma) {
            max_genoma = strlen(societa.individui[i].nome);
            index_max_genoma = i;
        }
    }
    return societa.individui[index_max_genoma];
}

void print_stats() {
    struct individuo child_longest_name = search_longest_name();
    struct individuo child_greater_genoma = search_greater_genoma();

    printf("P| *** STATISTIC UPDATE ***\n");
    printf("P| num_match: %d\n", statistics.num_match);
    printf("P| num_type_a: %d\n", statistics.num_type_a);
    printf("P| num_type_b: %d\n", statistics.num_type_b);
    printf("P| num_birth_death: %d\n", statistics.num_birth_death);
    printf("P| total population: %d\n", statistics.societa->cur_idx);
    printf("P| *** STATISTIC END ***\n");
}