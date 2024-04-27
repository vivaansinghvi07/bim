#ifndef EDITOR_BUF
#define EDITOR_BUF

#include "list.h"
#include "utils.h"

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
} buf_t;

list_typedef(buf_list, buf_t *);

void buf_free_list(buf_list *buffers);
void buf_free(buf_t *buf);
void buf_save(const buf_t *buf);
void buf_init(buf_t *buffer, const char *filename);
void buf_fill_files_view(buf_t *buf);
buf_t *buf_open(const char *filename, const int tab_width);

#endif // !EDITOR_BUF
