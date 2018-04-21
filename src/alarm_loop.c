#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>

const int BIRTH_DEATH = 3;
const int END_SIM = 10;

/* number of times #include <unistd.h>
the handle will run: */
volatile int breakflag = 0;
volatile int birthdeathflag = 0;
volatile int deathchild = 0;
volatile int totalchildren =0;

void handle(int sig) {
    if(breakflag > 0){
        --breakflag;
        --birthdeathflag;
        alarm(1);
    }
}

void handlechild(int sig) {
    --totalchildren;
    ++deathchild;
}

int main() {
    printf("OK, let's start: %li \n", time(NULL));

    breakflag = END_SIM;
    birthdeathflag = BIRTH_DEATH;

    signal(SIGALRM, handle);
    signal(SIGCHLD, handlechild);
    // randomize
    srand(time(NULL));

    for(int i=0; i<100000; i++){
        pid_t pid = fork();

        if(pid==0)
        {
            //printf("child pid: %d \n", getpid());
            sleep((lrand48()/5)+1);
            exit(EXIT_SUCCESS);
        }
        ++totalchildren;
    }

    while(breakflag) {
        alarm(1);
        printf("I'm working hard...\n");
        printf("%d seconds at end of the day \n", breakflag);
        printf("deathchild: %d \n", deathchild);

        if(birthdeathflag == 0){
            printf("** I have to kill one of my child ** \n");
            birthdeathflag = BIRTH_DEATH;
        }

        int childval=-1;
        int handledeath = deathchild;

        for(int i =0; i<handledeath;i++){
            int pid =  waitpid(-1,&childval, WNOHANG);
            if(pid>0){
                --deathchild;
                ++totalchildren;
                //printf("death child pid: %d | totalchildren: %d | childval: %d \n ", pid, totalchildren, childval);

            }
        }

        pause(); 
    }

    printf("totalchildren: %d \n",totalchildren);
    printf("End of the day:  %li \n", time(NULL));
    return 0;
}