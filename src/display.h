#include <sys/ioctl.h>
#include "buf.h"

#ifndef EDITOR_DISPLAY
#define EDITOR_DISPLAY

typedef enum {
        NORMAL, FILES, EDIT  // files is plural b/c FILE is a kw
} editor_mode;

typedef enum {
        ALPHA, RANDOM
} highlighting_mode ;

int store_cursor_pos(int *y, int *x);
struct winsize get_window_size(void);
int display_buffer(file_buf *buffer, editor_mode mode, highlighting_mode syntax_mode);

#endif // !EDITOR_DISPLAY
