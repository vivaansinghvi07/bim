#include "files.h"
#include "normal.h"
#include "../state.h"
#include "../display/display.h"

#define C_MOVE_UP       'w'
#define C_MOVE_DOWN     's'
#define C_BIG_MOVE_UP   'W'
#define C_BIG_MOVE_DOWN 'S'

#define C_REMOVE_FILE   'r'
#define C_MOVE_FILE     'm'
#define C_ENTER_FILE    13
#define C_EXIT_FILES    27 

bool is_same_filename(const char *filename, dyn_str *other) {
        return false;
}

void handle_c_enter_file(editor_state_t *state) {
        for (size_t i = 0; i < state->buffers->len; ++i) {
                buf_t *buf = state->buffers->items[i];
                if (is_same_filename(buf->filename, &state->command_target)) {
                
                }
        }
}

void handle_files_input(editor_state_t *state, char c) {

        struct winsize w = get_window_size();
        const int H = w.ws_row;
        buf_t *files_view_buf = &state->files_view_buf;

        switch (c) {
                case C_MOVE_UP: handle_c_move_up(files_view_buf); break;
                case C_MOVE_DOWN: handle_c_move_down(files_view_buf, H); break;
                
                case C_BIG_MOVE_UP: handle_c_big_move_up(files_view_buf, H); break;
                case C_BIG_MOVE_DOWN: handle_c_big_move_down(files_view_buf, H); break;
                
                case C_ENTER_FILE: handle_c_enter_file(state); break;
                case C_EXIT_FILES: state->mode = NORMAL; break;
        }
}

void handle_files_escape_sequence_input(editor_state_t *state, escape_sequence sequence) {
        
}
