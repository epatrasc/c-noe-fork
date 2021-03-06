#include "library.h"

bool isGood(unsigned long gen_a, unsigned long gen_b);

void send_msg_parent(pid_t pid_b);

void handle_termination(int signum);

void timeout_handler(int signum);

void exit_handler(void);

// Global variable
int sem_id, fifo_a = -1, selection_level = 10;
char *pid_b, *pid_s;

// handle termination
volatile sig_atomic_t done = 0;

int main(int argc, char *argv[]) {
    struct individuo my_info;
    struct sigaction action, sa, sa_old;

    // printf("\n ---> CHILD A START | pid: %d | argc: %d <---\n", getpid(), argc);
    if (argc < 3) {
        // printf("A | I need name and genoma input from argv. \n");
        // for (int i = 0; i < argc; i++) {
        //     printf("A | arg[%d]: %s \n ", i, argv[i]);
        // }
        exit(EXIT_FAILURE);
    }

    memset(&action, 0, sizeof(struct sigaction));

    // mask SIGTERM
    action.sa_handler = &handle_termination;
    action.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &action, NULL);
    sigemptyset(&action.sa_mask);

    // mask timeout alarm
    sa.sa_handler = &timeout_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, &sa_old);

    atexit(exit_handler);

    //creat sem 
    sem_id = semget(getpid(), 1, IPC_CREAT | 0666);
    // printf("A | pid %d | sem_id: %d\n", getpid(), sem_id);
    semctl(sem_id, 0, SETVAL, 1);
    TEST_ERROR;


    my_info.nome = argv[1];
    my_info.tipo = 'A';
    my_info.genoma = (unsigned long) strtol(argv[2], NULL, 10);

    // printf("A | my_info.nome: %s \n", my_info.nome);
    // printf("A | my_info.tipo: %c \n", my_info.tipo);
    // printf("A | my_info.genoma: %lu \n", my_info.genoma);

    //create fifo
    pid_s = calloc(sizeof(char), sizeof(pid_t));
    sprintf(pid_s, "%d_A", getpid());
    mkfifo(pid_s, S_IRUSR | S_IWUSR);

    int BUF_SIZE = 1024;
    ssize_t num_bytes;
    pid_b = calloc(sizeof(char), sizeof(pid_t));

    // printf("A | waiting for type b request...\n");
    while (!done) {
        alarm(5);
        if ((fifo_a = open(pid_s, O_RDONLY)) < 0) {
            usleep(10000);
            continue;
        }

        alarm(0);
        
        // printf("A | fifo_a opened\n");

        char *readbuf = calloc(sizeof(char), BUF_SIZE);
        if ((num_bytes = read(fifo_a, readbuf, BUF_SIZE)) <= 0) {
            // printf("A | read(fifo_a, readbuf, BUF_SIZE)t: %s\n", readbuf);
            close(fifo_a);
            continue;
        }

        close(fifo_a);

        // printf("A | recieved request: %s\n", readbuf);
        char *nome_b = NULL, *token = NULL;
        unsigned long genoma_b;

        token = strsep(&readbuf, ",");
        sprintf(pid_b, "%s_B", token);
        token = strsep(&readbuf, ",");
        nome_b = calloc(sizeof(char), strlen(token));
        sprintf(nome_b, "%s", token);
        genoma_b = (unsigned long) strtol(strsep(&readbuf, ","), NULL, 10);

        // printf("A | pid_b:%s\n", pid_b);
        // printf("A | nome_b:%s\n", nome_b);
        // printf("A | genoma_b:%lu\n", genoma_b);

        //evaluate candidate
        int answer = (int) isGood(my_info.genoma, genoma_b);

        // printf("A | accept pid %s? %d\n", pid_b, answer);
        // send response
        int fifo_b = open(pid_b, O_WRONLY);
        if (fifo_b < 0) {
            usleep(10000);
            fifo_b = open(pid_b, O_WRONLY);
        }

        char *my_msg = calloc(sizeof(char), 2);
        int str_len = sprintf(my_msg, "%d", answer);

        // printf("A | fifo_b: %d\n", fifo_b);
        write(fifo_b, my_msg, str_len);
        close(fifo_b);

        // if positive inform parent
        if (answer) {
            // printf("A | PID: %d, Message sent \n", getpid());
            send_msg_parent((int) strtol(pid_b, (char **) NULL, 10));
        }
    }

    free(pid_b);
    free(pid_s);

    // printf(" ---> CHILD A END | pid: %d <---\n", getpid());

    exit(EXIT_SUCCESS);
}

bool isGood(unsigned long gen_a, unsigned long gen_b) {
    if (gen_a % gen_b == 0) return true;
    if (mcd(gen_a, gen_b) >= selection_level) return true;

    selection_level--;

    return false;
}

void send_msg_parent(int pid_b) {
    // printf("A | PID: %d, contacting parent...\n", getpid());

    // open msg queue
    int key, mask, msgid;

    key = getppid();
    mask = 0666;
    msgid = msgget(key, mask);

    if (msgid == -1) {
        msgid = msgget(key, mask | IPC_CREAT);
        if (msgid == -1) {
            fprintf(stderr, "A | Could not create message queue.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Send message to the parent ...
    int mreturn= -1;
    struct msgbuf msg;

    msg.mtype = 1;
    msg.mtext.pid_sender = getpid();
    msg.mtext.pid_match = pid_b;

    // printf("A | PID: %d, Send message ...\n", getpid());

    if ((mreturn = msgsnd(msgid, &msg, sizeof(msg), IPC_NOWAIT)) == -1) {
        if (errno != EAGAIN) {
           fprintf(stderr, "A | errno: %d | Message could not be sended. Retry...\n", errno);
        }
        usleep(50000);//50 ms
        //try again
        if ((mreturn = msgsnd(msgid, &msg, sizeof(msg), IPC_NOWAIT)) == -1) {
            fprintf(stderr, "A | Message could not be sended.\n");
        }
    }
}

void handle_termination(int signum) {
    // printf("A | pid: %d | SIGTERM recevived\n", getpid());
    done = 1;
}

void timeout_handler(int signum) {
    // printf("A | pid: %d | timeout ALARM recevived\n", getpid());
    done = 1;
}

void exit_handler(void) {
    remove(pid_s);
    semctl(sem_id, 0, IPC_RMID, 0);
}