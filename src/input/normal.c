#include "../state.h"
#include "../display/display.h"

#include <stdio.h>

#define C_MOVE_UP 'w'
#define C_MOVE_LEFT 'a'
#define C_MOVE_DOWN 's'
#define C_MOVE_RIGHT 'd'

void handle_c_move_up(file_buf *buf) {
        if (buf->cursor_line > 1) {
                if (buf->screen_top_line == buf->cursor_line) { 
                        --buf->screen_top_line;
                } 
                --buf->cursor_line;
        }
}

void handle_c_move_down(file_buf *buf) {
        struct winsize w = get_window_size();
        if (buf->cursor_line < buf->lines.len) {
                if (buf->screen_top_line + w.ws_row - 2 == buf->cursor_line) {
                        ++buf->screen_top_line;
                }
                ++buf->cursor_line;
        }
}

void handle_normal_input(editor_state_t *state, char c) {

        int x, y;
        store_cursor_pos(&y, &x);
        file_buf *buf = state->buffers->items[state->buf_curr];

        switch (c) {
                case C_MOVE_UP: handle_c_move_up(buf); break;
                case C_MOVE_DOWN: handle_c_move_down(buf); break;
        }

        display_buffer(state);
}
