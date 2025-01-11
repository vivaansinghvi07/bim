#include "edit.h"
#include "esc.h"
#include "normal.h"
#include "../list.h"
#include "../state.h"
#include "../display/display.h"

#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

#define CHAR_CTRL_W 23
#define CHAR_CTRL_A 1
#define CHAR_CTRL_S 19
#define CHAR_CTRL_D 4

#define CHAR_TAB 9
#define CHAR_NEWLINE 13
#define CHAR_ESCAPE 27
#define CHAR_CTRL_E 5
#define CHAR_BACKSPACE 127

void insert_tab(buf_t *buf, dyn_str *line, const int tab_width) {
        int col = buf->cursor_col - 1;
        size_t spaces_to_add = tab_width - (col % tab_width);
        list_create_space(*line, spaces_to_add);
        memcpy(line->items + col + spaces_to_add, line->items + col, (line->len - spaces_to_add - col) * sizeof(*line->items));
        memset(line->items + col, ' ', spaces_to_add * sizeof(*line->items));
        buf->cursor_col += spaces_to_add;
        if (buf->cursor_col - buf->screen_left_col > W() - 1) {
                buf->screen_left_col += spaces_to_add;
        }
}

void insert_single_character(buf_t *buf, dyn_str *line, char c) {
        int col = buf->cursor_col - 1;
        list_insert(*line, col, c);
        handle_c_move_right(buf);
}

bool is_open_paren_char(const char c) {
        return c == '(' || c == '[' || c == '{';
}

void insert_newline(buf_t *buf, dyn_str *line, const int tab_width) {

        int col = buf->cursor_col - 1;
        size_t split_text_len = line->len - col;
        size_t new_len = max(MIN_NEW_LINE_LEN, split_text_len * 2);   // times 2 for some leeway
        list_insert(buf->lines, buf->cursor_line, list_init(dyn_str, new_len));
        line = buf->lines.items + buf->cursor_line - 1;   // in case of reallocation
        
        // for characters sent into the newline, copy them over
        dyn_str *next_line = buf->lines.items + buf->cursor_line;
        memcpy(next_line->items, line->items + col, split_text_len * sizeof(*line->items));
        memset(line->items + col, 0, split_text_len * sizeof(*line->items));
        next_line->len = split_text_len;
        line->len = col;
        handle_c_move_down(buf);
        buf->cursor_col = buf->screen_left_col = 1;

        ssize_t spaces_to_add = 0;
        for (;spaces_to_add < line->len && line->items[spaces_to_add] == ' '; ++spaces_to_add);
        spaces_to_add = spaces_to_add + (is_open_paren_char(line->items[col - 1])) * tab_width;
        if (spaces_to_add > 0) {
                list_create_space(*next_line, spaces_to_add);
                memmove(next_line->items, next_line->items + spaces_to_add, next_line->len - spaces_to_add);
                memset(next_line->items, ' ', spaces_to_add);
                jump_to(buf, &(text_pos_t){ .col = buf->cursor_col + spaces_to_add - 1, .line = buf->cursor_line - 1});
        } else {
                reset_prev_col();
        }
}

void delete_single_character(buf_t *buf, dyn_str *line, const int tab_width) {
        int col = buf->cursor_col - 2;

        // deletion is at the beginning of a line
        if (col < 0) {
                if (buf->cursor_line == 1) {
                        return;
                }
                dyn_str *prev_line = buf->lines.items + buf->cursor_line - 2;
                size_t prev_line_prev_len = prev_line->len;
                list_create_space(*prev_line, line->len);
                memcpy(prev_line->items + prev_line_prev_len, line->items, line->len * sizeof(*prev_line->items));
                free_list_items(1, line);
                list_pop(buf->lines, buf->cursor_line - 1);
                jump_to(buf, &(text_pos_t){.line = buf->cursor_line - 2, .col = prev_line_prev_len});
        } else {
                int deletes = 1;
                if (line->items[col] == ' ') {
                        int max_to_delete = col % tab_width + 1;
                        for (; deletes < max_to_delete && line->items[col - deletes] == ' '; ++deletes);
                }
                while (deletes-->0) {
                        list_pop(*line, buf->cursor_col - 2);
                        handle_c_move_left(buf);
                }
        }
}

void handle_exit_edit(editor_state_t *state) {
        state->mode = NORMAL;
        set_cursor_block();
}

void handle_edit_input(editor_state_t *state, char c) {

        buf_t *buf = state->buffers->items[state->buf_curr];
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;

        switch (c) {
                case CHAR_CTRL_W: handle_c_move_up(buf); break;
                case CHAR_CTRL_A: handle_c_move_left(buf); break; 
                case CHAR_CTRL_S: handle_c_move_down(buf); break;
                case CHAR_CTRL_D: handle_c_move_right(buf); break; 
                case CHAR_CTRL_E: case CHAR_ESCAPE: handle_exit_edit(state); break;
                case CHAR_TAB: insert_tab(buf, line, state->tab_width); break;
                case CHAR_NEWLINE: insert_newline(buf, line, state->tab_width); break;
                case CHAR_BACKSPACE: delete_single_character(buf, line, state->tab_width); break;
                default: {
                        if (!isprint(c)) {
                                return;
                        }
                        insert_single_character(buf, line, c);
                }
        }
}

void handle_esc_delete_key(buf_t *buf, dyn_str *line) {
        int col = buf->cursor_col - 1;
        if (col == line->len) {
                if (buf->cursor_line == buf->lines.len) {
                        return;
                }
                dyn_str *next_line = buf->lines.items + buf->cursor_line;
                list_create_space(*line, next_line->len);
                memcpy(line->items + col, next_line->items,
                       next_line->len * sizeof(*line->items)); 
                free_list_items(1, next_line);
                list_pop(buf->lines, buf->cursor_line);
        } else {
                list_pop(*line, col);
        }
}

void handle_edit_escape_sequence_input(editor_state_t *state, escape_sequence sequence) {

        buf_t *buf = state->buffers->items[state->buf_curr];
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;

        switch (sequence) {
                case ESC_UP_ARROW: handle_c_move_up(buf); break;
                case ESC_DOWN_ARROW: handle_c_move_down(buf); break;
                case ESC_LEFT_ARROW: handle_c_move_left(buf); break;
                case ESC_RIGHT_ARROW: handle_c_move_right(buf); break;
                case ESC_DELETE_KEY: handle_esc_delete_key(buf, line); break;
                case ESC_NONE: 
                default: return;
        }
}
