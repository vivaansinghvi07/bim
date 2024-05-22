#ifndef EDITOR_DISPLAY
#define EDITOR_DISPLAY

#include "../buf.h"
#include "../state.h"

#include <sys/ioctl.h>

// the standardized codes for this project will be as below:
// \033[0;38;2;XXX;XXX;XXX;XXm  <-- this is 24 characters long
#define ANSI_ESCAPE_LEN 24
#define ANSI_COLOR_FORMAT "\033[0;38;2;%03d;%03d;%03d;%02dm"
#define ANSI_STYLE_VARIATION 3  // affects the get_ansi_style function

typedef struct {
        rgb_t rgb;
        uint8_t style;
} ansi_code_t;  // todo - test if attribute packed makes this faster or not

typedef struct {
        size_t start;
        size_t end;
} token_t;

void display_buffer(const editor_state_t *state);
char *get_displayed_buffer_string(const editor_state_t *state);
char *get_bottom_bar(const int W, const editor_state_t *state);
void fill_color_tables(void);
void increment_angle(editor_state_t *state);
void decrement_angle(editor_state_t *state);
void step_rgb_state(editor_state_t *state);

#endif // !EDITOR_DISPLAY
