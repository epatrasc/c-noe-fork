// Example of message queue in C.
// For educational purposes only.
// Author: Vaclav Bohac

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


#define PRIMES 4


int main ( void )
{
    int key, mask, msgid;

    key = getuid();
    mask = 0666;
    msgid = msgget(key, mask);
    if (msgid == -1) {
        msgid = msgget(key, mask | IPC_CREAT);
        if (msgid == -1) {
            fprintf(stderr, "Could not create message queue.\n");
            exit(EXIT_FAILURE);
        }

        // Send messages ...
        int ret, prime, j, i = 3, msg[2] = {PRIMES, 0};
        while (1) {
            prime = 1;
            for (j = 2; j < i; j++) {
                if ((i % j) == 0) {
                    prime = 0;
                    break;
                }
            }
            if (prime == 0) {
                i++;
                continue;
            }
            msg[1] = i;
            ret = msgsnd(msgid, msg, sizeof(int), IPC_NOWAIT);
            if (ret == -1) {
                if (errno != EAGAIN) {
                    fprintf(stderr, "Message could not be sended.\n");
                    exit(EXIT_FAILURE);
                }
                usleep(50000);
                if (msgsnd(msgid, msg, sizeof(int), 0) == -1) {
                    fprintf(stderr, "Message could not be sended.\n");
                    exit(EXIT_FAILURE);
                }
            }
            i++;
        }
    }

    int rcv[2];
    while (1) {
        if (msgrcv(msgid, &rcv, sizeof(int), 0, IPC_NOWAIT) == -1) {
            if (errno != ENOMSG) {
                fprintf(stderr, "Message could not be received.\n");
                exit(EXIT_FAILURE);
            }
            usleep(50000);
            if (msgrcv(msgid, &rcv, sizeof(int), 0, 0) == -1) {
                fprintf(stderr, "Message could not be received.\n");
                exit(EXIT_FAILURE);
            }
        }
        printf("%d, ", rcv[1]);
    }

    return EXIT_SUCCESS;
}