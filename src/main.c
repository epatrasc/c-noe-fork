#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

const int MAX_POPOLATION = 20;
const int INIT_PEOPLE = 5;
const char CHILD_A_PATH [] = {"./exec/child_a.exe"};
const char CHILD_B_PATH []= {"./exec/child_b.exe"};

struct individuo
{
	char tipo;
	char nome [10];
	unsigned long genoma;
};

char roll_type()
{
	srand(time(0));
	return rand() % 2 ? 'A' : 'B';
}

void run_parent(pid_t my_pid, pid_t my_ppid, pid_t value)
{
	my_pid = getpid();
	my_ppid = getppid();
	printf("* PARENT: PID=%d, PPID=%d, fork_value=%d\n",
		   my_pid, my_ppid, value);
}

void run_child(char name)
{
    printf("run_child: %c\n", name);
    execve(name == 'A' ? CHILD_A_PATH : CHILD_B_PATH, NULL, NULL);
}

int main()
{
	int i;
	pid_t my_pid, my_ppid, value;
	int num_of_process = INIT_PEOPLE;
	char name = 'A';

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
		switch (value = fork())
		{
		case -1:
			/* Handle error */
			fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
			break;

		case 0:
			/* Perform actions specific to child */
            printf("CHILD %c is running...\n", name);
			run_child(name);
			exit(EXIT_SUCCESS);
			break;

		default:
			/* Perform actions specific to parent */
			run_parent(my_pid, my_ppid, value);
			name = roll_type();
			break;
		}
		sleep(1);
	}
	sleep(10);
	exit(EXIT_SUCCESS);
}
