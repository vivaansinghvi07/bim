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

#define C_REMOVE_FILE   127
#define C_RENAME_FILE   'r'
#define C_MOVE_FILE     'm'
#define C_ENTER_FILE    13
#define C_EXIT_FILES    27 

// check if the file is effectively the same - uses stat(2) from the man pages
bool is_same_file(const char *file, const char *other) {
        struct stat file_stat, other_stat;
        stat(file, &file_stat);
        stat(other, &other_stat);
        return file_stat.st_ino == other_stat.st_ino;
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
}

void handle_c_enter_file(editor_state_t *state, const int H) {
        
        buf_t *buf = &state->files_view_buf;

        // determine full path being pointed to
        dyn_str *pathless_filename = buf->lines.items + buf->cursor_line - 1;
        size_t dirname_len = strlen(buf->filename);
        char *path_to_open = malloc((dirname_len + pathless_filename->len + 1) * sizeof(char));
        memcpy(path_to_open, buf->filename, dirname_len);
        memcpy(path_to_open + dirname_len, pathless_filename->items, pathless_filename->len); 
        path_to_open[dirname_len + pathless_filename->len] = '\0';

        if (is_dir(path_to_open)) {
                free((void *) buf->filename);
                buf->filename = path_to_open;
                buf_fill_files_view(&state->files_view_buf);
                buf->cursor_line = min(buf->cursor_line, buf->lines.len);
                if (buf->cursor_line - buf->screen_top_line >= H - 1) {
                         buf->screen_top_line = buf->cursor_line > H / 2 ? buf->cursor_line - H / 2 : 1;
                }
        } else {
                editor_open_new_buffer(state, path_to_open);
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
                
                case C_BIG_MOVE_UP: handle_c_big_move_up(files_view_buf, H, W); break;
                case C_BIG_MOVE_DOWN: handle_c_big_move_down(files_view_buf, H, W); break;
                
                case C_ENTER_FILE: handle_c_enter_file(state, H); break;
                case C_EXIT_FILES: state->mode = NORMAL; break;
        }
}

void handle_files_escape_sequence_input(editor_state_t *state, escape_sequence sequence) {
        
}
