#include "library.h"

bool isForMe(unsigned long gen_a, unsigned long gen_b);

int searchPatner(struct individuo my_info, struct shared_data *shdata);

void open_shmemory();

bool isAlive(int index);

void setDead(int index);

void send_msg_to_gestore(int pid_a);

void exit_handler(void);

int contact_patner(int index, struct individuo my_info, int pida);

void handle_termination(int signum);

void timeout_handler(int signum);

// Global variable
const int SHMFLG = IPC_CREAT | 0666;
struct sembuf sem_1_l = {0, -1, 0};
struct sembuf sem_1_u = {0, 1, 0};
struct sembuf sem_2_l = {0, -1, 0};
struct sembuf sem_2_u = {0, 1, 0};

unsigned int shmid;
struct shared_data *shdata;
int sem_shm_id, sem_a_id;
char *pid_s;

// handle termination
volatile sig_atomic_t done = 0;

int main(int argc, char *argv[]) {
    struct individuo my_info;
    struct sigaction action, sa, sa_old;

    atexit(exit_handler);
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = &handle_termination;
    action.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &action, NULL);
    sigemptyset(&action.sa_mask);

    // mask timeout alarm
    sa.sa_handler = &timeout_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, &sa_old);

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
    pid_s = calloc(sizeof(char), sizeof(pid_t));
    sprintf(pid_s, "%d_B", getpid());
    mkfifo(pid_s, S_IRUSR | S_IWUSR);

    // search patner
    // printf("B | PID: %d, Cerco patner...\n", getpid());
    int foundMate = 0;
    while (!foundMate && !done) {
        int index = -1;

        if ((index = searchPatner(my_info, shdata)) >= 0) {
            // printf("B | PID: %d, patner ideale trovato: %d\n", getpid(), index);

            int pida = shdata->children_a[index].pid;
            foundMate = contact_patner(index, my_info, pida);

            if (foundMate == 1) {
                // printf("B | PID: %d, patner ha accettato: %d\n", getpid(), pida);
                send_msg_to_gestore(pida);
            } else {
                // printf("B | PID: %d, patner NON ha accettato: %d\n", getpid(), pida);
            }
        }
        usleep(50000);
    }
    // printf(" ---> CHILD B END | pid: %d <---\n", getpid());

    remove(pid_s);
    free(pid_s);
    shmdt(shdata);
    exit(EXIT_SUCCESS);
}

void send_msg_to_gestore(int pid_a) {
    // printf("B | PID: %d, contacting parent..\n", getpid());
    // open msg queue
    int key = getppid();
    int msgid = msgget(key, 0666 | IPC_CREAT);

    if (msgid == -1) {
        msgid = msgget(key, 0666 | IPC_CREAT);
        if (msgid == -1) {
            fprintf(stderr, "B | Could not create message queue.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Send message ..
    int mreturn= -1;
    struct msgbuf msg;

    msg.mtype = 1;
    msg.mtext.pid_sender = getpid();
    msg.mtext.pid_match = pid_a;

    // printf("A | PID: %d, Send message ...\n", getpid());

    if ((mreturn = msgsnd(msgid, &msg, sizeof(msg), IPC_NOWAIT)) == -1) {
        if (errno != EAGAIN) {
            fprintf(stderr, "B | errno: %d | Message could not be sended. Retry...\n", errno);
        }
        usleep(50000);//50 ms
        //try again
        if ((mreturn = msgsnd(msgid, &msg, sizeof(msg), IPC_NOWAIT)) == -1) {
            fprintf(stderr, "B | Message could not be sended.\n");
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

bool isForMe(unsigned long gen_a, unsigned long gen_b) {
    if (gen_a % gen_b == 0) {
        return true;
    }

    if (mcd(gen_a, gen_b) >= 10) {
        return true;
    }

    return false;
}

int searchPatner(struct individuo my_info, struct shared_data *shdata) {
    int patnerIndex = -1;
    int maxMCD = 0;

    for (int i = 0; i < shdata->cur_idx; i++) {
        if (!shdata->children_a[i].alive) continue;

        struct child_a child = shdata->children_a[i];

        // perfect match
        if (child.genoma % my_info.genoma == 0) {
            return i;
        }
        int mcd_value = mcd(child.genoma, my_info.genoma);

        // search max mcd
        if (mcd_value > maxMCD) {
            patnerIndex = i;
            maxMCD = mcd_value;
        }
    }

    return patnerIndex;
}

int contact_patner(int index, struct individuo my_info, int pida) {
    struct child_a child;
    char *pida_s = calloc(sizeof(char), sizeof(pid_t));
    char *pid_s = calloc(sizeof(char), sizeof(pid_t));
    sprintf(pid_s, "%d_B", getpid());

    // access to the child semaphore
    sem_a_id = semget(pida, 1, IPC_CREAT | 0666);

    // print semaphore status
    int sem_val = semctl(sem_a_id, 0, GETVAL);
    // printf("B | pid %d | sem_a_id: %d | sem_val: %d\n", getpid(), sem_a_id, sem_val);

    // lock resource
    semop(sem_a_id, &sem_2_l, 1);
    // printf("B | pid %d | took control of %d semaphore\n", getpid(), sem_a_id);

    // skip dead child
    if (!isAlive(index)) {
        // printf("B | pid: %d | shdata->children_a[%d] NOT alive \n", getpid(), index);
        // unlock resource
        semop(sem_a_id, &sem_2_u, 1);
        return 0;
    }
    // printf("B |  pid: %d | shdata->children_a[%d] IS alive \n", getpid(), index);

    child.pid = shdata->children_a[index].pid;
    child.genoma = shdata->children_a[index].genoma;
    child.alive = shdata->children_a[index].alive;

    // Write message to A
    sprintf(pida_s, "%d_A", child.pid);
    // printf("B | pid: %d, shdata->children_a[i].pid A: %s\n", getpid(), pida_s);

    alarm(5);
    int fifo_a = -1;
    if ((fifo_a = open(pida_s, O_WRONLY)) == -1) {
        // unlock resource
        semop(sem_a_id, &sem_2_u, 1);
        return -1;
    }
    alarm(0);

    // printf("B | PID: %d | writing to A: start\n", getpid());
    char *my_msg = calloc(sizeof(char), 1024);
    int str_len = sprintf(my_msg, "%d,%s,%lu", getpid(), my_info.nome, my_info.genoma);
    write(fifo_a, my_msg, str_len);
    close(fifo_a);
    free(my_msg);
    // printf("B | PID: %d | writing to A: end\n", getpid());

    // read answer
    // printf("B | PID: %d | reading response from A...\n", getpid());
    int BUF_SIZE = 1024;
    char *readbuf = calloc(sizeof(char), BUF_SIZE);
    int fifo_b = -1;
    alarm(5);
    if ((fifo_b = open(pid_s, O_RDONLY)) == -1) {
        // unlock resource
        semop(sem_a_id, &sem_2_u, 1);
        return -1;
    }
    alarm(0);

    ssize_t num_bytes = read(fifo_b, readbuf, BUF_SIZE);
    close(fifo_b);

    // printf("B | PID: %d | A with pid %s has response: %s\n", getpid(), pida_s, readbuf);

    int foundMate = 0;
    if (strtol(readbuf, NULL, 10) == 1) {
        foundMate = 1;
        // printf("B | pid %d | foundMate \n", getpid());
        setDead(index);
    }

    free(pida_s);
    free(readbuf);
    semop(sem_a_id, &sem_2_u, 1);

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

void exit_handler(void) {
    remove(pid_s);
    semop(sem_shm_id, &sem_1_u, 1);
    semop(sem_a_id, &sem_2_u, 1);
}

void handle_termination(int signum) {
    extern int sem_shm_id, sem_a_id;
    // printf("B | pid: %d | SIGTERM recevived\n", getpid());
    done = 1;
}

void timeout_handler(int signum) {
    // printf("B | pid: %d | timeout ALARM recevived\n", getpid());
    done = 1;
}