#include "list.h"
#include "utils.h"

#ifndef EDITOR_BUF
#define EDITOR_BUF

#define MIN_NEW_LINE_LEN 128

list_typedef(dyn_contents, dyn_str);

typedef struct {

        /* 
         * THESE ARE ONE-INDEXED!!!
         *   <cursor_line> represents what line number the cursor is on. 
         *   <screen_top_line> represents the line number at the top of the screen.
         *   <cursor_col> represents what column number the cursor is on.
         *   <screen_left_col> represents the column number at the left of the screen.
         */
        size_t cursor_line;  
        size_t screen_top_line;
        size_t cursor_col;
        size_t screen_left_col;

        const char *filename;
        dyn_contents lines;
} file_buf;

list_typedef(buf_list, file_buf *);

void buf_free_list(buf_list *buffers);
void buf_free(file_buf *buf);
void buf_save(const file_buf *buf);
file_buf *buf_open(const char *filename, const int tab_width);

#endif // !EDITOR_BUF
