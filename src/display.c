#include "display.h"
#include "buf.h"
#include "utils.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>

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
int display_buffer(file_buf *buffer) {
        struct winsize w = get_window_size();
        int W = w.ws_col, H = w.ws_row;
        return W;
}
