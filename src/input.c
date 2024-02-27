#include "input.h"

#include <stdbool.h>
#include <termios.h>

static struct termios tbufsave;

int input_set_tty_raw(void) {
        struct termios tbuf;
        if (tcgetattr(0, &tbufsave) == -1) {
                return 1;
        }

        tbuf = tbufsave;
        tbuf.c_iflag &= ~(INLCR | ICRNL | ISTRIP | IXON | BRKINT);
        tbuf.c_oflag &= ~OPOST;
        tbuf.c_lflag &= ~(ICANON | ISIG | ECHO);
        tbuf.c_cc[VMIN] = 1;
        tbuf.c_cc[VTIME] = 0;

        return tcsetattr(0, TCSANOW, &tbuf) == -1;   // 1 for error
}

int input_restore_tty(void) {
        return tcsetattr(0, TCSANOW, &tbufsave) == -1;   // 1 for error
}

bool is_ctrl(char c) {
        return 1 <= c && c <= 26;
}
