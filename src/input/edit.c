#include "../list.h"
#include "../state.h"
#include "../display/display.h"
#include "normal.h"

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
#define CHAR_BACKSPACE 127

void insert_tab(file_buf *buf, dyn_str *line, const int tab_width) {
        int col = buf->cursor_col - 1;
        size_t spaces_to_add = tab_width - (col % tab_width);
        list_create_space(*line, spaces_to_add);
        memcpy(line->items + col, line->items + col + spaces_to_add, (line->len - col) * sizeof(*line->items));
        memset(line->items + col, ' ', spaces_to_add * sizeof(*line->items));
        buf->cursor_col += spaces_to_add;
}

void insert_newline(file_buf *buf, dyn_str *line) {
        int col = buf->cursor_col - 1;
        size_t split_text_len = line->len - col;
        size_t new_len = max(MIN_NEW_LINE_LEN, split_text_len * 2);   // times 2 for some leeway
        list_insert(buf->lines, buf->cursor_line, list_init(dyn_str, new_len));
        
        // for characters sent into the newline, copy them over
        dyn_str *next_line = buf->lines.items + buf->cursor_line;
        memcpy(next_line->items, line->items + col, split_text_len * sizeof(*line->items));
        memset(line->items + col, 0, split_text_len * sizeof(*line->items));
        next_line->len = split_text_len;
        line->len = col;
        handle_c_move_down(buf); 
        handle_c_big_move_left(buf);
}

void insert_single_character(file_buf *buf, dyn_str *line, char c) {
        int col = buf->cursor_col - 1;
        list_insert(*line, col, c);
        handle_c_move_right(buf);
}

void delete_single_character(file_buf *buf, dyn_str *line) {
        int col = buf->cursor_col - 2;
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
                handle_c_move_up(buf);
                buf->cursor_col = prev_line_prev_len + 1;
                return;
        } else {
                list_pop(*line, col);
                handle_c_move_left(buf);
        }
}

void handle_edit_input(editor_state_t *state, char c) {
        struct winsize w = get_window_size();
        const int W = w.ws_col, H = w.ws_row;
        file_buf *buf = state->buffers->items[state->buf_curr];
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;

        switch (c) {
                case CHAR_CTRL_W: handle_c_move_up(buf); break;
                case CHAR_CTRL_A: handle_c_move_left(buf); break; 
                case CHAR_CTRL_S: handle_c_move_down(buf); break;
                case CHAR_CTRL_D: handle_c_move_right(buf); break; 
                case CHAR_ESCAPE: state->mode = NORMAL; break;
                case CHAR_TAB: insert_tab(buf, line, state->tab_width); break;
                case CHAR_NEWLINE: insert_newline(buf, line); break;
                case CHAR_BACKSPACE: delete_single_character(buf, line); break;
                default: {
                        if (isprint(c)) {
                                insert_single_character(buf, line, c);
                        }
                }
        }
}
