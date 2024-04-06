#include "normal.h"

#include "../list.h"
#include "../utils.h"
#include "../state.h"
#include "../display/display.h"

#include <stdio.h>

#define C_MOVE_UP         'w'
#define C_MOVE_LEFT       'a'
#define C_MOVE_DOWN       's'
#define C_MOVE_RIGHT      'd'
#define C_BIG_MOVE_UP     'W'
#define C_BIG_MOVE_LEFT   'A'
#define C_BIG_MOVE_DOWN   'S'
#define C_BIG_MOVE_RIGHT  'D'

#define C_GRAD_ANG_INCRE  '['
#define C_GRAD_ANG_DECRE  ']'
#define C_BUF_INCRE       '+'
#define C_BUF_DECRE       '-'
#define C_ENTER_EDIT      'e'
#define C_SAVE            'z'

#define C_DELETE_LINE     'R'
#define C_DELETE_ONE      'r'

// this is here in order to mimic the behavior of "saving" a column upon going up and down in files
int prev_col = 0;
void restore_prev_col(file_buf *buf) {
        if (prev_col == 0) {
                prev_col = buf->cursor_col;
        }
        int curr_line_len = buf->lines.items[buf->cursor_line - 1].len;
        if (prev_col > curr_line_len + 1) {
                buf->cursor_col = curr_line_len + 1;
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
                restore_prev_col(buf);
        }
}

/*
 * This keeps the cursor on the same line on the screen, 
 * but moves the buffer one screen-width up.
 */
void handle_c_big_move_up(file_buf *buf, const int H) {
        int lines_to_move = min(H - 1, buf->screen_top_line - 1);
        buf->screen_top_line -= lines_to_move;
        buf->cursor_line -= lines_to_move;
        restore_prev_col(buf);
}

void handle_c_move_down(file_buf *buf, const int H) {
        if (buf->cursor_line < buf->lines.len) {
                if (buf->screen_top_line + H - 2 == buf->cursor_line) {
                        ++buf->screen_top_line;
                }
                ++buf->cursor_line;
                restore_prev_col(buf);
        }
}

/*
 * This keeps the cursor on the same line on the screen, 
 * but moves the buffer one screen-width down.
 */
void handle_c_big_move_down(file_buf *buf, const int H) {
        int lines_to_move = min(H - 1, buf->lines.len - (buf->screen_top_line + H - 2));
        buf->screen_top_line += lines_to_move;
        buf->cursor_line += lines_to_move;
        restore_prev_col(buf);
}

void handle_c_move_left(file_buf *buf) {
        if (buf->cursor_col > 1) {
                if (buf->screen_left_col == buf->cursor_col) {
                        --buf->screen_left_col;
                }
                --buf->cursor_col;
                prev_col = 0;
        }
}

void handle_c_big_move_left(file_buf *buf) {
        buf->cursor_col = buf->screen_left_col = 1;
        prev_col = 0;
}

void handle_c_move_right(file_buf *buf, const int W) {
        if (buf->cursor_col < buf->lines.items[buf->cursor_line - 1].len + 1) {
                if (buf->cursor_col - buf->screen_left_col == W - 1) {
                        ++buf->screen_left_col;
                }
                ++buf->cursor_col;
                prev_col = 0;
        }
}

void handle_c_big_move_right(file_buf *buf, const int W) {
        buf->cursor_col = buf->lines.items[buf->cursor_line - 1].len + 1;
        if (buf->cursor_col - buf->screen_left_col > W - 1) {
                buf->screen_left_col = buf->cursor_col - W + 1;
        }
        prev_col = 0;
}

void handle_c_buf_incre(editor_state_t *state) {
        state->buf_curr++; 
        state->buf_curr %= state->buffers->len;
}

void handle_c_buf_decre(editor_state_t *state) {
        state->buf_curr += state->buffers->len - 1; 
        state->buf_curr %= state->buffers->len;
}

void handle_c_delete_line(file_buf *buf) {
        if (buf->lines.len > 1) {
                free_list_items(1, buf->lines.items + buf->cursor_line - 1);

                // this moves first and then deletes in case deleting messes with the moving
                list_pop(buf->lines, buf->cursor_line - 1);
                if (buf->cursor_line > buf->lines.len) {
                        handle_c_move_up(buf);
                }
        }
}

void handle_c_delete_one(file_buf *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        if (line->len > 0) {
                list_pop(*line, buf->cursor_col - 1);
                if (buf->cursor_col > line->len) {
                        handle_c_move_left(buf);
                }
        }
}

void handle_normal_input(editor_state_t *state, char c) {

        struct winsize w = get_window_size();
        const int W = w.ws_col, H = w.ws_row;
        file_buf *buf = state->buffers->items[state->buf_curr];

        switch (c) {
                case C_MOVE_UP: handle_c_move_up(buf); break;
                case C_MOVE_DOWN: handle_c_move_down(buf, H); break;
                case C_MOVE_RIGHT: handle_c_move_right(buf, W); break;
                case C_MOVE_LEFT: handle_c_move_left(buf); break;

                case C_BIG_MOVE_UP: handle_c_big_move_up(buf, H); break;
                case C_BIG_MOVE_DOWN: handle_c_big_move_down(buf, H); break;
                case C_BIG_MOVE_RIGHT: handle_c_big_move_right(buf, W); break;
                case C_BIG_MOVE_LEFT: handle_c_big_move_left(buf); break;

                case C_GRAD_ANG_INCRE: increment_gradient(state); break;
                case C_GRAD_ANG_DECRE: decrement_gradient(state); break;

                case C_BUF_INCRE: handle_c_buf_incre(state); break;
                case C_BUF_DECRE: handle_c_buf_decre(state); break;
                case C_ENTER_EDIT: state->mode = EDIT; break;
                case C_SAVE: buf_save(buf); break;

                case C_DELETE_LINE: handle_c_delete_line(buf); break;
                case C_DELETE_ONE: handle_c_delete_one(buf); break;

                default: return;
        }
        display_by_mode(state);
}

