#include "search.h"
#include "normal.h"
#include "../display/display.h"
#include <ctype.h>

#define C_EXIT_SEARCH   '\033'
#define C_SEARCH_ENTER  13
#define C_BACKSPACE     127

void handle_c_exit_search(editor_state_t *state) {
        state->search_target.len = 0;  // making it 0 here too for safety
        state->mode = NORMAL;
}

void handle_c_search_enter(editor_state_t *state, file_buf *buf, const int H, const int W) {
        state->mode = NORMAL;
        handle_c_jump_next(state, buf, H, W);
}

void handle_c_search_backspace(editor_state_t *state) {
        if (state->search_target.len) {
                list_pop(state->search_target, state->search_target.len - 1);
        }
}

void handle_c_add_to_search(editor_state_t *state, char c) {
        list_append(state->search_target, c);
}

void handle_search_input(editor_state_t *state, char c) {
        struct winsize w = get_window_size();
        const int W = w.ws_col, H = w.ws_row;
        file_buf *buf = state->buffers->items[state->buf_curr];

        switch (c) {
                case C_EXIT_SEARCH: handle_c_exit_search(state); break;
                case C_SEARCH_ENTER: handle_c_search_enter(state, buf, H, W); break;
                case C_BACKSPACE: handle_c_search_backspace(state); break;
                default: {
                        if (isprint(c)) {
                                handle_c_add_to_search(state, c); break;
                        }
                }
        }
}
