#include "list.h"
#include "str.h"

#ifndef EDITOR_BUF
#define EDITOR_BUF

list_typedef(dyn_contents, dyn_str);

typedef struct {
        dyn_str filename;
        dyn_contents lines;  // i believe it's okay to not use a pointer of this type
} file_buf;

list_typedef(buf_list, file_buf *);

file_buf *buf_open(const char *filename);

#endif // !EDITOR_BUF
