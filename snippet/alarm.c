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

volatile int done =0;

void alarm_handler(int sig) {
    ///printf("alarm_handler: %d\n", alarm(3));
    if(done){
        alarm(0);
    }
    printf("alarm_handler: %d\n", alarm(1));
}

int main(int argc, char const *argv[])
{
    int n=0, fd;
    char buff[100];
    printf(" *** START ***\n");
    
    struct sigaction sa, sa_old;
    sa.sa_handler = &alarm_handler;
    //sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, &sa_old);

    alarm(1);
    n = read(fd, buff, 100);
    //alarm(0);
    
    printf("alarm(0)\n");
    
    if (n == 0)
        printf("timeout expired!\n");
    else 
        printf("data has been read!\n");

    while(1){
        printf("sleep 5 \n");
        sleep(5);
        done =1;
    }
    printf(" *** END ***\n");
    exit(EXIT_SUCCESS);
}
