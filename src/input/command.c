#include "command.h"
#include "normal.h"
#include "files.h"
#include "../display/display.h"

#include <ctype.h>

#define C_EXIT_COMMAND   '\033'
#define C_COMMAND_ENTER  13
#define C_BACKSPACE      127

void handle_c_exit_command(editor_state_t *state) {
        state->command_target.len = 0;  // making it 0 here too for safety
        state->mode = NORMAL;
}

void handle_open_new_buffer_command(editor_state_t *state) {

        dyn_str *target = &state->command_target;
        for (; target->len && target->items[target->len - 1] == ' '; --target->len);  // strip whitespace lmao
        if (!target->len) {
                return;
        }

        char *filename = malloc((target->len + 1) * sizeof(char));  // will be freed at the end
        memcpy(filename, target->items, target->len);
        filename[target->len] = '\0';

        editor_open_new_buffer(state, filename);
}

void handle_c_rename_file(editor_state_t *state) {
        state->mode = CMD_RENAME;
}

void handle_c_command_enter(editor_state_t *state, buf_t *buf, const int H, const int W) {
        switch (state->mode) {
                case CMD_RENAME: handle_c_rename_file(state); break;
                case CMD_SEARCH: handle_c_jump_next(state, buf, H, W); break; 
                case CMD_OPEN: handle_open_new_buffer_command(state); break;
                default: break;  // should never be reached
        }
        state->mode = NORMAL;
}

void handle_c_command_backspace(editor_state_t *state) {
        if (state->command_target.len) {
                list_pop(state->command_target, state->command_target.len - 1);
        }
}

void handle_c_add_to_command(editor_state_t *state, char c) {
        if (isprint(c)) {
                list_append(state->command_target, c);
        }
}

void handle_command_input(editor_state_t *state, char c) {
        struct winsize w = get_window_size();
        const int W = w.ws_col, H = w.ws_row;
        buf_t *buf = state->buffers->items[state->buf_curr];

        switch (c) {
                case C_EXIT_COMMAND: handle_c_exit_command(state); break;
                case C_COMMAND_ENTER: handle_c_command_enter(state, buf, H, W); break;
                case C_BACKSPACE: handle_c_command_backspace(state); break;
                default: handle_c_add_to_command(state, c); break;
        }
}
