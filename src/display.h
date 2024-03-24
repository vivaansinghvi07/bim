#include <sys/ioctl.h>
#include "buf.h"
#include "state.h"

#ifndef EDITOR_DISPLAY
#define EDITOR_DISPLAY

typedef struct {
        rgb_t rgb;
        uint8_t style;
} ansi_code_t;

int store_cursor_pos(int *y, int *x);
struct winsize get_window_size(void);
char *get_displayed_buffer_string(const file_buf *buffer, const editor_mode mode, const display_state_t *state);
void display_buffer(const file_buf *buffer, const editor_mode mode, const display_state_t *state);
void fill_ansi_color_table(void);

#endif // !EDITOR_DISPLAY
