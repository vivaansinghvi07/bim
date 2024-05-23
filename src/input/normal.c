#include "normal.h"
#include "esc.h"
#include "command.h"
#include "../list.h"
#include "../mode.h"
#include "../utils.h"
#include "../state.h"
#include "../display/display.h"

#include <ctype.h>
#include <stdint.h>
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

#define C_LEXICAL_SHUF    '?'
#define C_CONFIG_RELOAD   '!'
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

#define C_JUMP_LINE       'l'
#define C_SEARCH          ';'
#define C_BACK_SEARCH     ':'
#define C_JUMP_NEXT       'j'
#define C_JUMP_PREVIOUS   'J'

#define C_TAB_ADD         '>'
#define C_TAB_REMOVE      '<'

#define C_MACRO_LOAD      'M' 
#define C_MACRO_CALL      'm'

#define C_OPEN_FILE       'o'
#define C_ENTER_FILES     'f'

#define C_NEXT_WORD       'n'
#define C_PREVIOUS_WORD   'N'
#define C_DELETE_WORD     'b'
#define C_DELETE_TO_END   'B'

// this is here in order to mimic the behavior of "saving" a column upon going up and down in files
static int prev_col = 0;
void reset_prev_col(void) {
        prev_col = 0;
}
void restore_prev_col(buf_t *buf) {
        if (prev_col == 0) {
                prev_col = buf->cursor_col;
        }

        int curr_line_len = buf->lines.items[buf->cursor_line - 1].len;
        if (prev_col > curr_line_len + 1) {
                buf->cursor_col = curr_line_len + 1;
        } else {
                buf->cursor_col = prev_col;
        }

        if (buf->cursor_col < buf->screen_left_col || buf->cursor_col - buf->screen_left_col >= W()) {
                buf->screen_left_col = max(buf->cursor_col - W() / 2, 1);
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
                int lines_to_move = min(H() / 2, buf->screen_top_line - 1);
                buf->cursor_line -= lines_to_move;
                if (buf->screen_top_line > buf->cursor_line) {
                        buf->screen_top_line -= lines_to_move;
                }
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
                int lines_to_move = min(H() / 2, buf->lines.len - (buf->screen_top_line + H() - 2));
                buf->cursor_line += lines_to_move;
                if (buf->cursor_line - buf->screen_top_line >= H() - 1) {
                        buf->screen_top_line += lines_to_move;
                }
        }
        restore_prev_col(buf);
}

void handle_c_move_left(buf_t *buf) {
        if (buf->cursor_col > 1) {
                if (buf->screen_left_col == buf->cursor_col) {
                        --buf->screen_left_col;
                }
                --buf->cursor_col;
                reset_prev_col();
        }
}

void handle_c_big_move_left(buf_t *buf) {
        if (buf->screen_left_col <= 1) {
                buf->cursor_col = 1;
        } else {
                int cols_to_move = min(W() / 2, buf->screen_left_col - 1);
                buf->cursor_col -= cols_to_move;
                if (buf->screen_left_col > buf->cursor_col) {
                        buf->screen_left_col -= cols_to_move;
                }
        }
        reset_prev_col();
}

void handle_c_move_right(buf_t *buf) {
        if (buf->cursor_col < buf->lines.items[buf->cursor_line - 1].len + 1) {
                if (buf->cursor_col - buf->screen_left_col == W() - 1) {
                        ++buf->screen_left_col;
                }
                ++buf->cursor_col;
                reset_prev_col();
        }
}

void handle_c_big_move_right(buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        if (buf->screen_left_col + W() - 1 >= line->len + 1) {
                buf->cursor_col = line->len + 1;
        } else {
                int cols_to_move = min(W() / 2, line->len - buf->cursor_col + 1);
                cols_to_move = max(0, cols_to_move);
                buf->cursor_col += cols_to_move;
                if (buf->cursor_col - buf->screen_left_col >= W()) {
                        buf->screen_left_col += cols_to_move;
                }
        }
        reset_prev_col();
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

void set_copy_register(editor_state_t *state, const char *to_copy, const size_t n) {
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

        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        set_copy_register(state, line->items, line->len);

        if (buf->lines.len == 1) {
                buf->lines.items[0].len = 0;  // delete without popping line
                restore_prev_col(buf);
                return;
        }

        free_list_items(1, line);
        list_pop(buf->lines, buf->cursor_line - 1);
        if (buf->cursor_line > buf->lines.len) {
                handle_c_move_up(buf);
        }
        restore_prev_col(buf);
}

void handle_c_delete_one(editor_state_t *state, buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        if (line->len <= 0) {
                return;
        }
        set_copy_register(state, line->items + buf->cursor_col - 1, 1);
        list_pop(*line, buf->cursor_col - 1);
        if (buf->cursor_col > line->len) {
                handle_c_move_left(buf);
        }
}

void handle_c_copy_line(editor_state_t *state, buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        set_copy_register(state, line->items, line->len);
}

void handle_c_copy_one(editor_state_t *state, buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        if (buf->cursor_col <= line->len) {
                set_copy_register(state, line->items + buf->cursor_col - 1, 1);
        }
}

ssize_t split_newlines(buf_t *buf, const ssize_t index) {
        dyn_str *line = buf->lines.items + index;
        ssize_t last_newline = line->len;
        ssize_t lines_detected = 0;
        for (ssize_t i = line->len - 1; i > -1; --i) {
                if (line->items[i] == '\n') {
                        list_insert(buf->lines, index + 1, list_init(dyn_str, 128));
                        list_create_space(buf->lines.items[index + 1], last_newline - i - 1);
                        line = buf->lines.items + index;  // potential reallocation in list_create_space moves the memory
                        memcpy(buf->lines.items[index + 1].items, line->items + i + 1, last_newline - i - 1);
                        ++lines_detected;
                        last_newline = i;
                }
        }
        line->len = last_newline;
        return lines_detected;
}

void handle_c_paste_inline(editor_state_t *state, buf_t *buf) {
        if (!state->copy_register.len) {
                return;
        }
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        ssize_t prev_len = line->len;
        list_create_space(*line, state->copy_register.len);

        memmove(line->items + buf->cursor_col - 1 + state->copy_register.len,
                line->items + buf->cursor_col - 1, (prev_len - buf->cursor_col + 1) * sizeof(*line->items));
        memcpy(line->items + buf->cursor_col - 1, state->copy_register.items,
               state->copy_register.len * sizeof(*line->items));

        ssize_t lines_down = split_newlines(buf, buf->cursor_line - 1);

        ssize_t cols_right = 0;
        for (ssize_t i = 0; i < state->copy_register.len; ++i) {
                if (state->copy_register.items[i] == '\n') {
                        cols_right = 0;
                }
                ++cols_right;
        }

        jump_to(buf, &(text_pos_t){.line = buf->cursor_line - 1 + lines_down,
                                   .col = lines_down ? cols_right - 1 : state->copy_register.len + buf->cursor_col - 1});
}

void handle_c_paste_newline(editor_state_t *state, buf_t *buf) {
        if (!state->copy_register.len) {
                return;
        }
        list_insert(buf->lines, buf->cursor_line, list_init(dyn_str, state->copy_register.len));
        list_create_space(buf->lines.items[buf->cursor_line], state->copy_register.len);
        memcpy(buf->lines.items[buf->cursor_line].items, state->copy_register.items, state->copy_register.len);
        split_newlines(buf, buf->cursor_line);
        handle_c_move_down(buf);
}

void handle_forward_search(editor_state_t *state) {
        state->search_forwards = true;
        enter_command_mode(state, CMD_SEARCH);
}

void handle_backward_search(editor_state_t *state) {
        state->search_forwards = false;
        enter_command_mode(state, CMD_SEARCH);
}

list_typedef(dyn_pos, text_pos_t);

void jump_to(buf_t *buf, text_pos_t *pos) {
        buf->cursor_line = pos->line + 1;
        if (buf->cursor_line - buf->screen_top_line >= H() - 1 || buf->cursor_line - buf->screen_top_line < 0) {
                buf->screen_top_line = pos->line > H() - 2 ? pos->line - (H() - 1) / 2 : 1;  
        }
        buf->cursor_col = pos->col + 1;
        if (buf->cursor_col - buf->screen_left_col >= W() || buf->cursor_col - buf->screen_left_col < 0) {
                buf->screen_left_col = pos->col > W() - 1 ? pos->col - W() / 2 : 1;
        }
}

void handle_search_jump(const editor_state_t *state, buf_t *buf, const bool reverse) {

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
                        jump_to(buf, pos);
                        return;
                }
        }
}

void handle_c_jump_next(const editor_state_t *state, buf_t *buf) {
        handle_search_jump(state, buf, !state->search_forwards);
}

void handle_c_jump_previous(const editor_state_t *state, buf_t *buf) {
        handle_search_jump(state, buf, state->search_forwards);
}

void handle_c_open_file(editor_state_t *state) {
        enter_command_mode(state, CMD_OPEN);
}

void handle_c_enter_files(editor_state_t *state) {
        state->mode = FILES;
}

void handle_c_tab_add(editor_state_t *state, buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        list_create_space(*line, state->tab_width);
        memmove(line->items + state->tab_width, line->items, line->len - state->tab_width);
        memset(line->items, ' ', state->tab_width);
}

void handle_c_tab_remove(editor_state_t *state, buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        size_t spaces_to_remove = 0;
        for (; spaces_to_remove < state->tab_width && spaces_to_remove < line->len && line->items[spaces_to_remove] == ' '
             ; ++spaces_to_remove);
        memmove(line->items, line->items + spaces_to_remove, line->len -= spaces_to_remove);
        restore_prev_col(buf);
}

void handle_c_macro_load(editor_state_t *state) {
        if (!state->tracking_macro) {
                state->macro_register.len = 0;
        } else {
                --state->macro_register.len;   // eliminate trailing load terminator
        }
        state->tracking_macro = !state->tracking_macro;
}

void handle_c_macro_call(editor_state_t *state) {
        if (state->tracking_macro) {
                show_error(state, "CANNOT CALL MACRO IN CURRENTLY RECORDING MACRO. RECORD AGAIN");
                state->tracking_macro = false;
                state->macro_register.len = 0;
                return;
        }
        void (*esc_handler)(editor_state_t *, escape_sequence);
        for (ssize_t i = 0; i < state->macro_register.len; ++i) {
                const input_t in = state->macro_register.items[i];
                if (in.is_escape_sequence) {
                        if ((esc_handler = mode_from(state->mode)->escape_sequence_handler)) {
                                esc_handler(state, in.sequence)   ;
                        }
                } else {
                        mode_from(state->mode)->input_handler(state, in.c);
                }
        }
}

static bool passed_whitespace;
bool next_non_whitespace(const char c) {
        return c != ' ';
}

bool next_after_name(const char c) {
        if (c == ' ') {
                passed_whitespace = true;
                return false;
        }
        return passed_whitespace || !is_name_char(c);
}

bool next_after_symbols(const char c) {
        if (c == ' ') {
                passed_whitespace = true;
                return false;
        }
        return passed_whitespace || is_name_char(c);
}

text_pos_t find_next_character(buf_t *buf, bool (*matcher)(const char)) {
        passed_whitespace = false;
        ssize_t x = buf->cursor_col, y = buf->cursor_line - 1;
        while (x != buf->cursor_col - 1 || y != buf->cursor_line - 1) {
                if (x > buf->lines.items[y].len) {
                        x = 0, y = (y + 1) % buf->lines.len;
                        passed_whitespace = true;
                }
                if (matcher(buf->lines.items[y].items[x])) {
                        break;
                }
                ++x;
        }
        return (text_pos_t) { .line = y, .col = x };
}

void handle_c_next_word(buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        char c;
        bool (*matcher)(const char) = buf->cursor_col > line->len || (c = line->items[buf->cursor_col - 1]) == ' ' ? next_non_whitespace
                                    : is_name_char(c) ? next_after_name
                                    : next_after_symbols;
        text_pos_t target = find_next_character(buf, matcher);
        jump_to(buf, &target);
}

bool previous_before_name(const char c) {
        return !is_name_char(c);
}

bool previous_before_symbol(const char c) {
        return is_name_char(c) || c == ' ';
}

// 0 for whitespace, 1 for symbol, 2 for name char - i can't be bothered enuming this
static uint8_t target_symbol_type;
bool previous_non_whitespace(const char c) {
        switch (target_symbol_type) {
                case 0: {
                        if (is_name_char(c)) { target_symbol_type = 2; }
                        else if (c == ' ') { target_symbol_type = 0; }
                        else { target_symbol_type = 1; }
                }; return false;
                case 1: return previous_before_symbol(c);
                case 2: return previous_before_name(c);
        }
        return false;
}

text_pos_t find_previous_character(buf_t *buf, bool (*matcher)(const char)) {
        target_symbol_type = 0;
        bool starting = true;
        ssize_t x = buf->cursor_col - 2, y = buf->cursor_line - 1;
        while (x != buf->cursor_col - 1 || y != buf->cursor_line - 1) {
                if (x < 0) {
                        if (buf->lines.items[y].items[0] != ' ' && !starting) {
                                return (text_pos_t) { .line = y, .col = 0 };
                        }
                        y = (y + buf->lines.len - 1) % buf->lines.len;
                        x = buf->lines.items[y].len - 1;
                }
                if (matcher(buf->lines.items[y].items[x])) {
                        return (text_pos_t) { .line = y, .col = x + 1};
                }
                --x;
                starting = false;
        }
        return (text_pos_t) { .line = y, .col = x};
}

void handle_c_previous_word(buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        char c;
        
        // edge case hell
        bool (*matcher)(const char);
        if (buf->cursor_col > line->len || buf->cursor_col - 1 <= 0) {
                matcher = previous_non_whitespace;
        } else {
                matcher = (c = line->items[buf->cursor_col - 2]) == ' ' ? previous_non_whitespace
                        : is_name_char(c) ? previous_before_name
                        : previous_before_symbol;
        }
        text_pos_t target = find_previous_character(buf, matcher);
        jump_to(buf, &target);
}

bool is_whitespace(char c) {
        return c == ' ';
}

bool is_symbol(char c) {
        return !(is_whitespace(c) || is_name_char(c));
}

void handle_c_delete_word(editor_state_t *state, buf_t *buf) {
        ssize_t start = buf->cursor_col - 1;
        ssize_t end = buf->cursor_col - 1;  // gonna make this inclusive
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        if (buf->cursor_col == line->len + 1) {
                return;
        }
        bool (*matcher)(char) = is_name_char(line->items[start]) ? is_name_char 
                              : is_whitespace(line->items[start]) ? is_whitespace 
                              : is_symbol;
        for (; start > 0 && matcher(line->items[start - 1]); --start);
        for (bool w_passed = false; end < line->len - 1 && (!w_passed && matcher(line->items[end + 1]) 
                                                            || is_whitespace(line->items[end + 1]) && (w_passed = true)); ++end);
        ssize_t len = end - start + 1;
        set_copy_register(state, line->items + start, len);
        memmove(line->items + start, line->items + start + len, line->len - end - 1);
        line->len -= len;
        buf->cursor_col = start + 1;
        restore_prev_col(buf);
}

void handle_c_delete_to_end(buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        line->len = buf->cursor_col - 1;
}

void handle_c_jump_line(editor_state_t *state, buf_t *buf) {
        uint64_t clamped_bottom = max(1, state->number_repeat);
        buf->cursor_line = min(buf->lines.len, clamped_bottom);
        if (buf->cursor_line < buf->screen_top_line || buf->cursor_line - buf->screen_top_line >= H() - 1) {
                buf->screen_top_line = buf->cursor_line - 1 < H() / 2 ? 1 : buf->cursor_line - H() / 2;
        }
        restore_prev_col(buf);
}

void handle_c_numbered_delete_one(editor_state_t *state, buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        size_t len_to_delete = min(line->len - buf->cursor_col + 1, state->number_repeat);
        if (len_to_delete == 0) {
                return;
        }
        set_copy_register(state, line->items + buf->cursor_col - 1, len_to_delete);
        memmove(line->items + buf->cursor_col - 1, line->items + buf->cursor_col - 1 + len_to_delete,
                line->len + - buf->cursor_col + 1 - len_to_delete);
        line->len -= len_to_delete;
}

void handle_c_numbered_copy_one(editor_state_t *state, buf_t *buf) {
        dyn_str *line = buf->lines.items + buf->cursor_line - 1;
        size_t len_to_copy = min(line->len - buf->cursor_col + 1, state->number_repeat);
        if (len_to_copy > 0) {
                set_copy_register(state, line->items + buf->cursor_col - 1, len_to_copy);
        }
}

void handle_c_numbered_delete_line(editor_state_t *state, buf_t *buf) {
        size_t num_to_delete = min(buf->lines.len - buf->cursor_line + 1, state->number_repeat);
        state->copy_register.len = 0;
        for (size_t i = 0; i < num_to_delete; ++i) {
                dyn_str *line = buf->lines.items + buf->cursor_line - 1;
                list_create_space(state->copy_register, line->len);
                memcpy(state->copy_register.items + state->copy_register.len - line->len, line->items, line->len);
                if (buf->lines.len == 1) {
                        line->len = 0;
                } else {
                        free_list_items(1, line);
                        list_pop(buf->lines, buf->cursor_line - 1);
                }
                if (i < num_to_delete - 1) {
                        list_append(state->copy_register, '\n');
                }
        }
        if (buf->cursor_line > buf->lines.len) {
                handle_c_move_up(buf);
        }
        restore_prev_col(buf);
}

void handle_c_numbered_copy_line(editor_state_t *state, buf_t *buf) {
        size_t num_to_copy = min(buf->lines.len - buf->cursor_line + 1, state->number_repeat);
        state->copy_register.len = 0;
        for (size_t i = 0; i < num_to_copy; ++i) {
                dyn_str *line = buf->lines.items + buf->cursor_line - 1 + i;
                list_create_space(state->copy_register, line->len);
                memcpy(state->copy_register.items + state->copy_register.len - line->len, line->items, line->len);
                if (i < num_to_copy - 1) {
                        list_append(state->copy_register, '\n');
                }
        }
}

bool handle_repeats_specially(editor_state_t *state, char c) {
        buf_t *buf = state->buffers->items[state->buf_curr];
        switch (c) {
                case C_JUMP_LINE: handle_c_jump_line(state, buf); break;
                case C_DELETE_ONE: handle_c_numbered_delete_one(state, buf); break;
                case C_DELETE_LINE: handle_c_numbered_delete_line(state, buf); break;
                case C_COPY_ONE: handle_c_numbered_copy_one(state, buf); break;
                case C_COPY_LINE: handle_c_numbered_copy_line(state, buf); break;
                default: return false;
        }
        state->number_repeat = 0;
        return true;
}

void handle_normal_input(editor_state_t *state, char c) {

        buf_t *buf = state->buffers->items[state->buf_curr];
        
        if (isdigit(c)) {
                uint64_t old = state->number_repeat;
                state->number_repeat = state->number_repeat * 10 + (c - '0');
                if (old > state->number_repeat) {
                        show_error(state, "OVERFLOW DETECTED: BE MORE CAREFUL INPUTTING NUMBERS");
                        state->number_repeat = 0;
                }
                return;
        } else if (state->number_repeat) {
                if (!handle_repeats_specially(state, c)) {
                        const uint64_t reps = state->number_repeat;
                        state->number_repeat = 0;
                        for (uint64_t i = 0; i < reps; ++i) {
                                handle_normal_input(state, c);
                        }
                }
                return;
        }

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

                case C_LEXICAL_SHUF: fill_color_tables(); break;
                case C_CONFIG_RELOAD: load_config(state); break;
                case C_DELETE_LINE: handle_c_delete_line(state, buf); break;
                case C_DELETE_ONE: handle_c_delete_one(state, buf); break;
                case C_COPY_LINE: handle_c_copy_line(state, buf); break;
                case C_COPY_ONE: handle_c_copy_one(state, buf); break;
                case C_PASTE_INLINE: handle_c_paste_inline(state, buf); break;
                case C_PASTE_NEWLINE: handle_c_paste_newline(state, buf); break;

                case C_SEARCH: handle_forward_search(state); break;
                case C_BACK_SEARCH: handle_backward_search(state); break;
                case C_JUMP_NEXT: handle_c_jump_next(state, buf); break;
                case C_JUMP_PREVIOUS: handle_c_jump_previous(state, buf); break;
                case C_JUMP_LINE: handle_c_jump_line(state, buf); break;

                case C_OPEN_FILE: handle_c_open_file(state); break;
                case C_ENTER_FILES: handle_c_enter_files(state); break;

                case C_TAB_ADD: handle_c_tab_add(state, buf); break;
                case C_TAB_REMOVE: handle_c_tab_remove(state, buf); break;
                case C_NEXT_WORD: handle_c_next_word(buf); break;
                case C_PREVIOUS_WORD: handle_c_previous_word(buf); break;
                case C_DELETE_WORD: handle_c_delete_word(state, buf); break;
                case C_DELETE_TO_END: handle_c_delete_to_end(buf); break;

                case C_MACRO_CALL: handle_c_macro_call(state); break;
                case C_MACRO_LOAD: handle_c_macro_load(state); break;
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
