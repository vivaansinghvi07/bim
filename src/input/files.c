#include "files.h"
#include "normal.h"
#include "../state.h"
#include "../display/display.h"

#define C_MOVE_UP       'w'
#define C_MOVE_DOWN     's'

#define C_REMOVE_FILE   'r'
#define C_MOVE_FILE     'm'
#define C_ENTER_FILE    13
#define C_EXIT_FILES    27 

void handle_files_input(editor_state_t *state, char c) {

        struct winsize w = get_window_size();
        const int H = w.ws_row;
        buf_t *files_view_buf = &state->files_view_buf;

        switch (c) {
                case C_MOVE_UP: handle_c_move_up(files_view_buf); break;
                case C_MOVE_DOWN: handle_c_move_down(files_view_buf, H); break;

                case C_EXIT_FILES: state->mode = NORMAL; break;
        }
}

void handle_files_escape_sequence_input(editor_state_t *state, escape_sequence sequence) {
        
}
