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
* Alternatively, if there is a message, it returns this:
*   MESSAGE...............................................................
*/
char *get_bottom_bar(const int W, const editor_state_t *state) {

        char *bar = malloc(W * sizeof(char));  // TOFREE
        memset(bar, ' ', W * sizeof(char));

        // error message?!?!?!?!?!?!?
        if (state->error_message.len) {
                int length = min(state->error_message.len, W - 2);
                memcpy(bar + 1, state->error_message.items, max(0, length));
                return bar;
        }

        // prevent a bar that is overflowing
        const size_t max_mode_len = 16;
        if (W < max_mode_len) {
                memset(bar, '=', W * sizeof(char));
                return bar;
        }

        // set the == mode_text == in the bar
        const char *mode_text;
        int mode_len;
        switch (state->mode) {
                case CMD_OPEN: mode_text = "open", mode_len = 4; break;
                case CMD_SEARCH: mode_text = "search", mode_len = 6; break;
                case CMD_RENAME: mode_text = "rename", mode_len = 6; break;
                case NORMAL: mode_text = "normal", mode_len = 6; break;
                case FILES: mode_text = "files", mode_len = 5; break;
                case EDIT: mode_text = "edit", mode_len = 4; break;
        }

        // prints the mode onto the bar
        memcpy(bar + 2               , "== ", 3);
        memcpy(bar + 2 + 3           , mode_text, mode_len);
        memcpy(bar + 2 + 3 + mode_len, " ==", 3);
        size_t used_up_front_space = mode_len + 10;  

        // add information about the filename and lines and stuff
        // in file mode, add information about the current directory
        switch (state->mode) {
                case CMD_RENAME:
                case CMD_OPEN:
                case CMD_SEARCH: {
                        int length = min(state->command_target.len, W - 4 - used_up_front_space);
                        memcpy(bar + used_up_front_space + 2, state->command_target.items, max(0, length));  // cap so its not negative
                } break;
                case NORMAL: 
                case EDIT: {
                        const buf_t *buf = state->buffers->items[state->buf_curr];
                        const char *filename = buf->filename;
                        const char *curr_line_str = num_to_str(buf->cursor_line);
                        const size_t curr_line_len = strlen(curr_line_str);
                        const char *total_lines_str = num_to_str(buf->lines.len);
                        const size_t total_lines_len = strlen(total_lines_str);

                        // chop off everything but the name of the file  --  TODO fix 
                        size_t filename_len = strlen(filename);
                        int64_t i = filename_len - 1;
                        for (; filename[i] != '/' && i >= 0; --i);
                        filename += ++i;
                        filename_len -= i;

                        // space_in_beginning + curr + "/" + total + "  "
                        if (used_up_front_space + curr_line_len + 1 + total_lines_len + 2 > W) {
                                return bar;  // return as is - forget about adding anything else
                        }
        
                        // fill the string with the fraction of the file read
                        memcpy(bar + W - 2 - total_lines_len, total_lines_str, total_lines_len);
                        bar[W - 2 - total_lines_len - 1] = '/';
                        memcpy(bar + W - 2 - total_lines_len - 1 - curr_line_len, curr_line_str, curr_line_len);

                        // check if there is space for the pipe operator and the filename
                        size_t used_up_back_space = curr_line_len + total_lines_len + 3;
                        if (W - used_up_front_space - used_up_back_space - 3 < filename_len) {
                                return bar;
                        }
        
                        // copy the filename and the straight line
                        memcpy(bar + W - used_up_back_space - 3, " | ", 3);
                        memcpy(bar + W - used_up_back_space - 3 - filename_len, filename, filename_len);
                } break;
                case FILES: {
                        const char *filename = state->files_view_buf.filename;
                        const size_t filename_len = strlen(filename);
                        
                        // absolutely unreadable pointer arithmetic
                        if (W - used_up_front_space - 2 < filename_len) {
                                memcpy(bar + used_up_front_space, "...", 3);
                                memcpy(bar + used_up_front_space + 3, filename + filename_len - (W - used_up_front_space - 2),
                                       W - used_up_front_space - 5);
                        } else {
                                memcpy(bar + W - 2 - filename_len, filename, filename_len);
                        }
                } break;
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
                ANSI_COLOR_TABLE[i].rgb.r = arc4random_uniform(156) + 100;
                ANSI_COLOR_TABLE[i].rgb.g = arc4random_uniform(156) + 100;
                ANSI_COLOR_TABLE[i].rgb.b = arc4random_uniform(156) + 100;
                ANSI_COLOR_TABLE[i].style = arc4random_uniform(ANSI_STYLE_VARIATION);
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
        return ANSI_NORMAL;
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
} highlighting_info_t;

void increment_angle(editor_state_t *state) {
        ++state->display_state.angle;
        state->display_state.angle %= 8;
}

void decrement_angle(editor_state_t *state) {
        state->display_state.angle += 7;
        state->display_state.angle %= 8;
}

double determine_angled_value(const angle_mode angle, double scalar, const bool is_reversed,
                              const double vertical_ratio, const double horizontal_ratio) {
        if (is_reversed) {
                scalar *= -1; 
        }

        switch (angle) {
                case ANG_180:
                case ANG_0: return scalar * horizontal_ratio;
                case ANG_90:
                case ANG_270: return scalar * vertical_ratio; 
                case ANG_135: 
                case ANG_315: return scalar * (vertical_ratio + horizontal_ratio) / 2;
                case ANG_225:
                case ANG_45: return scalar * (1 - vertical_ratio + horizontal_ratio) / 2;
        }
}

void fill_gradient_rgb(ansi_code_t *rgb_style, const highlighting_info_t *info,
                       const token_t t, const display_state_t *state) {
        rgb_t left = state->gradient_color.left, right = state->gradient_color.right;
        bool is_reversed = state->angle == ANG_180 || state->angle == ANG_90 || state->angle == ANG_135 || state->angle == ANG_225;
        rgb_t adder = is_reversed ? right : left;
        double vertical_ratio = (double) info->line_index / (info->H - 1);
        double horizontal_ratio = (double) t.start / info->line->len;
        rgb_style->rgb.r = determine_angled_value(state->angle, (double) right.r - left.r, is_reversed, vertical_ratio, horizontal_ratio) + adder.r;
        rgb_style->rgb.g = determine_angled_value(state->angle, (double) right.g - left.g, is_reversed, vertical_ratio, horizontal_ratio) + adder.g;
        rgb_style->rgb.b = determine_angled_value(state->angle, (double) right.b - left.b, is_reversed, vertical_ratio, horizontal_ratio) + adder.b;
}

/*
 * generated from https://colordesigner.io/gradient-generator, with the following settings:
 *   from #ff0000 to #00eaff HSL strategy, longer route 40 steps: plus 
 *   from #00eaff to #ff0000 OKLCH strategy, longer route 40 steps
 */
const rgb_t RGB_PROGRESSION[] = {
        (rgb_t){255, 0, 0},     (rgb_t){255, 20, 0},    (rgb_t){255, 40, 0},    (rgb_t){255, 60, 0},
        (rgb_t){255, 81, 0},    (rgb_t){255, 101, 0},   (rgb_t){255, 121, 0},   (rgb_t){255, 141, 0},
        (rgb_t){255, 161, 0},   (rgb_t){255, 181, 0},   (rgb_t){255, 202, 0},   (rgb_t){255, 222, 0},
        (rgb_t){255, 242, 0},   (rgb_t){248, 255, 0},   (rgb_t){228, 255, 0},   (rgb_t){208, 255, 0},
        (rgb_t){188, 255, 0},   (rgb_t){167, 255, 0},   (rgb_t){147, 255, 0},   (rgb_t){127, 255, 0},
        (rgb_t){107, 255, 0},   (rgb_t){87, 255, 0},    (rgb_t){67, 255, 0},    (rgb_t){46, 255, 0},
        (rgb_t){26, 255, 0},    (rgb_t){6, 255, 0},     (rgb_t){0, 255, 14},    (rgb_t){0, 255, 34},
        (rgb_t){0, 255, 54},    (rgb_t){0, 255, 74},    (rgb_t){0, 255, 95},    (rgb_t){0, 255, 115},
        (rgb_t){0, 255, 135},   (rgb_t){0, 255, 155},   (rgb_t){0, 255, 175},   (rgb_t){0, 255, 195},
        (rgb_t){0, 255, 216},   (rgb_t){0, 255, 236},   (rgb_t){0, 254, 255},   (rgb_t){0, 234, 255},
        (rgb_t){0, 231, 255},   (rgb_t){0, 228, 255},   (rgb_t){22, 225, 255},  (rgb_t){45, 222, 255},
        (rgb_t){67, 219, 255},  (rgb_t){85, 214, 255},  (rgb_t){97, 210, 255},  (rgb_t){107, 205, 255},
        (rgb_t){115, 201, 255}, (rgb_t){122, 197, 255}, (rgb_t){128, 193, 255}, (rgb_t){134, 189, 255},
        (rgb_t){140, 184, 255}, (rgb_t){146, 180, 255}, (rgb_t){151, 175, 255}, (rgb_t){157, 170, 255},
        (rgb_t){163, 165, 255}, (rgb_t){169, 159, 255}, (rgb_t){175, 153, 255}, (rgb_t){182, 146, 255},
        (rgb_t){190, 138, 255}, (rgb_t){198, 129, 255}, (rgb_t){206, 122, 255}, (rgb_t){212, 116, 255},
        (rgb_t){218, 110, 247}, (rgb_t){224, 104, 237}, (rgb_t){229, 97, 227},  (rgb_t){234, 91, 216},
        (rgb_t){238, 85, 204},  (rgb_t){242, 78, 192},  (rgb_t){246, 72, 180},  (rgb_t){249, 65, 166},
        (rgb_t){252, 58, 152},  (rgb_t){254, 51, 138},  (rgb_t){255, 43, 123},  (rgb_t){255, 35, 107},
        (rgb_t){255, 26, 90},   (rgb_t){255, 17, 71},   (rgb_t){255, 7, 47}, };

const uint8_t RGB_PROGRESSION_LEN = sizeof(RGB_PROGRESSION) / sizeof(rgb_t);

void step_rgb_state(editor_state_t *state) {
        ++state->display_state.rgb_state;
        state->display_state.rgb_state %= RGB_PROGRESSION_LEN;
}

void fill_rgb_mode_rgb(ansi_code_t *rgb_style, const highlighting_info_t *info,
                       const token_t t, const display_state_t *state) {
        int prog_index = determine_angled_value(state->angle, RGB_PROGRESSION_LEN, 
                                                    state->angle == ANG_180 || state->angle == ANG_90 || state->angle == ANG_135 || state->angle == ANG_225,
                                                    (double) info->line_index / (info->H - 1), (double) t.start / info->W);
        rgb_style->rgb = RGB_PROGRESSION[(prog_index + state->rgb_state + RGB_PROGRESSION_LEN) % RGB_PROGRESSION_LEN];
}

char *get_highlighting_for_token(const highlighting_info_t *info, const token_t t, const display_state_t *state) {
        char *code = malloc(ANSI_ESCAPE_LEN + 1);   // TO_FREE OUTSIDE
        ansi_code_t rgb_style = {0};
        switch (state->syntax_mode) {
                case HIGH_NONE: {
                        rgb_style.rgb = (rgb_t) {255, 255, 255};
                        rgb_style.style = get_style_from_style_enum(state->text_style_mode);
                } break;
                case HIGH_RANDOM: {
                        rgb_style.style = info->line->items[t.start] == ' ' ? 22 
                                : get_ansi_style(arc4random_uniform(ANSI_STYLE_VARIATION));
                        rgb_style.rgb.r = arc4random_uniform(156) + 100;
                        rgb_style.rgb.g = arc4random_uniform(156) + 100;
                        rgb_style.rgb.b = arc4random_uniform(156) + 100;
                } break;
                case HIGH_ALPHA: {
                        rgb_style = ANSI_COLOR_TABLE[(uint8_t) info->line->items[t.start]];
                        rgb_style.style = get_ansi_style(rgb_style.style);
                } break;
                case HIGH_GRADIENT: {
                        rgb_style.style = get_style_from_style_enum(state->text_style_mode);
                        fill_gradient_rgb(&rgb_style, info, t, state);
                } break;
                case HIGH_RGB: {
                        rgb_style.style = get_style_from_style_enum(state->text_style_mode);
                        fill_rgb_mode_rgb(&rgb_style, info, t, state);
                }
        }
        snprintf(code, ANSI_ESCAPE_LEN + 1, ANSI_COLOR_FORMAT, 
                 rgb_style.rgb.r, rgb_style.rgb.g, rgb_style.rgb.b, rgb_style.style);
        return code;
}

const char *apply_syntax_highlighting(const highlighting_info_t *info, const display_state_t *state) {

        // assuming one code per character, allocate enough to fill everything + \0
        char *output = malloc(((ANSI_ESCAPE_LEN + 1) * info->W) * sizeof(char));
        token_t tokens[info->line->len + 1];
        size_t t = 0;

        // fill up tokens - treat every series of [_a-zA-Z0-9] or punctuation as a unique token
        // alternatively, when the highlighting mode is a gradient, each character is a token
        tokens[t].start = info->col_start - 1;
        for (size_t c = info->col_start - 1, i = 0; c < info->line->len && i < info->W; ++c, ++i) {
                bool current_token_ending = c < info->line->len - 1 
                                            && !is_name_char(info->line->items[c + 1]);
                if (state->syntax_mode == HIGH_GRADIENT || state->syntax_mode == HIGH_RGB
                    || !is_name_char(info->line->items[c]) 
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

                // not sure exactly why I need this, but I think it skips an invalid token at the end of a line
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

const buf_t *get_buffer_by_state(const editor_state_t *state) {
        switch (state->mode) {
                case CMD_RENAME:
                case FILES: return &state->files_view_buf;
                default: return state->buffers->items[state->buf_curr];
        }
}

/*
 * Returns an entire string that will be printed to the screen while editing.
 * The string will be null-terminated.
 */
char *get_displayed_buffer_string(const editor_state_t *state) {
        
        // determine information about the screen
        const struct winsize w = get_window_size();
        const int W = w.ws_col, H = w.ws_row;
        const buf_t *buf = get_buffer_by_state(state);
        char blank_space_block[ANSI_ESCAPE_LEN + 2];
        snprintf(blank_space_block, ANSI_ESCAPE_LEN + 1, ANSI_COLOR_FORMAT, 0, 0, 0, 22);
        blank_space_block[ANSI_ESCAPE_LEN] = ' ';

        // creating a big string in which lines are printed into
        // so that screensaver functions can access the string without it being printed 
        char *output = malloc(H * (W + 1) * (ANSI_ESCAPE_LEN + 1) * sizeof(char));
        size_t len = 0;
        for (int i = 0; i < H - 1; ++i) { 

                // no lines left 
                if (buf->screen_top_line - 1 + i >= buf->lines.len) {
                        for (size_t j = 0; j < W; ++j) {
                                memcpy(output + len, blank_space_block, ANSI_ESCAPE_LEN + 1);
                                len += ANSI_ESCAPE_LEN + 1;
                        }
                        continue;
                } 

                const dyn_str *line = buf->lines.items + buf->screen_top_line - 1 + i;

                // after doing a benchmark, i found that strlen + memcpy seems to be faster  
                // for small strings than using snprintf
                const highlighting_info_t info = {
                        .W = W, .H = H, .line = line, .line_index = i, .col_start = buf->screen_left_col
                };
                const char *formatted_line = apply_syntax_highlighting(&info, &state->display_state);
                const size_t formatted_len = strlen(formatted_line);
                memcpy(output + len, formatted_line, formatted_len);
                len += formatted_len;
                for (size_t j = 0; j < W - formatted_len / (ANSI_ESCAPE_LEN + 1); ++j) {
                        memcpy(output + len, blank_space_block, ANSI_ESCAPE_LEN + 1);
                        len += ANSI_ESCAPE_LEN + 1;
                }
                free((void *) formatted_line);
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

        const buf_t *buf = get_buffer_by_state(state);
        const struct winsize w = get_window_size();
        const char *buffer_output = get_displayed_buffer_string(state);
        const char *bar = get_bottom_bar(w.ws_col, state);

        move_to_top_left();
        hide_cursor();
        printf("%s\033[0m%s%s", buffer_output, state->error_message.len ? "\033[1m" : "", bar);
        move_cursor_to(buf->cursor_line - buf->screen_top_line + 1,
                       buf->cursor_col - buf->screen_left_col + 1);
        if (!is_command_mode(state->mode)) {
                show_cursor();
        }
        free((void *) bar);
        free((void *) buffer_output);
        fflush(stdout);
}
