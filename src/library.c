#include "library.h"

unsigned int rand_interval(unsigned int min, unsigned int max) {
    // reference https://stackoverflow.com/a/17554531
    unsigned int r;
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

unsigned long rand_interval_lu(unsigned long min, unsigned long max) {
    // reference https://stackoverflow.com/a/17554531
    unsigned long r;
    const unsigned long range = 1 + max - min;
    const unsigned long buckets = RAND_MAX / range;
    const unsigned long limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do {
        r = lrand48();
    } while (r >= limit);

    return min + (r / buckets);
}

int len_of(int value) {
    int l = 1;

    while (value > 9) {
        l++;
        value /= 10;
    }

    return l;
}

unsigned long mcd(unsigned long a, unsigned long b) {
    int remainder;
    while (b != 0) {
        remainder = a % b;

        a = b;
        b = remainder;
    }
    return a;
}

const char *getfield(char *line, int num) {
    const char *tok;
    for (tok = strtok(line, ";");
         tok && *tok;
         tok = strtok(NULL, ";\n")) {
        if (!--num)
            return tok;
    }
    return NULL;
}