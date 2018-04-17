#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

extern char **environ;  /* declared extern, defined by the OS somewhere */

int main(int argc, char * argv[]) {
	unsigned int i;
	char * my_val;
    fprintf(stdout, "Salve, Sono il child A!\n");
	printf("Salve, Sono il child A!\n");

	// Print all environment variables
	for(i=0; environ[i]; i++) {
		printf("env_var[%d] = \"%s\"\n", i, environ[i]);
	}
	
	// Print the value of a variable specified by argv[1], if any
	if (argc > 1) {
		if ((my_val = getenv(argv[1]))) {
			printf("Env. var. \"%s\" is \"%s\"\n", argv[1], my_val);
		} else {
			printf("No environment variable %s\n", argv[1]);
		}
		return 0;
	}

	printf("No arguments passed!");
	exit(EXIT_SUCCESS);
}
