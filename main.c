#include "src/list.h"
#include "src/utils.h"
#include "src/buf.h"
#include "src/state.h"
#include "src/display/screensaver.h"
#include "src/display/display.h"
#include "src/input/normal.h"
#include "src/input/edit.h"

#include <poll.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#define POLL_TIMEOUT_MS 20

void setup_state(editor_state_t *state, const int argc, const char **argv) {

        fill_ansi_color_table();
        load_config(state);

        // open all buffers passed in cli
        if (argc == 1) {
                exit_error("Must pass in a filename to run this editor.\n\r");
        }
        buf_list *buffers = malloc(sizeof(buf_list));
        *buffers = list_init(buf_list, argc);   // NOLINT
        for (uint8_t i = 1; i < argc; ++i) {
                list_append(*buffers, buf_open(argv[i], state->tab_width));  // NOLINT
        }

        // according to the man pages, if NULL, space is allocated for it and a pointer to it is returned
        const char *cwd = getcwd(NULL, PATH_MAX + 1);  

        state->cwd = (char *) cwd;
        state->input_history = list_init(dyn_str, 128);
        state->copy_register = NULL;
        state->copy_register_len = 0;
        state->buf_curr = buffers->len - 1;
        state->buffers = (buf_list *) buffers;
        state->mode = NORMAL;
}

void init(editor_state_t *state) {
        display_buffer(state);
        set_timer(&state->inactive_timer);
        set_timer(&state->gradient_rotating_timer);
        set_timer(&state->rgb_cycle_timer);
}

void iterate_animated_displays(editor_state_t *state) {
        if (state->display_state.syntax_mode == HIGH_GRADIENT
            && state->display_state.gradient_cycle_duration_ms > 0 
            && get_ms_elapsed(&state->gradient_rotating_timer)
               > state->display_state.gradient_cycle_duration_ms) {
                increment_gradient(state);
                set_timer(&state->gradient_rotating_timer);
        } else if (state->display_state.syntax_mode == HIGH_RGB
                   && state->display_state.rgb_cycle_duration_ms > 0 
                   && get_ms_elapsed(&state->rgb_cycle_timer)
                      > state->display_state.rgb_cycle_duration_ms) {
                set_timer(&state->rgb_cycle_timer);
                step_rgb_state(state);
        } else {
                return;
        }
        display_by_mode(state);
}

int main(const int argc, const char **argv) {

        input_set_tty_raw();
        editor_state_t state;
        struct pollfd in = {.fd = 0, .events = POLLIN};

        setup_state(&state, argc, argv);
        init(&state);
        char c;
        while (true) {

                set_timer(&state.timer);
                iterate_animated_displays(&state);

                if (!poll(&in, 1, POLL_TIMEOUT_MS)) {
                        if (get_ms_elapsed(&state.inactive_timer) > state.display_state.screensaver_ms_inactive) {
                                run_screensaver(&state);
                                set_timer(&state.inactive_timer);
                        }
                        continue;
                }
                set_timer(&state.inactive_timer);
                read(0, &c, 1);
                if (state.mode == NORMAL && c == 'q') {
                        break;
                }

                if (c == '\033') {
                        // weird way of writing this but i need to continue only if polled
                        if (poll(&in, 1, 0)) {  
                                do {
                                        read(0, &c, 1);
                                } while (poll(&in, 1, 0));
                                continue;
                        } 
                }
                       
                switch (state.mode) {
                        case NORMAL: 
                                handle_normal_input(&state, c);
                                list_append(state.input_history, c);
                                break;
                        case FILES: 
                        case EDIT: handle_edit_input(&state, c); break;
                }
        }

        buf_free_list(state.buffers);
        move_to_top_left();
        return input_restore_tty();
}
