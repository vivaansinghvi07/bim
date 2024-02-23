#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

void exit_error(const char *msg) {
        printf("%s\n", msg);
        exit(1);
}
