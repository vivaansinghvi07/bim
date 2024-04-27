/*
 * Inspiration for this file and its implementation is Tsoding's [nob.h](https://github.com/tsoding/musializer/blob/master/src/nob.h)
 */

#ifndef EDITOR_LIST
#define EDITOR_LIST

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

// declares a new list type called <type> containing an array of items of <type>
// <len> and <cap> have been changed from `size_t` to allow for overflow-free operations
//   because some part of the program that essentially did `0 /* of size_t */ - 1` was breaking
#define list_typedef(name, type)     \
        typedef struct {             \
                int64_t cap;         \
                int64_t len;         \
                type *items;         \
        } name

// add one element, <item> to <list>
#define list_append(list, item)                                                                             \
        do {                                                                                                \
                if ((list).cap < ++(list).len) {                                                            \
                        (list).cap *= 2;                                                                    \
                        (list).items = realloc((list).items, (list).cap * sizeof(*(list).items));           \
                        assert((list).items && "Needs more memory");                                        \
                }                                                                                           \
                (list).items[(list).len - 1] = (item);                                                      \
        } while (0)                                                                              

// initialize a new list of type <type> and size <size> 
#define list_init(type, size)                                     \
        (type) {                                                  \
                (size),             /* capacity */                \
                0,                  /*  length  */                \
                malloc((size) * sizeof(*((type *) 0)->items))     \
        }           

// insert a value into a list, at the index <index>, pushing all values forward 
#define list_insert(list, index, value)                                                                   \
        do {                                                                                              \
                list_create_space((list), 1);                                                             \
                memcpy((list).items + (index) + 1, (list).items + (index),                                \
                       ((list).len - (index) - 1) * sizeof(*(list).items));                               \
                (list).items[(index)] = (value);                                                          \
        } while(0)

// remove a value from a list at the index <index>, pulling all values backward
#define list_pop(list, index)                                                                               \
        do {                                                                                                \
                if ((list).len) {                                                                           \
                        memcpy((list).items + (index), (list).items + (index) + 1,                          \
                               ((list).len - (index)) * sizeof(*(list).items));                             \
                        --(list).len;                                                                       \
                }                                                                                           \
        } while (0)

// assure there is enough space for <space> more items and add <space> to the length of the list
#define list_create_space(list, space)                                                                      \
        do {                                                                                                \
                while ((list).len + (space) > (list).cap) {                                                 \
                        (list).cap *= 2;                                                                    \
                        (list).items = realloc((list).items, (list).cap * sizeof(*(list).items));           \
                }                                                                                           \
                (list).len += (space);                                                                      \
        } while(0)

void free_list_items(int n, ...);

#endif
