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

#define die(e)                      \
	do                              \
	{                               \
		fprintf(stderr, "%s\n", e); \
		exit(EXIT_FAILURE);         \
	} while (0);

// Input arguments;
const int INIT_PEOPLE = 5; // number of inital children
const unsigned long GENES = 1000000;
const unsigned int BIRTH_DEATH = 1;   //seconds
const unsigned int SIM_TIME = 1 * 60; //seconds

const int MAX_POPOLATION = 20;
const int MIN_CHAR = 65; // A
const int MAX_CHAR = 90; // Z

struct individuo
{
	char tipo;
	char *nome;
	unsigned long genoma;
};

unsigned long gen_genoma();
char gen_name();
char gen_type();
struct individuo *gen_individuo();

void run_parent(pid_t gestore_pid, pid_t pg_pid);
void run_child(struct individuo *figlio);
unsigned int rand_interval(unsigned int min, unsigned int max);
static void wake_up_process(int signo);

int main()
{
	int i;
	pid_t gestore_pid, pg_pid, child_pid;
	struct individuo *figlio;
	int population = INIT_PEOPLE;
	pid_t pid_array[population];

	// check popolation limit
	if (population > 20)
	{
		printf("Warning: Popolazione iniziale maggiore di 20, il massimo consentito.");
		exit(EXIT_FAILURE);
	}

	// pid gestore, pid padre gestore
	gestore_pid = getpid();
	pg_pid = getppid();

	run_parent(gestore_pid, pg_pid);

	//generate population
	for (i = 0; i < INIT_PEOPLE; i++)
	{
		printf("\nchild number: %d", i + 1);

		// generate child
		figlio = gen_individuo();
		child_pid = fork();

		/* Handle error create child*/
		if(child_pid < 0){
			die(strerror(errno));
		}

		/* Perform actions specific to child */
		if(child_pid == 0){
            signal(SIGUSR1, wake_up_process);
			printf("** CHILD %d, my parent is %d\n", getpid(), getppid());
			pause();
			run_child(figlio);
			exit(EXIT_SUCCESS);
		}
		
		/* Perform actions specific to parent */
        pid_array[i] = child_pid;
	}

	// make sure child set the signal handler
	sleep(1);

	// send signal to wake up all the children
	for (int i = 0; i < population; i++){
		printf("sending the signal SIGUSR1 to the child %d...\n", pid_array[i]);
        kill(pid_array[i], SIGUSR1);
	}

    for (int i = 0; i < population; i++){
        wait(NULL); 
	}
		
	exit(EXIT_SUCCESS);
}

unsigned long gen_genoma()
{
	return (unsigned long)rand_interval(2, GENES);
}

char gen_name()
{
	return (char)rand_interval(MIN_CHAR, MAX_CHAR);
}

char gen_type()
{
	srand(time(0));
	return rand() % 2 ? 'A' : 'B';
}

struct individuo *gen_individuo()
{
	struct individuo *figlio;

	figlio = malloc(sizeof(*figlio));

	figlio->tipo = gen_type();
	figlio->nome = gen_name();
	figlio->genoma = gen_genoma();

	return figlio;
}

void run_parent(pid_t gestore_pid, pid_t pg_pid)
{
	printf("* PARENT: PID=%d, PPID=%d\n", gestore_pid, pg_pid);
}

void run_child(struct individuo *figlio)
{
	printf("\nHi, I'm the child with the name %c\n", figlio->nome);
	printf("My type is:  %c\n", figlio->tipo);
	printf("My genoma is:  %ul\n", figlio->genoma);

	int error = execve(figlio->tipo == 'A' ? "./exec/child_a.exe" : "./exec/child_b.exe", NULL, NULL);
	if (error == -1)
	{
		die(strcat("Errore execve child ", figlio->nome));
	}
}

/****** UTILS ******/
static void wake_up_process(int signo) {
	printf("wake_up_process signo: %d\n", signo);
	if(signo == SIGUSR1){
		printf("process %d as received SIGUSR1\n", getpid());
	}
}

unsigned int rand_interval(unsigned int min, unsigned int max)
{
	// reference https://stackoverflow.com/a/17554531
	int r;
	const unsigned int range = 1 + max - min;
	const unsigned int buckets = RAND_MAX / range;
	const unsigned int limit = buckets * range;

	srand(time(0));
	/* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
	do
	{
		r = rand();
	} while (r >= limit);

	return min + (r / buckets);
}