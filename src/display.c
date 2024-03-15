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

// the standardized codes for this project will be as below:
// \033[38;2;XXX;XXX;XXX;Xm  <-- this is 21 characters long
// so, this will be 21
#define ANSI_ESCAPE_LEN 21

typedef uint8_t ansi_num_t;

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
        dyn_str *filename;
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
                        dyn_str *filename = info.normal_info.filename;
                        const char *curr_line_str = num_to_str(info.normal_info.curr_line);
                        const char *total_lines_str = num_to_str(info.normal_info.total_lines);
                        int curr_line_len = strlen(curr_line_str);
                        int total_lines_len = strlen(total_lines_str);

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
                        if (width - used_up_front_space - used_up_back_space - 3 - filename->len < 0) {  
                                return bar;
                        }
        
                        // copy the filename and the straight line
                        memcpy(bar + width - used_up_back_space - 3, " | ", 3);
                        memcpy(bar + width - used_up_back_space - 3 - filename->len, filename->items, filename->len);

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

typedef struct {
        size_t start;
        size_t end;
} token_t;

char *get_highlighting_for_token(token_t t, highlighting_mode mode) {
        char *code = malloc(ANSI_ESCAPE_LEN + 1);   // TO_FREE OUTSIDE
        switch (mode) {
                case RANDOM: // TODO: make this not really random, seed with the token contents
                        snprintf(code, ANSI_ESCAPE_LEN + 1, "\033[38;2;%03d;%03d;%03d;%dm",
                                 rand() % 255, rand() % 255, rand() % 255, 1);
                        return code;
                case ALPHA:
                        return code;
        }
}

// applies syntax highlighting one of the following strategies:
char *apply_syntax_highlighting(dyn_str *line, highlighting_mode mode) { 

        char *output = malloc((ANSI_ESCAPE_LEN + 1) * line->len * sizeof(char));  // TO_FREE
        token_t tokens[line->len];
        
        // fill up tokens - treat every series of [_a-zA-Z0-9] or punctuation as a unique token
        size_t t = 0;
        tokens[t].start = 0;
        for (size_t c = 0; c < line->len; ++c) {
                if (!is_name_char(*(line->items + c)) || (c < line->len - 1 && !is_name_char(*(line->items + c + 1)))) {
                        tokens[t++].end = c + 1;
                        tokens[t].start = c + 1;
                }
        }
        tokens[t].end = line->len;

        // for each token in the line, build a new string with syntax highlighting
        size_t len = 0;
        for (size_t i = 0; i <= t; ++i) {
                char *code = get_highlighting_for_token(tokens[i], mode);
                memcpy(output + len, code, ANSI_ESCAPE_LEN);
                memcpy(output + (len += ANSI_ESCAPE_LEN), 
                       line->items + tokens[i].start, tokens[i].end - tokens[i].start);
                len += tokens[i].end - tokens[i].start;
                free(code);
        }
        output[len] = '\0';
        return output;
}

int display_buffer(file_buf *buffer, editor_mode mode, highlighting_mode syntax_mode) {

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
                char *formatted_line = apply_syntax_highlighting(&limited_line, syntax_mode);
                printf("%s\n\r", formatted_line);  // this should please work :D
                free(formatted_line); 
        }
        struct bar_info info = { .normal_info = (struct bar_info_buffer) { 
                .filename = &buffer->filename, .curr_line = buffer->cursor_line, .total_lines = buffer->lines.len}};
        const char *bar = get_bottom_bar(mode, W, info);
        printf("%s", bar);
        free((void *) bar);
        return 0;
}

// int display_buffer(file_buf *buffer, editor_mode mode) {
//
//         // determine information about the screen
//         struct winsize w = get_window_size();
//         int W = w.ws_col, H = w.ws_row;
//         int cursor_y, cursor_x;
//         store_cursor_pos(&cursor_y, &cursor_x);
//
//         // now, fill the string that contains what needs to be printed
//         char *output = malloc((H * W + 1) * sizeof(char));  // TOFREE
//         memset(output, ' ', H * W);
//         output[H * W] = '\0';
//         for (int i = 0; i < H - 1; ++i) { 
//                 if (buffer->screen_top_line - 1 + i >= buffer->lines.len) {  // no lines left
//                         continue;
//                 }
//                 dyn_str *line = buffer->lines.items + buffer->screen_top_line - 1 + i;
//                 memcpy(output + (i * W), line->items, line->len > W ? W : line->len);
//         }
//         struct bar_info info = { .normal_info = (struct bar_info_buffer) { 
//                 .filename = &buffer->filename, .curr_line = buffer->cursor_line, .total_lines = buffer->lines.len}};
//         const char *bar = get_bottom_bar(mode, W, info);
//         memcpy(output + (H - 1) * W, bar, W);
//
//         printf("%s", output);
//         free(output);
//         free((void *) bar);
//         return 0;
// }
