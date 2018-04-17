#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main()
{
    char* argv[] = {"test", NULL};
    char* envp[] = {"env=1","env2=2", NULL};

    fprintf(stdout, "Salve, Sono il processo test!\n");
    execve("./exec/child_a.exe", argv, envp);
}