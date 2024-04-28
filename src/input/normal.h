#ifndef EDITOR_INPUT_NORMAL
#define EDITOR_INPUT_NORMAL

#include "../state.h"
#include "esc.h"

void handle_normal_input(editor_state_t *state, char c);
void handle_normal_escape_sequence_input(editor_state_t *state, escape_sequence sequence);
void handle_c_big_move_left(buf_t *buf);
void handle_c_move_right(buf_t *buf, const int W);
void handle_c_move_left(buf_t *buf);
void handle_c_move_up(buf_t *buf, const int W);
void handle_c_big_move_up(buf_t *buf, const int H, const int W);
void handle_c_move_down(buf_t *buf, const int H, const int W);
void handle_c_big_move_down(buf_t *buf, const int H, const int W);
void handle_c_jump_next(const editor_state_t *state, buf_t *buf, const int H, const int W);
void handle_c_jump_previous(const editor_state_t *state, buf_t *buf, const int H, const int W);
void handle_c_search(editor_state_t *state);

#endif // !EDITOR_INPUT_NORMAL
