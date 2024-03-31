#include "src/list.h"
#include "src/utils.h"
#include "src/buf.h"
#include "src/state.h"
#include "src/display/screensaver.h"
#include "src/display/display.h"

#include <poll.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#define POLL_TIMEOUT_MS 20
#define MAX_TIME_INACTIVE_MS 1000

void handle_normal_input(editor_state_t *state, char c) {
        switch (c) {
                case 'j': display_buffer(state);
        }
}

void setup_state(editor_state_t *state, const buf_list *buffers, const char *cwd) {

        fill_ansi_color_table();
        parse_config_file(state);

        state->cwd = (char *) cwd;
        state->input_history = list_init(dyn_str, 128);
        state->buf_curr = buffers->len - 1;
        state->buffers = (buf_list *) buffers;
        state->mode = NORMAL;
}

void init(editor_state_t *state) {
        display_buffer(state);
        set_timer(&state->inactive_timer);
}

int main(int argc, char **argv) {

        input_set_tty_raw();

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
        struct pollfd in = {.fd = 0, .events = POLLIN};
        editor_state_t state;

        setup_state(&state, &buffers, cwd);
        init(&state);
        while (true) {

                set_timer(&state.timer);
                if (!poll(&in, 1, POLL_TIMEOUT_MS)) {
                        if (get_ms_elapsed(&state.inactive_timer) > MAX_TIME_INACTIVE_MS) {
                                run_screensaver(&state);
                                set_timer(&state.inactive_timer);
                        }
                        continue;
                }
                set_timer(&state.inactive_timer);
                char c = getchar(); 
                if (c == 'q') {
                        break;
                }

                // if the input was received soon enough after the previous one, 
                // it is reasonable to assume that it is the 27-91-XX combo from hitting arrow keys
                // or another possible input that could mess things up
                if (get_ms_elapsed(&state.timer) < 1) {
                        continue;
                }
        
                switch (state.mode) {
                        case NORMAL: 
                                handle_normal_input(&state, c);
                        case FILES:
                        case EDIT: break;
                }
        }

        return input_restore_tty();
}
