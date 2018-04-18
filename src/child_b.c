#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

extern char **environ;  /* declared extern, defined by the OS somewhere */

int main(int argc, char * argv[]) {
	unsigned int i;
	char * my_val;

	printf("Salve, Sono il child B!\n");

	// Print all environment variables
	for(i=0; environ[i]; i++) {
		printf("env_var[%d] = \"%s\"\n", i, environ[i]);
	}
	
	// Print the value of a variable specified by argv[1], if any
	if (argc == 1) {
		printf("Env. var: %s \n", argv[0]);
		return 0;
	}

	printf("No arguments passed!");

	return 0; 
}
