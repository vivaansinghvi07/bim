#include <sys/ioctl.h>
#include "buf.h"

#ifndef EDITOR_DISPLAY
#define EDITOR_DISPLAY

int store_cursor_pos(int *y, int *x);
struct winsize get_window_size(void);
int display_buffer(file_buf *buffer);

#endif // !EDITOR_DISPLAY
