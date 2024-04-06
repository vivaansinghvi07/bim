#include "display.h"
#include "../buf.h"
#include "../utils.h"

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

void display_by_mode(const editor_state_t *state) {
        switch (state->mode) {
                case NORMAL:
                case EDIT: {
                        display_buffer(state);
                } break;
        }
}

struct winsize get_window_size(void) {
        struct winsize w;
        if (ioctl(0, TIOCGWINSZ, &w) == -1) {
                exit_error("Could not determine terminal size.");
        }
        return w;
}

/*
* Creates a bottom bar like so: 
*   == MODE ==                                       filename.ext | curr/len 
* The length of the bar is guaranteed to be <width>
*/
char *get_bottom_bar(const int W, const editor_state_t *state) {

        const char *mode_text = "edit";
        int mode_len = 4;
        char *bar = malloc(W * sizeof(char));  // TOFREE
        memset(bar, ' ', W * sizeof(char));

        // prevent a bar that is overflowing
        if (W < 10) {
                memset(bar, '=', W * sizeof(char));
                return bar;
        }

        // set the == mode_text == in the bar
        switch (state->mode) {
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
        switch (state->mode) {
                case NORMAL:
                case EDIT:;
                        const file_buf *buf = state->buffers->items[state->buf_curr];
                        const char *filename = buf->filename;
                        const char *curr_line_str = num_to_str(buf->cursor_line);
                        const char *total_lines_str = num_to_str(buf->lines.len);
                        const size_t filename_len = strlen(filename);
                        const size_t curr_line_len = strlen(curr_line_str);
                        const size_t total_lines_len = strlen(total_lines_str);

                        // space_in_beginning + curr + "/" + total + "  "
                        if (used_up_front_space + curr_line_len + 1 + total_lines_len + 2 > W) {
                                return bar;  // return as is - forget about adding anything else
                        }
        
                        // fill the string with the fraction of the file read
                        memcpy(bar + W - 2 - total_lines_len, total_lines_str, total_lines_len);
                        bar[W - 2 - total_lines_len - 1] = '/';
                        memcpy(bar + W - 2 - total_lines_len - 1 - curr_line_len, curr_line_str, curr_line_len);

                        // check if there is space for the pipe operator and the filename
                        int used_up_back_space = curr_line_len + total_lines_len + 3;
                        if (W - used_up_front_space - used_up_back_space - 3 - filename_len < 0) {  
                                return bar;
                        }
        
                        // copy the filename and the straight line
                        memcpy(bar + W - used_up_back_space - 3, " | ", 3);
                        memcpy(bar + W - used_up_back_space - 3 - filename_len, filename, filename_len);

                case FILES:
                        break;  // TODO
        }
        return bar;
}

// check if [_a-zA-Z0-9] matches <c>
bool is_name_char(const char c) {
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
                ANSI_COLOR_TABLE[i].rgb.r = rand() % 256;
                ANSI_COLOR_TABLE[i].rgb.g = rand() % 256;
                ANSI_COLOR_TABLE[i].rgb.b = rand() % 256;
                ANSI_COLOR_TABLE[i].style = rand() % ANSI_STYLE_VARIATION;
        }
}

// return a random ANSI styling code 
uint8_t get_ansi_style(const uint8_t key) {
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

uint8_t get_style_from_style_enum(const text_style_mode mode) {
        switch (mode) {
                case STYLE_BOLD: return ANSI_BOLD; 
                case STYLE_NORMAL: return ANSI_NORMAL; 
                case STYLE_ITALIC: return ANSI_ITALIC; 
        }
        return ANSI_NORMAL;
}

typedef struct {
        const dyn_str *line;
        const int W, H, line_index, col_start; 
} gradient_line_info_t;

void increment_gradient(editor_state_t *state) {
        state->display_state.gradient_angle++;
        state->display_state.gradient_angle %= 8;
}

void decrement_gradient(editor_state_t *state) {
        state->display_state.gradient_angle += 7;
        state->display_state.gradient_angle %= 8;
}

void fill_gradient_rgb(ansi_code_t *rgb_style, const gradient_line_info_t *info, const token_t t, const display_state_t *state) {
        rgb_t left = state->gradient_color.left, right = state->gradient_color.right;
        rgb_style->style = get_style_from_style_enum(state->text_style_mode);

        switch (state->gradient_angle) {
                case GRAD_ANG_180: {
                        rgb_t temp = right; right = left; left = temp;
                }
                case GRAD_ANG_0: {
                        rgb_style->rgb.r = (double) (right.r - left.r) * t.start / info->line->len + left.r;  
                        rgb_style->rgb.b = (double) (right.b - left.b) * t.start / info->line->len + left.b;  
                        rgb_style->rgb.g = (double) (right.g - left.g) * t.start / info->line->len + left.g;
                } return;
                case GRAD_ANG_90: {
                        rgb_t temp = right; right = left; left = temp;
                }
                case GRAD_ANG_270: {
                        rgb_style->rgb.r = (double) (right.r - left.r) * info->line_index / (info->H - 1) + left.r;
                        rgb_style->rgb.g = (double) (right.g - left.g) * info->line_index / (info->H - 1) + left.g;
                        rgb_style->rgb.b = (double) (right.b - left.b) * info->line_index / (info->H - 1) + left.b;
                } return;
                case GRAD_ANG_135: {
                        rgb_t temp = right; right = left; left = temp;
                }
                case GRAD_ANG_315: {
                        rgb_style->rgb.r = (double) (right.r - left.r) * info->line_index / (info->H - 1) / 2 + 
                                           (double) (right.r - left.r) * t.start / info->line->len / 2 + left.r;
                        rgb_style->rgb.g = (double) (right.g - left.g) * info->line_index / (info->H - 1) / 2 + 
                                           (double) (right.g - left.g) * t.start / info->line->len / 2 + left.g;
                        rgb_style->rgb.b = (double) (right.b - left.b) * info->line_index / (info->H - 1) / 2 + 
                                           (double) (right.b - left.b) * t.start / info->line->len / 2 + left.b;
                } return;
                case GRAD_ANG_225: {
                        rgb_t temp = right; right = left; left = temp;
                }
                case GRAD_ANG_45: {
                        rgb_style->rgb.r = (double) (right.r - left.r) * (info->H - info->line_index - 1) / (info->H - 1) / 2 + 
                                           (double) (right.r - left.r) * t.start / info->line->len / 2 + left.r;
                        rgb_style->rgb.g = (double) (right.g - left.g) * (info->H - info->line_index - 1) / (info->H - 1) / 2 + 
                                           (double) (right.g - left.g) * t.start / info->line->len / 2 + left.g;
                        rgb_style->rgb.b = (double) (right.b - left.b) * (info->H - info->line_index - 1) / (info->H - 1) / 2 + 
                                           (double) (right.b - left.b) * t.start / info->line->len / 2 + left.b;
                }
        }
}

char *get_highlighting_for_token(const gradient_line_info_t *info, const token_t t, const display_state_t *state) {
        char *code = malloc(ANSI_ESCAPE_LEN + 1);   // TO_FREE OUTSIDE
        ansi_code_t rgb_style = {0};
        switch (state->syntax_mode) {
                case HIGH_NONE: {
                        rgb_style.rgb = (rgb_t) {255, 255, 255};
                        rgb_style.style = get_style_from_style_enum(state->text_style_mode);
                } break;
                case HIGH_RANDOM: {
                        rgb_style.style = info->line->items[t.start] == ' ' ? 22 
                                : get_ansi_style(rand() % ANSI_STYLE_VARIATION);
                        rgb_style.rgb.r = rand() % 156 + 100;
                        rgb_style.rgb.g = rand() % 156 + 100;
                        rgb_style.rgb.b = rand() % 156 + 100;
                } break;
                case HIGH_ALPHA: {
                        rgb_style = ANSI_COLOR_TABLE[info->line->items[t.start]];
                        rgb_style.style = get_ansi_style(rgb_style.style);
                } break;
                case HIGH_GRADIENT: {
                        fill_gradient_rgb(&rgb_style, info, t, state);
                } break;
        }
        snprintf(code, ANSI_ESCAPE_LEN + 1, ANSI_COLOR_FORMAT, 
                 rgb_style.rgb.r, rgb_style.rgb.g, rgb_style.rgb.b, rgb_style.style);
        return code;
}

char *apply_syntax_highlighting(const gradient_line_info_t *info, const display_state_t *state) {

        // assuming one code per character, allocate enough to fill everything + \0
        char *output = malloc(((ANSI_ESCAPE_LEN + 1) * info->line->len + 1) * sizeof(char));
        token_t tokens[info->line->len + 1];
        size_t t = 0;

        // fill up tokens - treat every series of [_a-zA-Z0-9] or punctuation as a unique token
        // alternatively, when the highlighting mode is a gradient, each character is a token
        tokens[t].start = 0;
        for (size_t c = info->col_start - 1, i = 0; c < info->line->len && i < info->W; ++c, ++i) {
                bool current_token_ending = c < info->line->len - 1 
                                            && !is_name_char(*(info->line->items + c + 1));
                if (state->syntax_mode == HIGH_GRADIENT 
                    || !is_name_char(*(info->line->items + c)) 
                    || current_token_ending) {
                        tokens[t++].end = c + 1;
                        tokens[t].start = c + 1;
                }
        }
        tokens[t].end = info->line->len - (info->col_start - 1) < info->W 
                        ? info->line->len : info->col_start - 1 + info->W - 1;

        // for each token in the info->line, build a new string with syntax highlighting
        size_t len = 0;
        for (size_t i = 0; i <= t; ++i) {
                token_t *tok = tokens + i;
                if (tok->start == tok->end) {
                        continue;
                }
                char *code = get_highlighting_for_token(info, *tok, state);
                for (size_t j = tok->start; j < tok->end; ++j, ++len) {
                        memcpy(output + len, code, ANSI_ESCAPE_LEN);
                        output[len += ANSI_ESCAPE_LEN] = info->line->items[j];
                }
                free(code);
        }
        output[len] = '\0';
        return output;
}


/*
 * Returns an entire string that will be printed to the screen while editing.
 * The string will be null-terminated.
 */
char *get_displayed_buffer_string(const editor_state_t *state) {
        
        // determine information about the screen
        const struct winsize w = get_window_size();
        const int W = w.ws_col, H = w.ws_row;
        const file_buf *buf = state->buffers->items[state->buf_curr];
        char blank[ANSI_ESCAPE_LEN + 2];
        snprintf(blank, ANSI_ESCAPE_LEN + 1, ANSI_COLOR_FORMAT, 0, 0, 0, 22);
        blank[ANSI_ESCAPE_LEN] = ' ';

        // creating a big string in which lines are printed into
        // so that screensaver functions can access the string without it being printed 
        char *output = malloc(H * (W + 1) * (ANSI_ESCAPE_LEN + 1) * sizeof(char));  // W + 1 accounting for \n
        int len = 0;
        for (int i = 0; i < H - 1; ++i) { 

                // no lines left 
                if (buf->screen_top_line - 1 + i >= buf->lines.len) {
                        for (size_t j = 0; j < W; ++j) {
                                memcpy(output + len, blank, ANSI_ESCAPE_LEN + 1);
                                len += ANSI_ESCAPE_LEN + 1;
                        }
                        continue;
                } 

                const dyn_str *line = buf->lines.items + buf->screen_top_line - 1 + i;

                // after doing a benchmark, i found that strlen + memcpy seems to be faster  
                // for small strings than using snprintf
                const gradient_line_info_t info = {
                        .W = W, .H = H, .line = line, .line_index = i, .col_start = buf->screen_left_col
                };
                char *formatted_line = apply_syntax_highlighting(&info, &state->display_state);
                const size_t formatted_len = strlen(formatted_line);
                memcpy(output + len, formatted_line, formatted_len);
                len += formatted_len;
                for (size_t j = 0; j < W - formatted_len / (ANSI_ESCAPE_LEN + 1); ++j) {
                        memcpy(output + len, blank, ANSI_ESCAPE_LEN + 1);
                        len += ANSI_ESCAPE_LEN + 1;
                }
                free(formatted_line); 
        }

        output[len] = '\0';
        return output;
}

/*
 * Determine the current cursor position, 
 *   display a buffer (meaning we are in the normal or edit mode),
 *   and return the cursor to the old position.
 */
void display_buffer(const editor_state_t *state) {

        const file_buf *buffer = state->buffers->items[state->buf_curr];
        const struct winsize w = get_window_size();
        char *buffer_output = get_displayed_buffer_string(state);
        char *bar = get_bottom_bar(w.ws_col, state);

        move_to_top_left();
        hide_cursor();
        printf("%s\033[0m%s", buffer_output, bar);
        move_cursor_to(buffer->cursor_line - buffer->screen_top_line + 1,
                       buffer->cursor_col - buffer->screen_left_col + 1);
        show_cursor();
        free(bar);
        free(buffer_output);
        fflush(stdout);
}

