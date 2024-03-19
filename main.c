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
#include <stdbool.h>

typedef struct {
        size_t buf_curr;
        struct timespec timer; 
        dyn_str input_history;
        bool initializing;
        editor_mode mode;
        display_state_t display_state;
} editor_state_t;

void set_timer(struct timespec *timer) {
        clock_gettime(CLOCK_MONOTONIC, timer);
}
double get_ms_elapsed(struct timespec *start) {
        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);
        return (end.tv_sec - start->tv_sec) * 1e3 + (end.tv_nsec - start->tv_nsec) / 1e6;
}

void setup(void) {
        fill_ansi_color_table();
        input_set_tty_raw();
}

int main(int argc, char **argv) {

        // open all the buffers, storing them in a list of buffers
        if (argc == 1) {
                exit_error("Must pass in a filename to run this editor.");
        }
        buf_list buffers = list_init(buf_list, argc);   // NOLINT
        for (uint8_t i = 1; i < argc; ++i) {
                list_append(buffers, buf_open(argv[i]));  // NOLINT
        }

        // according to the man pages, if NULL, space is allocated for it and a pointer to it is returned
        char *cwd = getcwd(NULL, PATH_MAX + 1);  

        // main loop, handling inputs and etc
        editor_state_t state;
        state.input_history = list_init(dyn_str, 128);
        state.initializing = true;
        state.buf_curr = buffers.len - 1;
        state.mode = NORMAL;
        state.display_state.syntax_mode = HIGH_GRADIENT;
        state.display_state.gradient_color = GRADIENT_PURPLE;

        setup();
        while (true) {

                // if the input was received soon enough after the previous one, 
                // it is reasonable to assume that it is the 27-91-XX combo from hitting arrow keys
                // or another possible input that could mess things up
                set_timer(&state.timer);
                char c = getchar();
                if (c == 'q') {
                        break;
                }
                if (get_ms_elapsed(&state.timer) < 1) {
                        continue;
                }
        
                if (state.initializing) {
                        display_buffer(buffers.items[state.buf_curr], state.mode, &state.display_state);
                        state.initializing = false;
                }
                switch (state.mode) {
                        case NORMAL: 
                        case FILES:
                        case EDIT: break;
                }
        }

        return input_restore_tty();
}
