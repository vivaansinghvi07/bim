#include "src/input/esc.h"
#include "src/list.h"
#include "src/utils.h"
#include "src/buf.h"
#include "src/state.h"
#include "src/display/screensaver.h"
#include "src/display/display.h"
#include "src/input/normal.h"
#include "src/input/edit.h"
#include "src/input/files.h"
#include "src/input/command.h"

#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#define POLL_TIMEOUT_MS 20

void iterate_animated_displays(editor_state_t *state) {
        if (state->display_state.syntax_mode == HIGH_GRADIENT
            && state->display_state.gradient_cycle_duration_ms > 0 
            && get_ms_elapsed(&state->gradient_rotating_timer)
               > state->display_state.gradient_cycle_duration_ms) {
                increment_angle(state);
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

void handle_escape_sequences(editor_state_t *state, struct pollfd *in) {
        dyn_str sequence_str = list_init(dyn_str, 20);
        char c;
        do {
                read(0, &c, 1);
                list_append(sequence_str, c);
        } while (poll(in, 1, 0));
        escape_sequence sequence = parse_escape_sequence(sequence_str.items, sequence_str.len);
        free_list_items(1, &sequence_str);
        switch (state->mode) {
                case NORMAL: handle_normal_escape_sequence_input(state, sequence); break;
                case EDIT: handle_edit_escape_sequence_input(state, sequence); break;
                case FILES: handle_files_escape_sequence_input(state, sequence); break;
                case CMD_SEARCH:
                case CMD_OPEN: break;
        }
        display_by_mode(state);
        clear_error_message(state);
}

int main(const int argc, const char **argv) {

        input_set_tty_raw();
        editor_state_t state;
        struct pollfd in = {.fd = 0, .events = POLLIN};

        setup_state(&state, argc, argv);
        display_by_mode(&state);
        char c;
        bool already_found_error = false;
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
                if (state.mode == NORMAL && (c == 'q' || c == 'Q')) {  // putting this here early
                        if (c == 'Q' || state.buffers->len == 1) {
                                break;
                        }
                        buf_free(state.buffers->items[state.buf_curr]);
                        editor_log("Buffer freed.\n");
                        list_pop(*state.buffers, state.buf_curr);  // NOLINT
                        if (state.buf_curr == state.buffers->len) {
                                --state.buf_curr;
                        }
                }

                if (c == '\033') {
                        if (poll(&in, 1, 0)) {  
                                handle_escape_sequences(&state, &in);
                                continue;
                        }
                }
                       
                switch (state.mode) {
                        case NORMAL: {
                                handle_normal_input(&state, c);
                                list_append(state.input_history, c);
                        } break;
                        case FILES: handle_files_input(&state, c); break;
                        case EDIT: handle_edit_input(&state, c); break;
                        case CMD_OPEN:
                        case CMD_SEARCH: handle_command_input(&state, c); break;
                }
                if (state.error_message.len) {
                        if (already_found_error) {
                                clear_error_message(&state);
                        }
                        already_found_error = !already_found_error;
                }
                display_by_mode(&state);
        }

        buf_free_list(state.buffers);
        move_to_top_left();
        return input_restore_tty();
}
