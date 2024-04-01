#include "list.h"
#include "utils.h"

#ifndef EDITOR_BUF
#define EDITOR_BUF

list_typedef(dyn_contents, dyn_str);

typedef struct {
        // THIS IS ONE-INDEXED!!!
        size_t cursor_line;  
        size_t cursor_col;
        size_t screen_top_line;

        const char *filename;
        dyn_contents lines;  // i believe it's okay to not use a pointer of this type
} file_buf;

list_typedef(buf_list, file_buf *);

void buf_free_list(buf_list *buffers);
void buf_free(file_buf *buf);
void buf_save(const file_buf *buf);
file_buf *buf_open(const char *filename);

#endif // !EDITOR_BUF
