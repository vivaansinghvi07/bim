#include "display.h"
#include "buf.h"
#include "utils.h"
#include <sys/ioctl.h>

struct winsize get_window_size(void) {
        struct winsize w;
        if (ioctl(0, TIOCGWINSZ, &w) == -1) {
                exit_error("Could not determine terminal size.");
        }
        return w;
}

int display_buffer(file_buf *buffer) {
        struct winsize w = get_window_size();
        int W = w.ws_col, H = w.ws_row;
        return W;
}
