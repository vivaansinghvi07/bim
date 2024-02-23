/*
* I don't know if this is necessary, or even good practice, but here are some functions
* that deal with the custom `dyn_str` dynamic string class I made.
*/

#include "list.h"
#include "str.h"
#include <string.h>
#include <stdint.h>

dyn_str *dyn_str_from_string(const char *str) {
        uint64_t len = strlen(str) + 1;
        char *target_str = malloc(len * sizeof(char));
        memcpy(target_str, str, len);
        dyn_str *retval = malloc(sizeof(dyn_str));
        retval->cap = retval->len = len;
        retval->items = target_str;
        return retval;
}
