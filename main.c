#include "src/list.h"
#include "src/utils.h"
#include "src/buf.h"
#include "src/input.h"
#include "src/display.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

void set_timer(struct timespec *timer) {
        clock_gettime(CLOCK_MONOTONIC, timer);
}
double get_ms_elapsed(struct timespec *start) {
        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);
        return (end.tv_sec - start->tv_sec) * 1e3 + (end.tv_nsec - start->tv_nsec) / 1e6;
}

int main(int argc, char **argv) {

        // open all the buffers, storing them in a list of buffers
        if (argc == 1) {
                exit_error("Must pass in a filename to run this editor.");
        }
        buf_list buffers = list_init(buf_list, argc); 
        for (uint8_t i = 1; i < argc; ++i) {
                list_append(buffers, buf_open(argv[i]));
        }

        // according to the man pages, if NULL, space is allocated for it and a pointer to it is returned
        char *cwd = getcwd(NULL, PATH_MAX + 1);  

        // main loop, handling inputs and etc
        editor_mode mode = EDIT;
        int buf_curr = buffers.len - 1;
        dyn_str input_history = list_init(dyn_str, 128);
        input_set_tty_raw();
        struct timespec timer;
        while (1) {

                // if the input was received soon enough after the previous one, 
                // it is reasonable to assume that it is the 27-91-XX combo from hitting arrow keys
                // or another possible input that could mess things up
                set_timer(&timer);
                char c = getchar();
                if (c == 'q') {
                        break;
                }
                if (get_ms_elapsed(&timer) < 1) {
                        continue;
                }

                if (mode == NORMAL || mode == EDIT) {
                        display_buffer(buffers.items[buf_curr], mode);
                }
                switch (mode) {
                        case NORMAL: 
                        case FILES:
                        case EDIT: break;
                }
        }
        
        return input_restore_tty();
}
