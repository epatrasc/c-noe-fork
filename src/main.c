#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

const int MAX_POPOLATION = 20;
const int INIT_PEOPLE = 20;

void run_parent(pid_t my_pid, pid_t my_ppid, pid_t value)
{
	my_pid = getpid();
	my_ppid = getppid();
	printf("PARENT: PID=%d, PPID=%d, fork_value=%d\n",
		   my_pid, my_ppid, value);
}

void run_child(pid_t my_pid, pid_t my_ppid, pid_t value)
{
	my_pid = getpid();
	my_ppid = getppid();
	printf("CHILD:  PID=%d, PPID=%d, fork_value=%d\n",
		   my_pid, my_ppid, value);
	exit(EXIT_SUCCESS);
}

int main()
{
	int i;
	pid_t my_pid, my_ppid, value;
	int num_of_process = INIT_PEOPLE;

	// check popolation limit
	if (num_of_process > 20)
	{
		printf("Warning: Popolazione iniziale maggiore di 20, il massimo consentito.");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < INIT_PEOPLE; i++)
	{
		printf("LOOP: %d", i+1);
		switch (value = fork())
		{
		case -1:
			/* Handle error */
			fprintf(stderr, "Error #%03d: %s\n", errno, strerror(errno));
			break;

		case 0:
			/* Perform actions specific to child */
			run_child(my_pid, my_ppid, value);
			break;

		default:
			/* Perform actions specific to parent */
			run_parent(my_pid, my_ppid, value);
			break;
		}
	}

	exit(EXIT_SUCCESS);
}
