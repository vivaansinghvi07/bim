#include <sys/ioctl.h>
#include "buf.h"

#ifndef EDITOR_DISPLAY
#define EDITOR_DISPLAY

typedef enum {
        NORMAL, FILES, EDIT  // files is plural b/c FILE is a kw
} editor_mode;

typedef enum {
        HIGH_ALPHA, HIGH_RANDOM, HIGH_GRADIENT
} highlighting_mode;
typedef enum {
        GRADIENT_BLUE, GRADIENT_RED, GRADIENT_PURPLE,
        GRADIENT_YELLOW, GRADIENT_CYAN, GRADIENT_GREEN
} gradient_color;

typedef struct {
        highlighting_mode syntax_mode;
        gradient_color gradient_color;
} display_state_t;

int store_cursor_pos(int *y, int *x);
struct winsize get_window_size(void);
int display_buffer(file_buf *buffer, editor_mode mode, display_state_t *state);
void fill_ansi_color_table(void);
#endif // !EDITOR_DISPLAY
