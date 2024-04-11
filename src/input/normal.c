#include "normal.h"

#include "esc.h"
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

#define C_GRAD_ANG_INCRE  '+'
#define C_GRAD_ANG_DECRE  '-'
#define C_BUF_INCRE       ']'
#define C_BUF_DECRE       '['
#define C_ENTER_EDIT      'e'
#define C_SAVE            'z'

#define C_DELETE_LINE     'R'
#define C_DELETE_ONE      'r'
#define C_COPY_LINE       'C'
#define C_COPY_ONE        'c'
#define C_PASTE_NEWLINE   'P'
#define C_PASTE_INLINE    'p'

#define C_SEARCH          ':'

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
        
        // if there are no more "pages" to move up by
        if (buf->screen_top_line <= 1) {
                buf->cursor_line = 1;
        } else {
                int lines_to_move = min(H - 5, buf->screen_top_line - 1);
                buf->screen_top_line -= lines_to_move;
                buf->cursor_line -= lines_to_move;
        }
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

        // already at the bottom-most "page" 
        if (buf->screen_top_line + H - 2 >= buf->lines.len) {
                buf->cursor_line = buf->lines.len;
        } else {
                int lines_to_move = min(H - 5, buf->lines.len - (buf->screen_top_line + H - 2));
                buf->screen_top_line += lines_to_move;
                buf->cursor_line += lines_to_move;
        }
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

void add_to_copy_register(editor_state_t *state, const char *to_copy, const size_t n) {
        if (state->copy_register.len) {
                free_list_items(1, &state->copy_register);
        }
        state->copy_register.items = malloc(n * sizeof(char));
        strncpy(state->copy_register.items, to_copy, n);
        state->copy_register.len = n;
}

void handle_c_delete_line(editor_state_t *state, file_buf *buf) {
        if (buf->lines.len > 1) {
                dyn_str *line = buf->lines.items + buf->cursor_line - 1;
                add_to_copy_register(state, line->items, line->len);
                free_list_items(1, line);

                // this moves first and then deletes in case deleting messes with the moving
                list_pop(buf->lines, buf->cursor_line - 1);
                if (buf->cursor_line > buf->lines.len) {
                        handle_c_move_up(buf);
                }
        }
}

void handle_c_delete_one(editor_state_t *state, file_buf *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        if (line->len > 0) {
                add_to_copy_register(state, line->items + buf->cursor_col - 1, 1);
                list_pop(*line, buf->cursor_col - 1);
                if (buf->cursor_col > line->len) {
                        handle_c_move_left(buf);
                }
        }
}

void handle_c_copy_line(editor_state_t *state, file_buf *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        add_to_copy_register(state, line->items, line->len);
}

void handle_c_copy_one(editor_state_t *state, file_buf *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        if (buf->cursor_col <= line->len) {
                add_to_copy_register(state, line->items + buf->cursor_col - 1, 1);
        }
}

void handle_c_paste_inline(editor_state_t *state, file_buf *buf, const int W) {
        if (!state->copy_register.len) {
                return;
        }
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        size_t prev_len = line->len;
        list_create_space(*line, state->copy_register.len);
        memcpy(line->items + buf->cursor_col - 1 + state->copy_register.len,
               line->items + buf->cursor_col - 1, (prev_len - buf->cursor_col + 1) * sizeof(*line->items));
        memcpy(line->items + buf->cursor_col - 1, state->copy_register.items,
               state->copy_register.len * sizeof(*line->items));
        for (size_t i = 0; i < state->copy_register.len; ++i) {
                handle_c_move_right(buf, W);
        }
}

void handle_c_paste_newline(editor_state_t *state, file_buf *buf, const int H) {
        if (!state->copy_register.len) {
                return;
        }
        list_insert(buf->lines, buf->cursor_line, list_init(dyn_str, state->copy_register.len));
        list_create_space(buf->lines.items[buf->cursor_line], state->copy_register.len);
        memcpy(buf->lines.items[buf->cursor_line].items, state->copy_register.items, state->copy_register.len);
        handle_c_move_down(buf, H);
}

const char *get_search_keyword(void) {
        
}

void handle_c_search(editor_state_t *state, file_buf *buf) {
        
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

                case C_DELETE_LINE: handle_c_delete_line(state, buf); break;
                case C_DELETE_ONE: handle_c_delete_one(state, buf); break;
                case C_COPY_LINE: handle_c_copy_line(state, buf); break;
                case C_COPY_ONE: handle_c_copy_one(state, buf); break;
                case C_PASTE_INLINE: handle_c_paste_inline(state, buf, W); break;
                case C_PASTE_NEWLINE: handle_c_paste_newline(state, buf, H); break;

                default: return;
        }
        display_by_mode(state);
}

void handle_normal_escape_sequence_input(editor_state_t *state, escape_sequence sequence) {

        struct winsize w = get_window_size();
        const int W = w.ws_col, H = w.ws_row;
        file_buf *buf = state->buffers->items[state->buf_curr];

        switch (sequence) {
                case ESC_UP_ARROW: handle_c_move_up(buf); break;
                case ESC_DOWN_ARROW: handle_c_move_down(buf, H); break;
                case ESC_LEFT_ARROW: handle_c_move_left(buf); break;
                case ESC_RIGHT_ARROW: handle_c_move_right(buf, W); break;
                case ESC_DELETE_KEY:
                case ESC_NONE: break;
        }
}

