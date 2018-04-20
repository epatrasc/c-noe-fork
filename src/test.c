#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const int MIN_CHAR = 65; // A
const int MAX_CHAR = 90; // Z

unsigned int rand_interval(unsigned int min, unsigned int max) {
    // reference https://stackoverflow.com/a/17554531
    int r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do {
        r = lrand48();
    } while (r >= limit);

    return min + (r / buckets);
}

char* gen_name(char* name) {
    int new_length = (int) (strlen(name) + 2);
    char* new_name = (char*) calloc(sizeof(char), new_length);
    char* buffer = calloc(sizeof(char),2);

    memset(new_name, '\0', new_length);

    sprintf(buffer, "%c",(char)rand_interval(MIN_CHAR, MAX_CHAR));
    strcat(new_name, name);
    strcat(new_name, buffer);

    return new_name;
}



int main()
{
    int n=10;
    char* str1="HELLO";
    char* str2;
    char* str4;
    char  str3[10];

    printf("RUNNING test.c \n");
    for(int i=0; i<11; i++){
        str3[i] = 'A';
    }

//    strcat(str1, str2);
//    str1 = (char*) calloc(sizeof(char), n+1);
//    str4= (char*) calloc(sizeof(char), 2);
//    memset(str1, '\0', 11);
//    memset(str4, '\0', 2);
//    sprintf(str4,"%c",(char)rand_interval(MIN_CHAR, MAX_CHAR));
//    strcpy(str1, "ABCDGFG");
//    strcat(str1, str4);
    printf("gen name: %s \n", gen_name("b"));


//    printf("string1: %s \n",str1);
//    printf("string4: %s \n",str4);
//    printf("string2: %s \n",str2);
//    printf("string3: %s \n",str3);
}