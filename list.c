#include <stdlib.h>
#include <stddef.h>

#ifndef LIST
#define LIST 

#define typedef_list(name, type)    \
        typedef struct {            \
                size_t cap;         \
                size_t n;           \
                type *items;        \
        } name;                  

#define list_append(list, item)                                                                             \
        do {                                                                                                \
                if ((list).cap < ++(list).n) {                                                            \
                        (list).cap *= 2;                                                                   \
                        (list).items = realloc((list).items, (list).cap * sizeof((list).items[0]));     \
                }                                                                                           \
                (list).items[(list).n - 1] = item;                                                       \
        }                                                                                                   \
        while (0);                                                                              

#define list_init(list, size)                                                               \
        do {                                                                                \
                (list).items = malloc(size * sizeof(*(list).items));   /* problem line */   \
                (list).cap = size;                                                          \
        } while (0);

#define list_test(list) do { printf("%lu\n", sizeof(*(list.items))); } while(0);

#endif
