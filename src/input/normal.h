#ifndef EDITOR_INPUT_NORMAL
#define EDITOR_INPUT_NORMAL

#include "../state.h"
#include "esc.h"

void handle_normal_input(editor_state_t *state, char c);
void handle_normal_escape_sequence_input(editor_state_t *state, escape_sequence sequence);

void handle_c_big_move_left(buf_t *buf);
void handle_c_move_right(buf_t *buf);
void handle_c_move_left(buf_t *buf);
void handle_c_move_up(buf_t *buf);
void handle_c_big_move_up(buf_t *buf);
void handle_c_move_down(buf_t *buf);
void handle_c_big_move_down(buf_t *buf);
void handle_c_jump_next(const editor_state_t *state, buf_t *buf);
void handle_c_jump_previous(const editor_state_t *state, buf_t *buf);
void handle_c_macro_call(editor_state_t *state);
void handle_c_macro_load(editor_state_t *state);
void handle_search(editor_state_t *state);

typedef struct {
        size_t line, col;  // 0 indexed to work with the following loop
} text_pos_t;
void jump_to(buf_t *buf, text_pos_t *pos);

#endif // !EDITOR_INPUT_NORMAL
