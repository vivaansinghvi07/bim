#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

void exit_error(const char *msg) {
        printf("%s\n", msg);
        exit(1);
}

int num_len(int n) {
        int len = snprintf(NULL, 0, "%d", n);
        return len;
}

const char *num_to_str(int n) {
        char *buf = malloc(num_len(n) * sizeof(char) + 1);
        sprintf(buf, "%d", n);
        return buf;
}
