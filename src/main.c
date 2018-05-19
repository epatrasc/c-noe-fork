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

struct  popolazione {
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
    int cnt_total_pop;
};

unsigned long gen_genoma();
char *gen_name(char *name);
char gen_type();
struct individuo gen_individuo();
void mate_and_fork(struct individuo a, struct individuo b);
void run_parent(pid_t gestore_pid, pid_t pg_pid);
void run_child(struct individuo figlio);
unsigned int rand_interval(unsigned int min, unsigned int max);
int len_of(int x);
static void wake_up_process(int signo);
unsigned int compile_child_code(char type);
void init_shmemory();
void free_shmemory();
void publish_shared_data(struct individuo figlio);
char get_type_from_pid(pid_t pid, struct popolazione);
struct individuo get_individuo_by_pid(int pid, struct popolazione);
void init_societa(int init_pop);
void add_to_societa(struct individuo);
void del_societa ();

//void alarm_handler(int sig);

//costants
const int MIN_CHAR = 65; // A
const int MAX_CHAR = 90; // Z
const int SHMFLG = IPC_CREAT | 0666;

// Global variables
int shmid;
key_t key = 1060;
struct shared_data *shdata;
struct popolazione societa;

// Input arguments;
int INIT_PEOPLE = 10; // number of inital children
unsigned long GENES = 10000;
unsigned int BIRTH_DEATH = 5;   //seconds
unsigned int SIM_TIME = 1 * 60; //seconds

int main(int argc, char *argv[]) {
    //init steps
    srand48(time(NULL));
    init_shmemory();
    compile_child_code('A');
    compile_child_code('B');

    run_parent(getpid(), getppid());

    for (int i = 0; i < argc; i++) {
        printf("P | arg[%d]: %s \n", i, argv[i]);
    }

    if (argc < 2) {
        printf("P | insufficient arguments\n");
        exit(EXIT_FAILURE);
    }

    if(atoi(argv[1]) < 2){
        printf("P | initial population is less then 2\n");
        exit(EXIT_FAILURE);
    }

    INIT_PEOPLE = atoi(argv[1]);
    printf("P | INIT_PEOPLE: %d \n", INIT_PEOPLE);

    init_societa(INIT_PEOPLE);
    
    //init population
    pid_t child_pid;
    for (int i = 0; i < INIT_PEOPLE; i++) {
        // generate child
        struct individuo figlio = gen_individuo();
        if(i==0)figlio.tipo= 'A';
        if(i==1)figlio.tipo= 'B';
        if(i==2)figlio.tipo= 'B';
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
    printf("P | pid %d | sem_id: %d\n", getpid(),sem_id);
    semctl(sem_id, 0, SETVAL, 1);
    TEST_ERROR;

    // make sure child set the signal handler
    sleep(1);

    // send signal to wake up all the children
    for (int i = 0; i < INIT_PEOPLE; i++) {
        printf("P | Sending SIGUSR1 to the child %d...\n", societa.individui[i].pid);
        kill(societa.individui[i].pid, SIGUSR1);
    }

    // struct sigaction sa, sa_old;
    // sa.sa_handler = &Alarm_handler;
    // sa.sa_flags = 0;
    // if (sigaction(SIGALRM, &sa, &sa_old) == -1) {  //settaggi handler
    //     perror("ERRORE SIGACTION");
    //     exit(EXIT_FAILURE);
    // }
    // signal(SIGALRM, alarm_handler);

    int status;
    int a_death=0, b_death=0, e_death=0;
    while ((child_pid = wait(&status)) > 0) {
        printf("P | Ended child : %d | status: %d \n", child_pid, status);
        char type = get_type_from_pid(child_pid, societa);
        if (type == 'E') {
            e_death++;
        } else {
            type == 'A' ? a_death++ : b_death++;
        }
        printf("P | TYPE STAT | A: %d, B: %d, E: %d\n", a_death, b_death, e_death);

        if (status != 0) {
            continue;
        }
        int key, mask, msgid, rcv[2], pid_a, pid_b;

        key = getpid();
        mask = 0666;
        msgid = msgget(key, mask);
        if (msgid == -1) {
            usleep(50000);//50ms
            msgid = msgget(key, mask);
        }

        if (msgrcv(msgid, &rcv, sizeof(int), 0, IPC_NOWAIT) == -1) {
            if (errno != ENOMSG) {
                fprintf(stderr, "P | Message could not be received.\n");
                continue;
            }
           usleep(50000);
           if (msgrcv(msgid, &rcv, sizeof(int), 0, 0) == -1) {
               fprintf(stderr, "Message could not be received.\n");
               continue;
           }
        }
        printf("P | QUEUE MSG[0]:%d \n", rcv[0]);
        printf("P | QUEUE MSG[1]:%d \n", rcv[1]);

        struct individuo a = get_individuo_by_pid(rcv[0], societa);
        struct individuo b = get_individuo_by_pid(rcv[1], societa);
        mate_and_fork(a,b);
    }

    free_shmemory();
    // Now the semaphore can be deallocated
	semctl(sem_id, 0, IPC_RMID);
    printf("\n ---> PARENT END | pid: %d <---\n", getpid());
    exit(EXIT_SUCCESS);
}

unsigned long gen_genoma() {
    return (unsigned long) lrand48();
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
    return rand_interval(1, 10) % 2 == 0 ? 'A' : 'B';
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
    printf("P |  PID=%d, PPID=%d\n", gestore_pid, pg_pid);
}

void run_child(struct individuo figlio) {
    int error = 0;
    char *buffer;
    char *argv[] = {NULL, NULL, NULL, NULL};

    printf("CHILD -> NAME: %s | TYPE: %c | GENOMA: %lu \n", figlio.nome, figlio.tipo, figlio.genoma);

    // run execve
    // argv[0] = program name
    argv[0] = "NOME PROGRAMMA"; //TODO

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
    return 'E';
}

struct individuo get_individuo_by_pid(int pid, struct  popolazione societa){
    for (int i = 0; i < societa.cur_idx; i++) {
        if (societa.individui[i].pid == pid) {
            return societa.individui[i];
        }
    }
    struct individuo no_match;

    return no_match;
}

void mate_and_fork(struct individuo a, struct individuo b){
    struct individuo new_born;
    pid_t child_pid;
    
    new_born = gen_individuo();

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

void init_societa(int  init_pop){
    size_t size = init_pop * sizeof (struct individuo);
    societa.individui = malloc (size);
    if (societa.individui == NULL) {
        free (societa.individui);
    }

    societa.size = size;
    societa.cur_idx = 0;
}

void add_to_societa(struct individuo child){
    if(societa.size == societa.cur_idx + 1){
        size_t new_size = (societa.size + 50) * sizeof (struct individuo);
        societa.individui = realloc(societa.individui, new_size);
        societa.size = new_size;
    }
    societa.individui[societa.cur_idx++] = child;
}

void del_societa () {
    if (societa.individui != NULL) {
        free (societa.individui);
    }
    societa.size = 0;
    societa.cur_idx = 0;
}

// void alarm_handler(int sig){
//     extern char **argvarb;
//     int i,status;
//     pid_t Child;
//     extern bool terminato;
//     struct sembuf palla;
//     extern pid_t pallaID;
//     free(argvarb);
    
//     palla.sem_num=0;
//     palla.sem_op=-1;    // tolgo la palla per la terminazione corretta di tutti i processi
//     semop(pallaID,&palla,1);
    
//     signal(SIGTERM, SIG_IGN);
    
//     for(i=0; i<3; i++){         //deallocazione
//         kill(Spid[i],SIGTERM);
        
//         while((Child = wait(&status)) !=-1); //aspetto la terminazione delle squadre e del fato
            
//     }
    
//     if(semctl(pallaID,0,IPC_RMID)==-1)      //rimuovo semaforo palla
//         perror("errore rimozione del semaforo");
    
//     printf("________________________FINE PARTITA______________________\n\n");
//     printf("   IL PUNTEGGIO FINALE E' :  SQUADRA A: %d   SQUADRA B: %d\n\n",Punt_A,Punt_B);
//     terminato=TRUE;
// }

