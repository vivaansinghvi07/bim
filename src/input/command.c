#include "command.h"
#include "normal.h"
#include "files.h"
#include "../mode.h"
#include "../display/display.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define C_EXIT_COMMAND   '\033'
#define C_COMMAND_ENTER  13
#define C_BACKSPACE      127

void handle_c_exit_command(editor_state_t *state) {
        set_cursor_block();
        state->command_target.len = 0;  // making it 0 here too for safety
        state->mode = mode_from(state->mode)->command_destination;
}

void handle_open_command(editor_state_t *state) {

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
                char *real_filename = realpath(filename, NULL);
                filename = append_slash(real_filename);
                open_new_files_view(state, filename);
        } else {
                editor_open_new_buffer(state, filename);
        }
}

void handle_rename_command(editor_state_t *state) {
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
        open_new_files_view(state, strdup(state->files_view_buf.filename));
        free((void *) old_name_str);
        if (!found_same_file) {
                free((void *) new_name_str);
        }
}

void handle_search_command(editor_state_t *state) {
        buf_t *buf = state->buffers->items[state->buf_curr];
        handle_c_jump_next(state, buf);
}

void handle_file_search_command(editor_state_t *state) {
        buf_t *buf = &state->files_view_buf;
        handle_c_jump_next(state, buf);
}

void handle_c_command_enter(editor_state_t *state) {
        mode_from(state->mode)->command_enter_handler(state);
        state->mode = mode_from(state->mode)->command_destination;
        set_cursor_block();
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
        set_cursor_bar();
        switch (c) {
                case C_EXIT_COMMAND: handle_c_exit_command(state); break;
                case C_COMMAND_ENTER: handle_c_command_enter(state); break;
                case C_BACKSPACE: handle_c_command_backspace(state); break;
                default: handle_c_add_to_command(state, c); break;
        }
}
