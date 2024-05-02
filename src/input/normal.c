#include "normal.h"
#include "esc.h"
#include "../list.h"
#include "../utils.h"
#include "../state.h"
#include "../display/display.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

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
#define C_SAVE_ALL        'Z'

#define C_DELETE_LINE     'R'
#define C_DELETE_ONE      'r'
#define C_COPY_LINE       'C'
#define C_COPY_ONE        'c'
#define C_PASTE_NEWLINE   'P'
#define C_PASTE_INLINE    'p'

#define C_SEARCH          ':'
#define C_JUMP_NEXT       'j'
#define C_JUMP_PREVIOUS   'J'

#define C_OPEN_FILE       'o'
#define C_ENTER_FILES     'f'

// this is here in order to mimic the behavior of "saving" a column upon going up and down in files
static int prev_col = 0, prev_left_col = 0;
void restore_prev_col(buf_t *buf) {
        if (prev_col == 0) {
                prev_col = buf->cursor_col;
                prev_left_col = buf->screen_left_col;
        }

        int curr_line_len = buf->lines.items[buf->cursor_line - 1].len;
        if (prev_col > curr_line_len + 1) {
                buf->cursor_col = curr_line_len + 1;
        } else {
                buf->cursor_col = prev_col;
        }

        if (buf->cursor_col < buf->screen_left_col) {
                buf->screen_left_col = 1;
        } else if (buf->cursor_col > buf->screen_left_col + W() - 1) {
                buf->screen_left_col = prev_left_col;
        }
}

void handle_c_move_up(buf_t *buf) {
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
void handle_c_big_move_up(buf_t *buf) {
        
        // if there are no more "pages" to move up by
        if (buf->screen_top_line <= 1) {
                buf->cursor_line = 1;
        } else {
                int lines_to_move = min(H() - 5, buf->screen_top_line - 1);
                buf->screen_top_line -= lines_to_move;
                buf->cursor_line -= lines_to_move;
        }
        restore_prev_col(buf);
}

void handle_c_move_down(buf_t *buf) {
        if (buf->cursor_line < buf->lines.len) {
                if (buf->screen_top_line + H() - 2 == buf->cursor_line) {
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
void handle_c_big_move_down(buf_t *buf) {

        // already at the bottom-most "page" 
        if (buf->screen_top_line + H() - 2 >= buf->lines.len) {
                buf->cursor_line = buf->lines.len;
        } else {
                int lines_to_move = min(H() - 5, buf->lines.len - (buf->screen_top_line + H() - 2));
                buf->screen_top_line += lines_to_move;
                buf->cursor_line += lines_to_move;
        }
        restore_prev_col(buf);
}

void handle_c_move_left(buf_t *buf) {
        if (buf->cursor_col > 1) {
                if (buf->screen_left_col == buf->cursor_col) {
                        --buf->screen_left_col;
                }
                --buf->cursor_col;
                prev_col = 0;
        }
}

void handle_c_big_move_left(buf_t *buf) {
        buf->cursor_col = buf->screen_left_col = 1;
        prev_col = 0;
}

void handle_c_move_right(buf_t *buf) {
        if (buf->cursor_col < buf->lines.items[buf->cursor_line - 1].len + 1) {
                if (buf->cursor_col - buf->screen_left_col == W() - 1) {
                        ++buf->screen_left_col;
                }
                ++buf->cursor_col;
                prev_col = 0;
        }
}

void handle_c_big_move_right(buf_t *buf) {
        buf->cursor_col = buf->lines.items[buf->cursor_line - 1].len + 1;
        if (buf->cursor_col - buf->screen_left_col > W() - 1) {
                buf->screen_left_col = buf->cursor_col - W() + 1;
        }
        prev_col = 0;
}

void handle_c_enter_edit(editor_state_t *state) {
        state->mode = EDIT;
        set_cursor_bar();
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
        state->copy_register.len = 0;
        list_create_space(state->copy_register, n);
        memcpy(state->copy_register.items, to_copy, n * sizeof(*state->copy_register.items));
}

void handle_c_save_all(editor_state_t *state) {
        for (size_t i = 0; i < state->buffers->len; ++i) {
                buf_save(state->buffers->items[i]);
        }
}

void handle_c_delete_line(editor_state_t *state, buf_t *buf) {
        if (buf->lines.len > 1) {
                dyn_str *line = buf->lines.items + buf->cursor_line - 1;
                add_to_copy_register(state, line->items, line->len);
                free_list_items(1, line);

                // this moves first and then deletes in case deleting messes with the moving
                list_pop(buf->lines, buf->cursor_line - 1);
                if (buf->cursor_line > buf->lines.len) {
                        handle_c_move_up(buf);
                }

                // when going to a new line, make sure the cursor is in the right position
                if (buf->cursor_col > buf->lines.items[buf->cursor_line - 1].len + 1) {
                        buf->cursor_col = buf->lines.items[buf->cursor_line - 1].len + 1;
                }
        }
}

void handle_c_delete_one(editor_state_t *state, buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        if (line->len > 0) {
                add_to_copy_register(state, line->items + buf->cursor_col - 1, 1);
                list_pop(*line, buf->cursor_col - 1);
                if (buf->cursor_col > line->len) {
                        handle_c_move_left(buf);
                }
        }
}

void handle_c_copy_line(editor_state_t *state, buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        add_to_copy_register(state, line->items, line->len);
}

void handle_c_copy_one(editor_state_t *state, buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        if (buf->cursor_col <= line->len) {
                add_to_copy_register(state, line->items + buf->cursor_col - 1, 1);
        }
}

void handle_c_paste_inline(editor_state_t *state, buf_t *buf) {
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
                handle_c_move_right(buf);
        }
}

void handle_c_paste_newline(editor_state_t *state, buf_t *buf) {
        if (!state->copy_register.len) {
                return;
        }
        list_insert(buf->lines, buf->cursor_line, list_init(dyn_str, state->copy_register.len));
        list_create_space(buf->lines.items[buf->cursor_line], state->copy_register.len);
        memcpy(buf->lines.items[buf->cursor_line].items, state->copy_register.items, state->copy_register.len);
        handle_c_move_down(buf);
}

void handle_search(editor_state_t *state) {
        state->command_target.len = 0;
        state->mode = CMD_SEARCH;
}

typedef struct {
        size_t line, col;  // 0 indexed to work with the following loop
} text_pos_t;
list_typedef(dyn_pos, text_pos_t);

void handle_jump(const editor_state_t *state, buf_t *buf, const bool reverse) {

        const dyn_str *target = &state->command_target;
        if (!target->len) {
                return;
        }

        // fill positions of possible matches based off of checking first and last chars
        dyn_pos positions = list_init(dyn_pos, 64);
        for (size_t i = 0; i <= buf->lines.len; ++i) {

                size_t line_index = (i + buf->cursor_line - 1) % buf->lines.len;
                dyn_str *line = buf->lines.items + line_index;
                size_t start = (i == 0) ? buf->cursor_col : 0;
                size_t end = (i == buf->lines.len) ? min(buf->cursor_col - 1, line->len) : line->len;

                if (end < target->len) {
                        continue;
                }

                for (size_t j = start; j < end - target->len + 1; ++j) {
                        bool first_char_match = line->items[j] == target->items[0];
                        bool last_char_match = line->items[j + target->len - 1] == target->items[target->len - 1];
                        if (first_char_match && last_char_match) {
                                list_append(positions, ((text_pos_t) {.line = line_index, .col = j}));
                        }
                }
        }

        // now check for real match - jump if found
        for (size_t i = (reverse ? positions.len - 1 : 0); i >= 0 && i < positions.len; (reverse ? --i : ++i)) {
                text_pos_t *pos = positions.items + i;
                if (!strncmp(target->items, buf->lines.items[pos->line].items + pos->col, target->len)) {
                        buf->cursor_line = pos->line + 1;
                        if (buf->cursor_line - buf->screen_top_line >= H() - 1 
                            || buf->cursor_line - buf->screen_top_line < 0) {
                                buf->screen_top_line = pos->line > H() - 2 ? pos->line - (H() - 1) / 2 : 1;  
                        }
                        buf->cursor_col = pos->col + 1;
                        buf->screen_left_col = pos->col > W() - 1 ? pos->col - W() / 2 : 1;
                        return;
                }
        }
}

void handle_c_jump_next(const editor_state_t *state, buf_t *buf) {
        handle_jump(state, buf, false);
}

void handle_c_jump_previous(const editor_state_t *state, buf_t *buf) {
        handle_jump(state, buf, true);
}

void handle_c_open_file(editor_state_t *state) {
        state->command_target.len = 0;
        state->mode = CMD_OPEN;
}

void handle_c_enter_files(editor_state_t *state) {
        state->mode = FILES;
}

void handle_normal_input(editor_state_t *state, char c) {

        buf_t *buf = state->buffers->items[state->buf_curr];

        switch (c) {
                case C_MOVE_UP: handle_c_move_up(buf); break;
                case C_MOVE_DOWN: handle_c_move_down(buf); break;
                case C_MOVE_RIGHT: handle_c_move_right(buf); break;
                case C_MOVE_LEFT: handle_c_move_left(buf); break;

                case C_BIG_MOVE_UP: handle_c_big_move_up(buf); break;
                case C_BIG_MOVE_DOWN: handle_c_big_move_down(buf); break;
                case C_BIG_MOVE_RIGHT: handle_c_big_move_right(buf); break;
                case C_BIG_MOVE_LEFT: handle_c_big_move_left(buf); break;

                case C_GRAD_ANG_INCRE: increment_angle(state); break;
                case C_GRAD_ANG_DECRE: decrement_angle(state); break;

                case C_BUF_INCRE: handle_c_buf_incre(state); break;
                case C_BUF_DECRE: handle_c_buf_decre(state); break;
                case C_ENTER_EDIT: handle_c_enter_edit(state); break;
                case C_SAVE: buf_save(buf); break;
                case C_SAVE_ALL: handle_c_save_all(state); break;

                case C_DELETE_LINE: handle_c_delete_line(state, buf); break;
                case C_DELETE_ONE: handle_c_delete_one(state, buf); break;
                case C_COPY_LINE: handle_c_copy_line(state, buf); break;
                case C_COPY_ONE: handle_c_copy_one(state, buf); break;
                case C_PASTE_INLINE: handle_c_paste_inline(state, buf); break;
                case C_PASTE_NEWLINE: handle_c_paste_newline(state, buf); break;

                case C_SEARCH: handle_search(state); break;
                case C_JUMP_NEXT: handle_c_jump_next(state, buf); break;
                case C_JUMP_PREVIOUS: handle_c_jump_previous(state, buf); break;

                case C_OPEN_FILE: handle_c_open_file(state); break;
                case C_ENTER_FILES: handle_c_enter_files(state); break;

                default: return;  // nothing changes, don't waste time displaying
        }
}

void handle_normal_escape_sequence_input(editor_state_t *state, escape_sequence sequence) {

        buf_t *buf = state->buffers->items[state->buf_curr];

        switch (sequence) {
                case ESC_UP_ARROW: handle_c_move_up(buf); break;
                case ESC_DOWN_ARROW: handle_c_move_down(buf); break;
                case ESC_LEFT_ARROW: handle_c_move_left(buf); break;
                case ESC_RIGHT_ARROW: handle_c_move_right(buf); break;
                case ESC_DELETE_KEY:
                case ESC_NONE:
                default: return;
        }
}
