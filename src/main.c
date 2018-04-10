#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>

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

void run_parent(pid_t my_pid, pid_t my_ppid, pid_t value);
void run_child(struct individuo *figlio);

int main()
{
	int i;
	pid_t my_pid, my_ppid, value;
	int num_of_process = INIT_PEOPLE;
	char name;
	struct individuo *figlio;

	// check popolation limit
	if (num_of_process > 20)
	{
		printf("Warning: Popolazione iniziale maggiore di 20, il massimo consentito.");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < INIT_PEOPLE; i++)
	{
		printf("Name: %c | ", name);
		printf("LOOP: %d\n", i + 1);

		// anagrafica child
		figlio = gen_individuo();

		// creazione child
		switch (value = fork())
		{
		case -1:
			/* Handle error */
			fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
			break;

		case 0:
			/* Perform actions specific to child */
			run_child(figlio);
			exit(EXIT_SUCCESS);
			break;

		default:
			/* Perform actions specific to parent */
			run_parent(my_pid, my_ppid, value);
			break;
		}
		sleep(1);
	}
	sleep(10);
	exit(EXIT_SUCCESS);
}

unsigned long gen_genoma()
{
	sleep(0.001);
	srand(time(0));
	return (unsigned long)rand() % (GENES + 1 - 2) + 2;
}

char gen_name()
{
	sleep(0.001);
	srand(time(0));
	return (char)rand() % (MAX_CHAR + 1 - MIN_CHAR) + MIN_CHAR;
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

void run_parent(pid_t my_pid, pid_t my_ppid, pid_t value)
{
	my_pid = getpid();
	my_ppid = getppid();
	printf("* PARENT: PID=%d, PPID=%d, fork_value=%d\n",
		   my_pid, my_ppid, value);
}

void run_child(struct individuo *figlio)
{
	printf("Hi, I'm the child with the name %c\n", figlio->nome);
	execve(figlio->tipo == 'A' ? "./exec/child_a.exe" : "./exec/child_b.exe", NULL, NULL);
}