#include <sys/ioctl.h>
#include "buf.h"
#include "state.h"

#ifndef EDITOR_DISPLAY
#define EDITOR_DISPLAY

int store_cursor_pos(int *y, int *x);
struct winsize get_window_size(void);
int display_buffer(file_buf *buffer, editor_mode mode, display_state_t *state);
void fill_ansi_color_table(void);
#endif // !EDITOR_DISPLAY
