/*
 * Inspiration for this file and its implementation is Tsoding's [nob.h](https://github.com/tsoding/musializer/blob/master/src/nob.h)
 */

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>

#ifndef EDITOR_LIST
#define EDITOR_LIST

// declares a new list type called <type> containing an array of items of <type>
#define list_typedef(name, type)    \
        typedef struct {            \
                size_t cap;         \
                size_t len;         \
                type *items;        \
        } name

// add one element, <item> to <list>
#define list_append(list, item)                                                                             \
        do {                                                                                                \
                if ((list).cap < ++(list).len) {                                                            \
                        (list).cap *= 2;                                                                    \
                        (list).items = realloc((list).items, (list).cap * sizeof(*(list).items));           \
                        assert((list).items && "Needs more memory");                                        \
                }                                                                                           \
                (list).items[(list).len - 1] = item;                                                        \
        } while (0)                                                                              

// initialize a new list of type <type> and size <size> 
#define list_init(type, size)                                     \
        (type) {                                                  \
                (size),             /* capacity */                \
                0,                  /*  length  */                \
                malloc((size) * sizeof(*((type *) 0)->items))     \
        }           

// assure there is enough space for <space> more items
#define list_create_space(list, space)                                                                      \
        do {                                                                                                \
                while ((list).cap + space < (list).len) {                                                   \
                        (list).cap *= 2;                                                                    \
                        (list).items = realloc((list).items, (list).cap * sizeof(*(list).items));           \
                }                                                                                           \
        } while(0)

void free_list_items(int n, ...);

#endif
