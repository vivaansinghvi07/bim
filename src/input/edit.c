#include "../list.h"
#include "../state.h"
#include "../display/display.h"
#include "normal.h"

#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

#define CHAR_CTRL_W 23
#define CHAR_CTRL_A 1
#define CHAR_CTRL_S 19
#define CHAR_CTRL_D 4

#define CHAR_ESCAPE 27
#define CHAR_DELETE 127

void insert_single_character(file_buf *buf, dyn_str *line, char c) {
        if (buf->cursor_col == line->len) {
                list_append(*line, c);
                return;
        } 
        list_append(*line, '\0');
        int col = buf->cursor_col - 1;
        memcpy(line->items + col + 1, line->items + col, line->len - col - 1);
        line->items[col] = c;
        handle_c_move_right(buf);
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
                case CHAR_DELETE: break;
                default: {
                        if (isprint(c)) {
                                insert_single_character(buf, line, c);
                        }
                }
        }
}
