#include "display.h"
#include "buf.h"
#include "utils.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>

typedef struct {
        int x;
        int y;
} point;

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
                case FILES: 
                        mode_text = "file";
                case EDIT:  // "  == mode ==  "
                        memcpy(bar + 2               , "== ", 3);
                        memcpy(bar + 2 + 3           , mode_text, mode_len);
                        memcpy(bar + 2 + 3 + mode_len, " ==", 3);
        }
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

int display_buffer(file_buf *buffer, editor_mode mode) {

        // determine information about the screen
        struct winsize w = get_window_size();
        int W = w.ws_col, H = w.ws_row;
        int cursor_y, cursor_x;
        store_cursor_pos(&cursor_y, &cursor_x);

        // now, fill the string that contains what needs to be printed
        char *output = malloc(H * W * sizeof(char));  // TOFREE
        memset(output, ' ', H * W);
        for (int i = 0; i < H - 1; ++i) { 
                if (buffer->screen_top_line + i >= buffer->lines.len) {  // no lines left
                        continue;
                }
                dyn_str *line = buffer->lines.items + buffer->screen_top_line + i;
                memcpy(output + (i * W), line->items, line->len > W ? W : line->len);
        }
        struct bar_info info = { .normal_info = (struct bar_info_buffer) { 0 } };  // TODO
        memcpy(output + (H - 1) * W, get_bottom_bar(mode, W, info), W);
        return 0;
}

