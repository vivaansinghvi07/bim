#include "utils.h"
#include "list.h"
#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

dyn_str *dyn_str_from_string(const char *str) {
        uint64_t len = strlen(str);
        char *target_str = malloc(len * sizeof(char));
        memcpy(target_str, str, len);
        dyn_str *retval = malloc(sizeof(dyn_str));
        retval->cap = retval->len = len;
        retval->items = target_str;
        return retval;
}

void exit_error(const char *msg) {
        input_restore_tty();
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
