/*
 * Inspiration for this file and its implementation is Tsoding's [nob.h](https://github.com/tsoding/musializer/blob/master/src/nob.h)
 */

#include <stdlib.h>
#include <stddef.h>

#ifndef EDITOR_LIST
#define EDITOR_LIST

#define list_typedef(name, type)    \
        typedef struct {            \
                size_t cap;         \
                size_t len;         \
                type *items;        \
        } name

#define list_append(list, item)                                                                             \
        do {                                                                                                \
                if ((list).cap < ++(list).len) {                                                            \
                        (list).cap *= 2;                                                                    \
                        (list).items = realloc((list).items, (list).cap * sizeof(*(list).items));           \
                }                                                                                           \
                (list).items[(list).len - 1] = item;                                                        \
        } while (0)                                                                              

#define list_init(type, size)                                     \
        (type) {                                                  \
                (size),             /* capacity */                \
                0,                  /*  length  */                \
                malloc((size) * sizeof(*((type *) 0)->items))     \
        }           

#endif
