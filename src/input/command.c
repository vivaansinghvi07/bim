#include "command.h"
#include "normal.h"
#include "files.h"
#include "../display/display.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define C_EXIT_COMMAND   '\033'
#define C_COMMAND_ENTER  13
#define C_BACKSPACE      127

void handle_c_exit_command(editor_state_t *state) {
        state->command_target.len = 0;  // making it 0 here too for safety
        state->mode = NORMAL;
}

void strip_whitespace(dyn_str *target) {
        for (; target->len && target->items[target->len - 1] == ' '; --target->len);  // strip whitespace lmao
}

void handle_open_new_buffer_command(editor_state_t *state, const int H) {

        dyn_str *target = &state->command_target;
        strip_whitespace(target);
        if (!target->len) {
                show_error(state, "CANNOT OPEN FILE WITH NO NAME.");
                return;
        }

        char *filename = malloc((target->len + 1) * sizeof(char));  // will be freed at the end
        memcpy(filename, target->items, target->len);
        filename[target->len] = '\0';

        if (is_dir(filename)) {
                const char *real_filename = realpath(filename, NULL);
                const size_t real_filename_len = strlen(real_filename);
                free(filename);
                filename = malloc((real_filename_len + 2) * sizeof(char));
                memcpy(filename, real_filename, real_filename_len);
                filename[real_filename_len] = '/';
                filename[real_filename_len + 1] = '\0';
                free((void *) real_filename);

                open_new_files_view(state, filename, H);
                state->mode = FILES;
                return;
        }

        editor_open_new_buffer(state, filename, H);
}

bool file_exists(const char *path) {
        FILE *f = fopen(path, "r");
        if (f == NULL) {
                return false;
        }
        fclose(f);
        return true;
}

void handle_c_rename_file(editor_state_t *state) {
        dyn_str *target = &state->command_target;
        strip_whitespace(target);
        if (!target->len) {
                show_error(state, "CANNOT GIVE FILE NO NAME.");
                return;
        }

        const char *new_name_str = fill_file_name(state->files_view_buf.filename, target);
        if (is_dir(new_name_str)) {
                free((void *) new_name_str);
                show_error(state, "IS A DIRECTORY: %s", new_name_str);
                return;
        } else if (file_exists(new_name_str)) {
                free((void *) new_name_str);
                show_error(state, "FILE ALREADY EXISTS: %s", new_name_str);
                return;
        }

        const dyn_str *old_name = state->files_view_buf.lines.items + state->files_view_buf.cursor_line - 1;
        const char *old_name_str = fill_file_name(state->files_view_buf.filename, old_name);
                                                  
        bool found_same_file = false;
        for (size_t i = 0; i < state->buffers->len; ++i) {
                if (is_same_file(old_name_str, state->buffers->items[state->buf_curr]->filename)) {
                        state->buffers->items[state->buf_curr]->filename = new_name_str;
                        found_same_file = true;
                }
        }
        rename(old_name_str, new_name_str);
        free((void *) old_name_str);
        if (found_same_file) {
                free((void *) new_name_str);
        }
}

void handle_c_command_enter(editor_state_t *state, buf_t *buf, const int H, const int W) {
        switch (state->mode) {
                case CMD_RENAME: handle_c_rename_file(state); break;
                case CMD_SEARCH: handle_c_jump_next(state, buf, H, W); break; 
                case CMD_OPEN: handle_open_new_buffer_command(state, H); break;
                default: break;  // should never be reached
        }
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
