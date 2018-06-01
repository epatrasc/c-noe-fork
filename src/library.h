#ifndef __LIBRARY_H__
#define __LIBRARY_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>

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

#define DEBUG_LINE                                \
    printf("P | DEBUG %s:%d: PID=%5d\n", __FILE__, __LINE__, getpid());

struct individuo {
    char tipo;
    char *nome;
    unsigned long genoma;
    pid_t pid;
    bool alive;
};

struct message {
    pid_t pid_sender;
    pid_t pid_match;
};

struct msgbuf {
    long mtype;             /* message type, must be > 0 */
    struct message mtext;    /* message data */
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
    int num_birth_death;
    int num_match_born;
    struct popolazione *societa;
};

unsigned int rand_interval(unsigned int min, unsigned int max);

unsigned long rand_interval_lu(unsigned long min, unsigned long max);

int len_of(int x);

unsigned long mcd(unsigned long a, unsigned long b);

const char *getfield(char *line, int num);

#endif   // __LIBRARY_H__#