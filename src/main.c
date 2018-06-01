#include "library.h"

unsigned long gen_genoma(unsigned long min, unsigned long max);

char *gen_name(char *name);

char gen_type();

struct individuo gen_individuo();

struct individuo gen_individuo_erede(struct individuo a, struct individuo b);

void mate_and_fork(struct individuo a, struct individuo b);

void run_child(struct individuo figlio);

static void wake_up_process(int signo);

unsigned int compile_child_code(char type);

void init_stats();

void publish_shared_data(struct individuo figlio);

struct individuo get_individuo_by_pid(int pid, struct popolazione);

int get_index_by_pid(int pid, struct popolazione societa);

void init_societa(int init_pop);

void add_to_societa(struct individuo);

void del_societa();

void print_stats();

int pick_random_process();

struct message read_queue();

void alarm_handler(int sig);

void init_shmemory();

void free_shmemory();

struct individuo search_longest_name();

struct individuo search_highest_genoma();

//costants
const int MIN_CHAR = 65; // A
const int MAX_CHAR = 90; // Z
const int SHMFLG = IPC_CREAT | 0666;

// handle termination
volatile sig_atomic_t end_simulation = 0;

// Global variables
int shmid, sem_id;
key_t key = 1060;
struct shared_data *shdata;
struct popolazione societa;
struct stats statistics;
int trigger_birth_death = 0;
int trigger_end_sim = 0;
struct sembuf sem_1_l = {0, -1, 0};
struct sembuf sem_1_u = {0, 1, 0};
int time_sim_remain = 0;

// Input arguments;
unsigned int INIT_PEOPLE = 10; // number of inital children
unsigned long GENES = 100000;
unsigned int BIRTH_DEATH = 5;   //seconds
unsigned int SIM_TIME = 60; //seconds

int main(int argc, char *argv[]) {
    // init config
    if (argc < 2) {
        printf("P | argc < 2 | Switch to csv mode config\n");
        // config from csv
        char line_1[1024];
        char line_2[1024];
        char *tmp = NULL;
        FILE *stream = fopen("./config/config.csv", "r");
        // first line header
        fgets(line_1, 1024, stream);
        // print first csv header
        printf("P | Printing csv header...\n");
    
        for (int i = 1; i < 5; i++) {
            tmp = strdup(line_1);
            printf("P | Field[%d] name:  %s\n", i, getfield(tmp, i));
        }
        free(tmp);
        tmp = NULL;
    
        // second line arguments
        fgets(line_2, 1024, stream);
    
        
        tmp = strdup(line_2);
        INIT_PEOPLE = (unsigned int) strtol(getfield(tmp, 1), NULL, 10); // int
        tmp = strdup(line_2);
        GENES = (unsigned long) strtol(getfield(tmp, 2), NULL, 10);
        tmp = strdup(line_2);
        BIRTH_DEATH = (unsigned int) strtol(getfield(tmp, 3), NULL, 10);
        tmp = strdup(line_2);
        SIM_TIME = (unsigned int) strtol(getfield(tmp, 4), NULL, 10);

        printf("P | Field[INIT_PEOPLE]: %d\n", INIT_PEOPLE);
        printf("P | Field[GENES]: %lu\n", GENES);
        printf("P | Field[BIRTH_DEATH]: %d\n", BIRTH_DEATH);
        printf("P | Field[SIM_TIME]: %d\n", SIM_TIME);

        free(tmp);
        tmp = NULL;
    }

    if (argc >= 2) {
        printf("P | argc > 2 | Set config from command-line\n");
        for (int i = 0; i < argc; i++) {
            printf("P | arg[%d]: %s \n", i, argv[i]);
        }

        INIT_PEOPLE = (unsigned int) strtol(argv[1], NULL, 10); // int

        if(argc >= 3){
            GENES = (unsigned long) strtol(argv[2], NULL, 10);
            BIRTH_DEATH = (unsigned int) strtol(argv[3], NULL, 10);
            SIM_TIME = (unsigned int) strtol(argv[4], NULL, 10);
        }
    }

    printf("P | Run with the following config:\n");
    printf("P | INIT_PEOPLE: %d\n", INIT_PEOPLE);
    printf("P | GENES: %lu\n", GENES);
    printf("P | BIRTH_DEATH: %d\n", BIRTH_DEATH);
    printf("P | SIM_TIME: %d\n", SIM_TIME);

    // validate input
    if (INIT_PEOPLE < 2) {
        printf("P | ERROR | INIT_PEOPLE is less then 2. Change config.\n");
        exit(EXIT_FAILURE);
    }

    if (GENES < 2) {
        printf("P | ERROR | GENES is less then 2. Change config.\n");
        exit(EXIT_FAILURE);
    }

    if (BIRTH_DEATH < 1) {
        printf("P | ERROR | BIRTH_DEATH is less then 1. Change config.\n");
        exit(EXIT_FAILURE);
    }

    if (SIM_TIME < 1 && SIM_TIME <= BIRTH_DEATH ) {
        printf("P | ERROR | SIM_TIME is less then 1 or BIRTH_DEATH. Change config.\n");
        exit(EXIT_FAILURE);
    }

    //init steps
    srand48(time(NULL));
    compile_child_code('A');
    compile_child_code('B');

    init_societa(INIT_PEOPLE);
    init_stats();
    init_shmemory();
    time_sim_remain = SIM_TIME;

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
    sem_id = semget(getpid(), 1, IPC_CREAT | 0666);
    printf("P | pid %d | sem_a_id: %d\n", getpid(), sem_id);
    semctl(sem_id, 0, SETVAL, 1);

    // make sure child set the signal handler
    usleep(50000);

    // send signal to wake up all the children
    for (int i = 0; i < INIT_PEOPLE; i++) {
        printf("P | Sending SIGUSR1 to the child %d...\n", societa.individui[i].pid);
        kill(societa.individui[i].pid, SIGUSR1);
    }
    struct sigaction sa, sa_old;
    sa.sa_handler = &alarm_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, &sa_old);

    alarm(1);

    // main loop
    int wstatus;
    int cnt_kill = 0;
    while ((child_pid = wait(&wstatus)) > 0) {
        cnt_kill++;
        if (child_pid <= 0) {
            usleep(50000);
            continue;
        }
        printf("P | Ended child : %d | status: %d \n", child_pid, wstatus);

        // update alive
        int index = get_index_by_pid(child_pid, societa);
        printf("P | index: %d\n", index);

        if (index > -1) societa.individui[index].alive = 0;

        // handle queue msg
        struct message received;

        received = read_queue();

        while (received.pid_sender > 0) {

            struct individuo a = get_individuo_by_pid(received.pid_sender, societa);
            struct individuo b = get_individuo_by_pid(received.pid_match, societa);
            mate_and_fork(a, b);
            received = read_queue();
        }
    }

    // clean-up
    free_shmemory();
    del_societa();
    semctl(sem_id, 0, IPC_RMID);

    if (msgctl(sem_id, IPC_RMID, NULL) == -1) {
        fprintf(stderr, "P | Message queue could not be deleted.\n");
    }

    printf("\n ---> PARENT END | pid: %d <---\n", getpid());
    exit(EXIT_SUCCESS);
}

struct message read_queue() {
    int msgid = msgget(getpid(), 0666);
    struct msgbuf msgbuffer;

    msgbuffer.mtext.pid_sender = -1;
    msgbuffer.mtext.pid_match = -1;

    int mreturn = -1;
    if ((mreturn = msgrcv(msgid, &msgbuffer, sizeof(msgbuffer), 0, IPC_NOWAIT)) == -1) {
        if (errno != ENOMSG) {
            printf("P | msgrcv errno: %d | Retry...\n", errno);
            usleep(50000);//50 ms
            if ((mreturn = msgrcv(msgid, &msgbuffer, sizeof(msgbuffer), 0, IPC_NOWAIT)) == -1) {
                printf("P | msgrcv errno: %d \n", errno);
            }
        }

        if (errno == ENOMSG) {
            printf("P | errno: %d | NO MESSAGE QUEUE\n", errno);
        }
    }

    pid_t sender = msgbuffer.mtext.pid_sender;
    pid_t match = msgbuffer.mtext.pid_match;
    printf("P | QUEUE MSG | sender: %d | match: %d\n", sender, match);

    return msgbuffer.mtext;
}

unsigned long gen_genoma(unsigned long min, unsigned long max) {
    return rand_interval_lu(min, max);
}

char *gen_name(char *name) {
    int new_length = (int) (strlen(name) + 2);
    char *new_name = calloc(sizeof(char), new_length);
    printf("P | gen_name name: %s\n", name);
    sprintf(new_name, "%s%c", name, (char) rand_interval(MIN_CHAR, MAX_CHAR));
    printf("P | gen_name new_name: %s\n", new_name);
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

void run_child(struct individuo figlio) {
    int error = 0;
    char *argv[] = {NULL, NULL, NULL, NULL};

    printf("CHILD -> NAME: %s | TYPE: %c | GENOMA: %lu \n", figlio.nome, figlio.tipo, figlio.genoma);

    // run execve
    argv[0] = "c_noe_fork_child.exe";

    // argv[1] = child name
    argv[1] = calloc(strlen(figlio.nome) + 1, sizeof(char));
    strcat(argv[1], figlio.nome);

    // argv[2] = child genoma
    argv[2] = calloc(len_of(figlio.genoma) + 1, sizeof(char));
    sprintf(argv[2], "%lu", figlio.genoma);

    error = execv(figlio.tipo == 'A' ? "./exec/child_a.exe" : "./exec/child_b.exe", argv);

    // if here i'm in error
    if (error < 0) {
        die("P | Errore execve child");
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

unsigned int compile_child_code(char type) {
    char *file[] = {"gcc ./src/child_a.c ./exec/library.o -o ./exec/child_a.exe", "gcc ./src/child_b.c ./exec/library.o -o ./exec/child_b.exe"};
    int status = system(type == 'A' ? file[0] : file[1]);

    printf("P | COMPILE TYPE FILE: %c | terminated with exit status %d\n", type, status);

    return status;
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

    printf("P | mate_and_fork \n");

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

    // update statistics
    new_born.tipo == 'A' ? statistics.num_type_a++ : statistics.num_type_b++;
    statistics.num_match_born++;
}

void init_societa(int init_pop) {
    size_t size = init_pop + 50 * sizeof(struct individuo);
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
    societa.individui[societa.cur_idx] = child;
    societa.cur_idx++;
}

void del_societa() {
    if (societa.individui != NULL) {
        free(societa.individui);
    }
    societa.size = 0;
    societa.cur_idx = 0;
}

void init_stats() {
    statistics.num_type_a = 0;
    statistics.num_type_b = 0;
    statistics.num_birth_death = 0;
    statistics.societa = &societa;
}

int pick_random_process() {
    int pids_index[societa.cur_idx];
    unsigned int num_alive = 0;

    //filter alive
    for (int i = 0; i < societa.cur_idx; i++) {
        if (societa.individui[i].alive) {
            pids_index[num_alive] = i;
            num_alive++;
        }
    }

    if (num_alive == 0) return -1;
    if (num_alive == 1) return pids_index[0];

    // pick one of them
    int index = rand_interval(0, num_alive - 1);

    return pids_index[index];
}

void alarm_handler(int sig) {
    printf("P | signal: %d | Seconds to end sim: %d\n", sig, --time_sim_remain);

    if (trigger_end_sim == SIM_TIME) {
        printf("P | ---> END SIMULAZIONE <--- | alarm_handler: trigger_end_sim\n");
        for (int i = 0; i < societa.cur_idx; i++) {
            kill(societa.individui[i].pid, SIGTERM);
        }
        print_stats();
        end_simulation = true;
    } else if (trigger_birth_death == BIRTH_DEATH) {
        trigger_birth_death = 0;

        // kill random child
        int index = pick_random_process();
        if (index >= 0) {
            semop(sem_id, &sem_1_l, 1);
            shdata->children_a[index].alive = 0;
            semop(sem_id, &sem_1_u, 1);

            pid_t pid = societa.individui[index].pid;
            kill(pid, SIGTERM);
        }

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

        figlio.tipo == 'A' ? statistics.num_type_a++ : statistics.num_type_b++;
        statistics.num_birth_death++;

        // print stats
        print_stats();
    }

    trigger_birth_death++;
    trigger_end_sim++;
    if (!end_simulation) alarm(1);
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

struct individuo search_highest_genoma() {
    int max_genoma = 0;
    int index_max_genoma = 0;
    for (int i = 0; i < societa.cur_idx; i++) {
        if (societa.individui[i].genoma > max_genoma) {
            max_genoma = societa.individui[i].genoma;
            index_max_genoma = i;
        }
    }
    return societa.individui[index_max_genoma];
}

void print_stats() {
    struct individuo child_longest_name = search_longest_name();
    struct individuo child_highest_genoma = search_highest_genoma();

    putchar ( '\n' );

    printf("*** AGGIORNAMENTO STATISTICHE ***\n");
    printf("Numero processi | A: %d , B: %d\n",
           statistics.num_type_a, statistics.num_type_b);
    printf("Numero di birth_death: %d\n", statistics.num_birth_death);
    printf("Numero di match: %d\n", statistics.num_match_born);
    printf("Processo con nome piu' lungo:\n");
    printf("- nome: %s\n", child_longest_name.nome);
    printf("- genoma: %lu\n", child_longest_name.genoma);
    printf("- tipo: %c\n", child_longest_name.tipo);
    printf("- pid: %d\n", child_longest_name.pid);

    printf("Processo con genoma piu' alto:\n");
    printf("- nome: %s\n", child_highest_genoma.nome);
    printf("- genoma: %lu\n", child_highest_genoma.genoma);
    printf("- tipo: %c\n", child_highest_genoma.tipo);
    printf("- pid: %d\n", child_highest_genoma.pid);

    printf("Popolazione totale: %d\n", statistics.societa->cur_idx);
    printf("*** FINE STATISTICHE ***\n");
}