#include "files.h"
#include "normal.h"
#include "../utils.h"
#include "../state.h"
#include "../display/display.h"

#include <sys/stat.h>

#define C_MOVE_UP       'w'
#define C_MOVE_DOWN     's'
#define C_BIG_MOVE_UP   'W'
#define C_BIG_MOVE_DOWN 'S'

#define C_JUMP_NEXT     'j'
#define C_JUMP_PREVIOUS 'J'
#define C_ENTER_SEARCH  ':'

#define C_REMOVE_FILE   127
#define C_RENAME_FILE   'r'
#define C_MOVE_FILE     'm'
#define C_ENTER_FILE    13
#define C_EXIT_FILES    27 

void open_new_files_view(editor_state_t *state, const char *filename, const int H) {
        buf_t *buf = &state->files_view_buf;
        free((void *) buf->filename);
        buf->filename = filename;
        buf_fill_files_view(&state->files_view_buf);
        buf->cursor_line = min(buf->cursor_line, buf->lines.len);
        if (buf->cursor_line - buf->screen_top_line >= H - 1) {
                 buf->screen_top_line = buf->cursor_line > H / 2 ? buf->cursor_line - H / 2 : 1;
        }
        state->mode = FILES;
}

void editor_open_new_buffer(editor_state_t *state, const char *filename, const int H) {
        
        for (size_t i = 0; i < state->buffers->len; ++i) {
                buf_t *buf = state->buffers->items[i];
                if (is_same_file(buf->filename, filename)) {
                        state->buf_curr = i;
                        return;
                }
        }

        buf_t *buf = buf_open(filename, state->tab_width);
        if (buf == NULL) {
                show_error(state, "INVALID FILE: %s", filename);
                return;
        }
        list_append(*state->buffers, buf);  // NOLINT
        state->buf_curr = state->buffers->len - 1;
        state->mode = NORMAL;
}

void handle_c_enter_file(editor_state_t *state, const int H) {
        
        buf_t *buf = &state->files_view_buf;

        // determine full path being pointed to
        char *path_to_open;
        size_t dirname_len = strlen(buf->filename);
        dyn_str *pathless_filename = buf->lines.items + buf->cursor_line - 1;
        if (is_curr_dir(pathless_filename)) {
                return;
        } else if (is_parent_dir(pathless_filename)) {
                if (dirname_len == 1) {
                        return;
                }
                int i = dirname_len - 2;  // end of dirname guaranteed to be /
                for (; buf->filename[i] != '/'; --i);
                path_to_open = malloc((i + 2) * sizeof(char));
                memcpy(path_to_open, buf->filename, i + 1);
                path_to_open[i + 1] = '\0';
        } else {
                path_to_open = (char *) fill_file_name(buf->filename, pathless_filename);
        }

        if (is_dir(path_to_open)) {
                open_new_files_view(state, path_to_open, H);
        } else {
                editor_open_new_buffer(state, path_to_open, H);
                state->mode = NORMAL;
        }
}

void handle_c_rename(editor_state_t *state) {
        state->command_target.len = 0;
        state->mode = CMD_RENAME;
}

void handle_c_exit_files(editor_state_t *state) {
        if (state->buffers->len) {
                state->mode = NORMAL;
        }
}

void handle_files_input(editor_state_t *state, char c) {

        struct winsize w = get_window_size();
        const int H = w.ws_row, W = w.ws_col;
        buf_t *files_view_buf = &state->files_view_buf;

        switch (c) {
                case C_MOVE_UP: handle_c_move_up(files_view_buf, W); break;
                case C_MOVE_DOWN: handle_c_move_down(files_view_buf, H, W); break;
                case C_ENTER_SEARCH: handle_c_search(state); break;
                case C_JUMP_NEXT: handle_c_jump_next(state, files_view_buf, H, W); break;
                case C_JUMP_PREVIOUS: handle_c_jump_previous(state, files_view_buf, H, W); break;
                
                case C_BIG_MOVE_UP: handle_c_big_move_up(files_view_buf, H, W); break;
                case C_BIG_MOVE_DOWN: handle_c_big_move_down(files_view_buf, H, W); break;
                
                case C_RENAME_FILE: handle_c_rename(state); break;
                case C_ENTER_FILE: handle_c_enter_file(state, H); break;
                case C_EXIT_FILES: handle_c_exit_files(state); break;
        }
}

void handle_files_escape_sequence_input(editor_state_t *state, escape_sequence sequence) {
        struct winsize w = get_window_size();
        const int H = w.ws_row, W = w.ws_col;
        buf_t *files_view_buf = &state->files_view_buf;

        switch (sequence) {
                case ESC_UP_ARROW: handle_c_move_up(files_view_buf, W); break;
                case ESC_DOWN_ARROW: handle_c_move_down(files_view_buf, H, W); break;
                default: break;
        }        
}
