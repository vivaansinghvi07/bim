#include "files.h"
#include "normal.h"
#include "command.h"
#include "../utils.h"
#include "../state.h"
#include "../display/display.h"

#include <stdio.h>
#include <sys/stat.h>

#define C_MOVE_UP          'w'
#define C_MOVE_DOWN        's'
#define C_BIG_MOVE_UP      'W'
#define C_BIG_MOVE_DOWN    'S'

#define C_JUMP_NEXT        'j'
#define C_JUMP_PREVIOUS    'J'
#define C_ENTER_SEARCH     ';'
#define C_ENTER_REV_SEARCH ':'

#define C_LOAD_MACRO       'M'
#define C_CALL_MACRO       'm'

#define C_CREATE_FILE      'o'
#define C_DELETE_FILE      127
#define C_RENAME_FILE      'r'

#define C_ENTER_FILE       13
#define C_EXIT_FILES       27 
#define C_BACK_DIR         '<'

void open_new_files_view(editor_state_t *state, const char *filename) {
        buf_t *buf = &state->files_view_buf;
        free((void *) buf->filename);
        buf->filename = filename;
        buf_fill_files_view(&state->files_view_buf);
        buf->cursor_line = min(buf->cursor_line, buf->lines.len);
        if (buf->cursor_line - buf->screen_top_line >= H() - 1) {
                 buf->screen_top_line = buf->cursor_line > H() / 2 ? buf->cursor_line - H() / 2 : 1;
        }
        state->mode = FILES;
}

void editor_open_new_buffer(editor_state_t *state, const char *filename) {
        
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

const char *to_parent_dir(const char *path) {
        const size_t len = strlen(path);
        if (len == 1) {
                return strdup(path);
        }
        int i = len - 2;  // end of dirname guaranteed to be /
        for (; path[i] != '/'; --i);
        char *output = malloc((i + 2) * sizeof(char));
        memcpy(output, path, i + 1);
        output[i + 1] = '\0';
        return output;
}

void handle_c_enter_file(editor_state_t *state) {
        
        buf_t *buf = &state->files_view_buf;

        // determine full path being pointed to
        dyn_str *pathless_filename = buf->lines.items + buf->cursor_line - 1;
        if (is_curr_dir(pathless_filename)) {
                return;
        }
                
        const char *path_to_open = is_parent_dir(pathless_filename) ? to_parent_dir(buf->filename)
                                 : fill_file_name(buf->filename, pathless_filename);
        if (is_dir(path_to_open)) {
                open_new_files_view(state, path_to_open);
        } else {
                editor_open_new_buffer(state, path_to_open);
                state->mode = NORMAL;
        }
}

void handle_c_delete_file(editor_state_t *state) {
        enter_command_mode(state, CMD_DEL_CONFIRM);
}

void handle_c_rename(editor_state_t *state) {
        enter_command_mode(state, CMD_RENAME);
}

void handle_c_create(editor_state_t *state) {
        enter_command_mode(state, CMD_CREATE);
}

void handle_c_exit_files(editor_state_t *state) {
        if (state->buffers->len) {
                state->mode = NORMAL;
        }
}

void handle_c_back_dir(editor_state_t *state) {
        open_new_files_view(state, to_parent_dir(state->files_view_buf.filename));
}

void handle_file_search(editor_state_t *state) {
        state->search_forwards = true;
        enter_command_mode(state, CMD_FILE_SEARCH);
}

void handle_rev_file_search(editor_state_t *state) {
        state->search_forwards = false;
        enter_command_mode(state, CMD_FILE_SEARCH);
}

void handle_files_input(editor_state_t *state, char c) {

        buf_t *files_view_buf = &state->files_view_buf;

        switch (c) {
                case C_MOVE_UP: handle_c_move_up(files_view_buf); break;
                case C_MOVE_DOWN: handle_c_move_down(files_view_buf); break;
                case C_ENTER_SEARCH: handle_file_search(state); break;
                case C_ENTER_REV_SEARCH: handle_rev_file_search(state); break;
                case C_JUMP_NEXT: handle_c_jump_next(state, files_view_buf); break;
                case C_JUMP_PREVIOUS: handle_c_jump_previous(state, files_view_buf); break;
                case C_CALL_MACRO: handle_c_macro_call(state); break;
                case C_LOAD_MACRO: handle_c_macro_load(state); break;
        
                case C_BIG_MOVE_UP: handle_c_big_move_up(files_view_buf); break;
                case C_BIG_MOVE_DOWN: handle_c_big_move_down(files_view_buf); break;
                
                case C_CREATE_FILE: handle_c_create(state); break;
                case C_DELETE_FILE: handle_c_delete_file(state); break;
                case C_RENAME_FILE: handle_c_rename(state); break;
                case C_BACK_DIR: handle_c_back_dir(state); break;
                case C_ENTER_FILE: handle_c_enter_file(state); break;
                case C_EXIT_FILES: handle_c_exit_files(state); break;
        }
}

void handle_files_escape_sequence_input(editor_state_t *state, escape_sequence sequence) {

        buf_t *files_view_buf = &state->files_view_buf;

        switch (sequence) {
                case ESC_UP_ARROW: handle_c_move_up(files_view_buf); break;
                case ESC_DOWN_ARROW: handle_c_move_down(files_view_buf); break;
                default: break;
        }        
}
