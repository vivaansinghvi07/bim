#include "list.h"

#include <stdarg.h>

list_typedef(__base_to_free, char);

/*
 * The var-arguments for this function MUST BE (list *);
 * It frees the memory allocated with malloc.
 */
void free_list_items(int n, ...) {
        va_list list_args; 
        va_start(list_args, n);
        for (int i = 0; i < n; ++i) {
                __base_to_free *list = va_arg(list_args, __base_to_free *);
                free(list->items);
        }
        va_end(list_args);
}
