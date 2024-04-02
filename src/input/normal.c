#include "../state.h"
#include "../display/display.h"

#include <stdio.h>

#define C_MOVE_UP 'w'
#define C_MOVE_LEFT 'a'
#define C_MOVE_DOWN 's'
#define C_MOVE_RIGHT 'd'

#define C_GRAD_ANG_INCRE '['
#define C_GRAD_ANG_DECRE ']'
#define C_BUF_INCRE '+'
#define C_BUF_DECRE '-'


// this is here in order to mimic the behavior of "saving" a column upon going up and down in files
int prev_col = 0;
void handle_new_line_col(file_buf *buf) {
        if (prev_col == 0) {
                prev_col = buf->cursor_col;
        }
        int curr_line_len = buf->lines.items[buf->cursor_line - 1].len;
        if (prev_col > curr_line_len) {
                buf->cursor_col = curr_line_len;
        } else {
                buf->cursor_col = prev_col;
        }
}

void handle_c_move_up(file_buf *buf) {
        if (buf->cursor_line > 1) {
                if (buf->screen_top_line == buf->cursor_line) { 
                        --buf->screen_top_line;
                } 
                --buf->cursor_line;
                handle_new_line_col(buf);
        }
}

void handle_c_move_down(file_buf *buf) {
        struct winsize w = get_window_size();
        if (buf->cursor_line < buf->lines.len) {
                if (buf->screen_top_line + w.ws_row - 2 == buf->cursor_line) {
                        ++buf->screen_top_line;
                }
                ++buf->cursor_line;
                handle_new_line_col(buf);
        }
}

void handle_c_move_left(file_buf *buf) {
        if (buf->cursor_col > 1) {
                --buf->cursor_col;
                prev_col = 0;
        } 
}

void handle_c_move_right(file_buf *buf) {
        if (buf->cursor_col < buf->lines.items[buf->cursor_line - 1].len) {
                ++buf->cursor_col;
                prev_col = 0;
        }
}

void handle_c_buf_incre(editor_state_t *state) {
        state->buf_curr++; 
        state->buf_curr %= state->buffers->len;
}

void handle_c_buf_decre(editor_state_t *state) {
        state->buf_curr += state->buffers->len - 1; 
        state->buf_curr %= state->buffers->len;
}

void handle_normal_input(editor_state_t *state, char c) {

        file_buf *buf = state->buffers->items[state->buf_curr];

        switch (c) {
                case C_MOVE_UP: handle_c_move_up(buf); break;
                case C_MOVE_DOWN: handle_c_move_down(buf); break;
                case C_MOVE_RIGHT: handle_c_move_right(buf); break;
                case C_MOVE_LEFT: handle_c_move_left(buf); break;
                case C_GRAD_ANG_INCRE: increment_gradient(state); break;
                case C_GRAD_ANG_DECRE: decrement_gradient(state); break;
        }
}
