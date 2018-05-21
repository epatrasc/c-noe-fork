#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/file.h>
#include <sys/sem.h>

#define TEST_ERROR    if (errno) {fprintf(stderr, \
                       "%s:%d: PID=%5d: Error %d (%s)\n",\
                       __FILE__,\
                       __LINE__,\
                       getpid(),\
                       errno,\
                       strerror(errno));}

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
    struct child_a children_a[];
};

int mcd(unsigned long a, unsigned long b);
bool isForMe(unsigned long gen_a, unsigned long gen_b);
int searchPatner(struct individuo my_info, struct shared_data *shdata);
void open_shmemory();
bool isAlive(int index);
void setDead(int index);
void send_msg_to_gestore(int pid_a);
void exit_handler(void);
int contact_patner(int index, struct individuo my_info, int pida);
void handle_termination(int signum);


// Global variable
const int SHMFLG = IPC_CREAT | 0666;
struct sembuf sem_1_l = {0, -1, 0};
struct sembuf sem_1_u = {0, 1, 0};
struct sembuf sem_2_l = {0, -1, 0};
struct sembuf sem_2_u = {0, 1, 0};

unsigned int shmid;
struct shared_data *shdata;
int sem_shm_id, sem_a_id;

// handle termination
volatile sig_atomic_t done = 0;

int main(int argc, char *argv[]) {
    struct individuo my_info;
    struct sigaction action;

    atexit(exit_handler);
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = handle_termination;
    sigaction(SIGTERM, &action, NULL);
    
    // printf("\n ---> CHILD B START | pid: %d <---\n", getpid());
    if (argc < 2) {
        // printf("B | I need name and genoma input from argv. \n");
        for (int i = 0; i < argc; i++) {
            // printf("B | arg[%d]: %s \n ", i, argv[i]);
        }
        exit(EXIT_FAILURE);
    }

    my_info.nome = argv[1];
    my_info.tipo = 'B';
    my_info.genoma = (unsigned long) strtol(argv[2], NULL, 10);

    // printf("B | pid: %d | my_info.nome: %s \n", getpid(), my_info.nome);
    // printf("B | pid: %d | my_info.tipo: %c \n", getpid(), my_info.tipo);
    // printf("B | pid: %d | my_info.genoma: %lu \n", getpid(), my_info.genoma);

    sem_shm_id = semget(getppid(), 1, IPC_CREAT | 0666);
    TEST_ERROR;

    // attach shm
    semop(sem_shm_id, &sem_1_l, 1);
    open_shmemory();
    semop(sem_shm_id, &sem_1_u, 1);


    // create fifo
    char *pid_s = calloc(sizeof(char), sizeof(pid_t));
    sprintf(pid_s, "%d", getpid());
    mkfifo(pid_s, S_IRUSR | S_IWUSR);

    // search patner
    int foundMate = 0;
    while(!foundMate && !done){
        int index = -1;
        // printf("B | PID: %d, Cerco patner...\n", getpid());
        if((index = searchPatner(my_info, shdata))>=0){
            // printf("B | PID: %d, patner ideale trovato: %d\n", getpid(), index);
            
            int pida =  shdata->children_a[index].pid;
            if((foundMate = contact_patner(index, my_info, pida)) == 1){
                // printf("B | PID: %d, patner ha accettato: %d\n", getpid(), index);
                send_msg_to_gestore(pida);
            }else{
                // printf("B | PID: %d, patner NON ha accettato: %d\n", getpid(), index);
            }
        }
        usleep(50000);
    }
    // printf(" ---> CHILD B END | pid: %d <---\n", getpid());

    remove(pid_s);
    free(pid_s);
    exit(EXIT_SUCCESS);
}

void send_msg_to_gestore(int pid_a) {
    // printf("B | PID: %d, contacting parent..\n", getpid());
    // send info to gestore
    int key = getppid();
    int msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1) {
        msgid = msgget(key, 0666 | IPC_CREAT);
        if (msgid == -1) {
            fprintf(stderr, "B | Could not create message queue.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Send messages ..
    int ret, msg[2] = {getpid(), pid_a};
    ret = msgsnd(msgid, msg, sizeof(int), IPC_NOWAIT);
    if (ret == -1) {
        if (errno != EAGAIN) {
            fprintf(stderr, "B | Message could not be sended.\n");
            exit(EXIT_FAILURE);
        }
        usleep(50000);//50ms
        //try again
        if (msgsnd(msgid, msg, sizeof(int), 0) == -1) {
            fprintf(stderr, "B | Message could not be sended.\n");
            exit(EXIT_FAILURE);
        }
    }
    // printf("B | PID: %d, message to the parent sent.\n", getpid());
}

void open_shmemory() {
    key_t key = ftok("shdata", 1);
    if ((shmid = shmget(key, 0, 0)) == 0) {
        perror("B | cannot get shared memory id | shdata\n");
        exit(EXIT_FAILURE);
    }

    if ((shdata = shmat(shmid, NULL, 0)) == (void *) -1) {
        perror("B | cannot attach shared memory to address \n");
        exit(EXIT_FAILURE);
    }
}

int mcd(unsigned long a, unsigned long b) {
    int remainder;
    while (b != 0) {
        remainder = a % b;

        a = b;
        b = remainder;
    }
    return a;
}

bool isForMe(unsigned long gen_a, unsigned long gen_b) {
    if (gen_a % gen_b == 0) {
        return true;
    }

    if (mcd(gen_a, gen_b) >= 1) {
        return true;
    }

    return false;
}

int searchPatner(struct individuo my_info, struct shared_data *shdata){
    int patnerIndex = -1;
    int maxMCD = 0;

    for (int i = 0; i < shdata->cur_idx; i++) {
        if(!shdata->children_a[i].alive) continue;

        struct child_a child = shdata->children_a[i];
        
        // perfect match
        if (child.genoma % my_info.genoma == 0) {
            return i;
        }
        int mcd_value = mcd(child.genoma, my_info.genoma);

        // search max mcd
        if ( mcd_value > maxMCD) {
            patnerIndex = i;
            maxMCD  = mcd_value;
        }
    }

    return patnerIndex;
}

int contact_patner(int index, struct individuo my_info, int pida){
    struct child_a child;
    char *pida_s = calloc(sizeof(char), sizeof(pid_t));
    char *pid_s = calloc(sizeof(char), sizeof(pid_t));
    sprintf(pid_s, "%d", getpid());

    // access to the child semaphore
    sem_a_id = semget(pida, 1, IPC_CREAT | 0666);
    
    // print semaphore status
    int sem_val = semctl(sem_a_id, 0, GETVAL);
    // printf("B | pid %d | sem_a_id: %d | sem_val: %d\n", getpid(), sem_a_id, sem_val);

    // lock resource
    semop(sem_a_id, &sem_2_l, 1);
    TEST_ERROR;
    // printf("B1 | pid %d | took control of %d semaphore\n", getpid(), sem_a_id);

    // skip dead child
    if (!isAlive(index)) {
        // printf("B | pid: %d | shdata->children_a[%d] NOT alive \n", getpid(), index);
        // lock resource
        semop(sem_a_id, &sem_2_u, 1);
        TEST_ERROR;
        return 0;
    }
    // printf("B |  pid: %d | shdata->children_a[%d] IS alive \n", getpid(), index);

    child.pid = shdata->children_a[index].pid;
    child.genoma = shdata->children_a[index].genoma;
    child.alive = shdata->children_a[index].alive;
    // printf("B |  pid: %d | child->pid: %d \n", getpid(), child.pid);
    // printf("B |  pid: %d | child->genoma: %lu \n", getpid(), child.genoma);
    // printf("B |  pid: %d | child->alive: %d \n", getpid(), child.alive);
    
    // Write message to A
    sprintf(pida_s, "%d", child.pid);
    // printf("B | pid: %d, shdata->children_a[i]: %s\n", getpid(), pida_s);

    int fifo_a = open(pida_s, O_WRONLY);
    if (fifo_a == -1) {
        // printf("B | pid: %d | %s:%d | error fifo_a returned: %d\n", getpid(), __FILE__, __LINE__, fifo_a);
        exit(EXIT_FAILURE);
    }

    // printf("B | PID: %d | writing to A: start\n", getpid());
    char *my_msg = calloc(sizeof(char), 1024);
    int str_len = sprintf(my_msg, "%d,%s,%lu", getpid(), my_info.nome, my_info.genoma);
    write(fifo_a, my_msg, str_len);
    free(my_msg);
    close(fifo_a);
    // printf("B | PID: %d | writing to A: end\n", getpid());

    // read answer
    // printf("B | PID: %d | reading response from A...\n", getpid());
    int BUF_SIZE = 1024;
    char *readbuf = calloc(sizeof(char), BUF_SIZE);
    int fifo_b = open(pid_s, O_RDONLY);
    ssize_t num_bytes = read(fifo_b, readbuf, BUF_SIZE);
    close(fifo_b);

    // printf("B | num_bytes: %li\n", num_bytes);
    // printf("B | PID: %d | A with pid %s has response: %s\n", getpid(), pida_s, readbuf);

    // printf("B | readbuf[0] == '1': %li\n", strtol(readbuf, NULL, 10));

    int foundMate = 0;
    if (strtol(readbuf, NULL, 10) == 1) {
        foundMate = 1;
        // printf("B | pid %d | foundMate \n", getpid());
        setDead(index);
    }

    free(pida_s);
    free(readbuf);
    // Release the resource
    semop(sem_a_id, &sem_2_u, 1);
    TEST_ERROR;

    return foundMate;
}

bool isAlive(int index) {
    semop(sem_shm_id, &sem_1_l, 1);
    bool alive = shdata->children_a[index].alive;
    semop(sem_shm_id, &sem_1_u, 1);;
    return alive;
}

void setDead(int index) {
    semop(sem_shm_id, &sem_1_l, 1);
    shdata->children_a[index].alive = false;
    semop(sem_shm_id, &sem_1_u, 1);
}

void exit_handler(void)
{
    // printf("B | PID: %d | exit_handler \n", getpid());
    semop(sem_shm_id, &sem_1_u, 1);
    semop(sem_a_id, &sem_2_u, 1);
    //abort();
}

void handle_termination(int signum) {
    extern int sem_shm_id, sem_a_id;
    printf("B | pid: %d | SIGTERM recevived\n", getpid());
    done = 1;
}