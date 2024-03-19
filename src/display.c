#include "display.h"
#include "buf.h"
#include "list.h"
#include "utils.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>

#define ANSI_NORMAL 22
#define ANSI_BOLD 1
#define ANSI_ITALIC 3
#define ANSI_DIM 2
#define ANSI_UNDERLINE 4
#define ANSI_STRIKETHROUGH 9
#define ANSI_BLINKING 5

// the standardized codes for this project will be as below:
// \033[0;38;2;XXX;XXX;XXX;XXm  <-- this is 24 characters long
#define ANSI_ESCAPE_LEN 24
#define ANSI_COLOR_FORMAT "\033[0;38;2;%03d;%03d;%03d;%02dm"
#define ANSI_STYLE_VARIATION 3  // affects the get_ansi_style function

typedef struct {
        uint8_t r, g, b, style;
} ansi_code_t;

struct winsize get_window_size(void) {
        struct winsize w;
        if (ioctl(0, TIOCGWINSZ, &w) == -1) {
                exit_error("Could not determine terminal size.");
        }
        return w;
}

/*
 * stores the position of the cursor in the given integer pointers,
 * with 1, 1 representing the top left
 * https://stackoverflow.com/a/50888457
 */
int store_cursor_pos(int *y, int *x) {

        struct termios term, restore;
        char buf[30] = {0};

        // controlling terminal status to avoid displaying
        tcgetattr(0, &term);
        tcgetattr(0, &restore);
        term.c_lflag &= ~(ICANON|ECHO);
        tcsetattr(0, TCSANOW, &term);
        write(1, "\033[6n", 4);

        // get response in the form ^[[xx;xxR
        int curr;
        char ch;
        for(curr = 0, ch = 0; ch != 'R'; ++curr) {
                if (!read(0, &ch, 1)) {
                        tcsetattr(0, TCSANOW, &restore);
                        return 1;
                }
                buf[curr] = ch;
        }

        // invalid response, there is not enough information given
        if (curr <= 6) {
                tcsetattr(0, TCSANOW, &restore);
                return 1;
        }

        *y = 0; *x = 0;
        int pow;
        for(curr -= 2, pow = 1; buf[curr] != ';'; curr--, pow *= 10) {  // parsing the second number
                *x = *x + (buf[curr] - '0') * pow;
        }
        for(curr--, pow = 1; buf[curr] != '['; curr--, pow *= 10) {  // parsing the first number
                *y = *y + (buf[curr] - '0') * pow;
        }

        tcsetattr(0, TCSANOW, &restore);
        return 0;
}

// is this even good code? what am i doing???
struct bar_info_buffer {
        int curr_line;
        int total_lines;
        const char *filename;
};

struct bar_info {
        union {
                struct bar_info_buffer normal_info;
                char *file_mode_dir;
        };
};

/*
* Creates a bottom bar like so: 
*   == MODE ==                                       filename.ext | curr/len 
* The length of the bar is guaranteed to be <width>
*/
const char *get_bottom_bar(editor_mode mode, int width, struct bar_info info) {
        const char *mode_text = "edit";
        int mode_len = 4;
        char *bar = malloc(width * sizeof(char));  // TOFREE
        memset(bar, ' ', width * sizeof(char));

        // prevent a bar that is overflowing
        if (width < 10) {
                memset(bar, '=', width * sizeof(char));
                return bar;
        }

        // set the == mode_text == in the bar
        switch (mode) {
                case NORMAL:
                        mode_text = "normal", mode_len = 6;
                        break;
                case FILES: 
                        mode_text = "file";
                case EDIT: 
                        break;
        }

        memcpy(bar + 2               , "== ", 3);
        memcpy(bar + 2 + 3           , mode_text, mode_len);
        memcpy(bar + 2 + 3 + mode_len, " ==", 3);
        int used_up_front_space = mode_len + 10;  

        // add information about the filename and lines and stuff
        // in file mode, add information about the current directory
        switch (mode) {
                case NORMAL:
                case EDIT:;
                        const char *filename = info.normal_info.filename;
                        const char *curr_line_str = num_to_str(info.normal_info.curr_line);
                        const char *total_lines_str = num_to_str(info.normal_info.total_lines);
                        size_t filename_len = strlen(filename);
                        size_t curr_line_len = strlen(curr_line_str);
                        size_t total_lines_len = strlen(total_lines_str);

                        // space_in_beginning + curr + "/" + total + "  "
                        if (used_up_front_space + curr_line_len + 1 + total_lines_len + 2 > width) {
                                return bar;  // return as is - forget about adding anything else
                        }
        
                        // fill the string with the fraction of the file read
                        memcpy(bar + width - 2 - total_lines_len, total_lines_str, total_lines_len);
                        bar[width - 2 - total_lines_len - 1] = '/';
                        memcpy(bar + width - 2 - total_lines_len - 1 - curr_line_len, curr_line_str, curr_line_len);

                        // check if there is space for the pipe operator and the filename
                        int used_up_back_space = curr_line_len + total_lines_len + 3;
                        if (width - used_up_front_space - used_up_back_space - 3 - filename_len < 0) {  
                                return bar;
                        }
        
                        // copy the filename and the straight line
                        memcpy(bar + width - used_up_back_space - 3, " | ", 3);
                        memcpy(bar + width - used_up_back_space - 3 - filename_len, filename, filename_len);

                case FILES:
                        break;  // TODO
        }
        return bar;
}

// check if [_a-zA-Z0-9] matches <c>
bool is_name_char(char c) {
        return c >= 'a' && c <= 'z' ||
               c >= 'A' && c <= 'Z' || 
               c >= '0' && c <= '9' ||
               c == '_';
}

/*
 * In the mode HIGH_ALPHA, the first character of each token corresponds to a color.
 * The below function must be called when the editor starts to fill the array.
 */
ansi_code_t ANSI_COLOR_TABLE[256];
void fill_ansi_color_table(void) {
        for (uint16_t i = 0; i < 256; ++i) {
                ANSI_COLOR_TABLE[i].r = rand() % 256;
                ANSI_COLOR_TABLE[i].g = rand() % 256;
                ANSI_COLOR_TABLE[i].b = rand() % 256;
                ANSI_COLOR_TABLE[i].style = rand() % ANSI_STYLE_VARIATION;
        }
}

// return a random ANSI styling code 
uint8_t get_ansi_style(uint8_t key) {
        switch (key) {
                case 0: return ANSI_NORMAL;
                case 1: return ANSI_BOLD;
                case 2: return ANSI_ITALIC;
                case 3: return ANSI_DIM;
                case 4: return ANSI_UNDERLINE;
                case 5: return ANSI_STRIKETHROUGH;
                case 6: return ANSI_BLINKING;
        }
        return 22;
}

typedef struct {
        size_t start;
        size_t end;
} token_t;

void store_gradient_color(ansi_code_t *code, gradient_color color, uint8_t grad_value) {
        code->style = 1;
        switch (color) {
                case GRADIENT_RED: code->r = grad_value; break;
                case GRADIENT_BLUE: code->b = grad_value; break;
                case GRADIENT_GREEN: code->g = grad_value; break;
                case GRADIENT_PURPLE: code->r = code->b = grad_value; break;
                case GRADIENT_YELLOW: code->r = code->g = grad_value; break;
                case GRADIENT_CYAN: code->g = code->b = grad_value; break;
        }
}

char *get_highlighting_for_token(dyn_str *line, token_t t, display_state_t *state, int width) {
        char *code = malloc(ANSI_ESCAPE_LEN + 1);   // TO_FREE OUTSIDE
        ansi_code_t rgb_style = {0};
        switch (state->syntax_mode) {
                case HIGH_RANDOM: {
                        rgb_style.style = line->items[t.start] == ' ' ? 22 
                                : get_ansi_style(rand() % ANSI_STYLE_VARIATION);
                        rgb_style.r = rand() % 256;
                        rgb_style.g = rand() % 256;
                        rgb_style.b = rand() % 256;
                } break;
                case HIGH_ALPHA: {
                        rgb_style = ANSI_COLOR_TABLE[line->items[t.start]];
                        rgb_style.style = get_ansi_style(rgb_style.style);
                } break;
                case HIGH_GRADIENT: {
                        uint8_t grad_value = 150 * (width - t.start) / width + 105;
                        store_gradient_color(&rgb_style, state->gradient_color, grad_value);
                } break;
        }
        snprintf(code, ANSI_ESCAPE_LEN + 1, ANSI_COLOR_FORMAT, 
                 rgb_style.r, rgb_style.g, rgb_style.b, rgb_style.style);
        return code;
}

// applies syntax highlighting one of the following strategies:
char *apply_syntax_highlighting(dyn_str *line, display_state_t *state, int width) {

        char *output = malloc((ANSI_ESCAPE_LEN + 1) * line->len * sizeof(char));  // TO_FREE
        token_t tokens[line->len + 1];
        size_t t = 0;

        // fill up tokens - treat every series of [_a-zA-Z0-9] or punctuation as a unique token
        // alternatively, when the highlighting mode is a gradient, each character is a token
        tokens[t].start = 0;
        for (size_t c = 0; c < line->len; ++c) {
                if (state->syntax_mode == HIGH_GRADIENT 
                    || !is_name_char(*(line->items + c)) 
                    || (c < line->len - 1 && !is_name_char(*(line->items + c + 1)))) {
                        tokens[t++].end = c + 1;
                        tokens[t].start = c + 1;
                }
        }
        tokens[t].end = line->len;

        // for each token in the line, build a new string with syntax highlighting
        size_t len = 0;
        for (size_t i = 0; i <= t; ++i) {
                if (tokens[i].start == tokens[i].end) {
                        continue;
                }
                char *code = get_highlighting_for_token(line, tokens[i], state, width);
                memcpy(output + len, code, ANSI_ESCAPE_LEN);
                memcpy(output + (len += ANSI_ESCAPE_LEN), 
                       line->items + tokens[i].start, tokens[i].end - tokens[i].start);
                len += tokens[i].end - tokens[i].start;
                free(code);
        }
        output[len] = '\0';
        return output;
}

int display_buffer(file_buf *buffer, editor_mode mode, display_state_t *state) {

        // determine information about the screen
        struct winsize w = get_window_size();
        int W = w.ws_col, H = w.ws_row;
        int cursor_y, cursor_x;
        store_cursor_pos(&cursor_y, &cursor_x);

        // print each line with syntax highlighting
        char *printer = malloc((W + 1) * sizeof(char));
        printer[W] = '\0';
        for (int i = 0; i < H - 1; ++i) { 
                if (buffer->screen_top_line - 1 + i >= buffer->lines.len) {  // no lines left 
                        continue;
                } 
                dyn_str *line = buffer->lines.items + buffer->screen_top_line - 1 + i;
                dyn_str limited_line = *line; 
                if (line->len + 1 > W) {
                        limited_line.len = W - 1;
                }
                char *formatted_line = apply_syntax_highlighting(&limited_line, state, W);
                printf("%s\n\r", formatted_line);  // this should please work :D
                free(formatted_line); 
        }

        struct bar_info info;
        switch (mode) {
                case NORMAL:
                case EDIT: {
                        info = (struct bar_info) { .normal_info = (struct bar_info_buffer) { 
                                .filename = buffer->filename, .curr_line = buffer->cursor_line,
                                .total_lines = buffer->lines.len}};
                } break;
        }
        const char *bar = get_bottom_bar(mode, W, info);
        printf("%s", bar);
        free((void *) bar);
        return 0;
}

