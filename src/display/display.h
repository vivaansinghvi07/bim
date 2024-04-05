#include <sys/ioctl.h>
#include "../buf.h"
#include "../state.h"

#ifndef EDITOR_DISPLAY
#define EDITOR_DISPLAY

// the standardized codes for this project will be as below:
// \033[0;38;2;XXX;XXX;XXX;XXm  <-- this is 24 characters long
#define ANSI_ESCAPE_LEN 24
#define ANSI_COLOR_FORMAT "\033[0;38;2;%03d;%03d;%03d;%02dm"
#define ANSI_STYLE_VARIATION 3  // affects the get_ansi_style function

typedef struct {
        rgb_t rgb;
        uint8_t style;
} ansi_code_t;

void display_by_mode(const editor_state_t *state);
struct winsize get_window_size(void);
char *get_displayed_buffer_string(const editor_state_t *state);
char *get_bottom_bar(const int W, const editor_state_t *state);
void display_buffer(const editor_state_t *state);
void fill_ansi_color_table(void);
void increment_gradient(editor_state_t *state);
void decrement_gradient(editor_state_t *state);

#endif // !EDITOR_DISPLAY
