/*
 * Inspiration for this file and its implementation is Tsoding's [nob.h](https://github.com/tsoding/musializer/blob/master/src/nob.h)
 */

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
                if ((list).cap < ++(list).n) {                                                              \
                        (list).cap *= 2;                                                                    \
                        (list).items = realloc((list).items, (list).cap * sizeof(*(list).items));           \
                }                                                                                           \
                (list).items[(list).n - 1] = item;                                                          \
        }                                                                                                   \
        while (0);                                                                              

#define list_init(type, size)                                                 \
        {                                                                     \
                .items = malloc((size) * sizeof(*((type *) 0)->items)),       \
                .cap = (size), .n = 0                                         \
        }

#define list_test(list) do { printf("%lu\n", sizeof(*(list.items))); } while(0);

#endif
