#include "../state.h"
#include "esc.h"

#ifndef EDITOR_INPUT_NORMAL
#define EDITOR_INPUT_NORMAL

void handle_normal_input(editor_state_t *state, char c);
void handle_normal_escape_sequence_input(editor_state_t *state, escape_sequence sequence);
void handle_c_big_move_left(file_buf *buf);
void handle_c_move_right(file_buf *buf, const int W);
void handle_c_move_left(file_buf *buf);
void handle_c_move_up(file_buf *buf);
void handle_c_move_down(file_buf *buf, const int H);

#endif // !EDITOR_INPUT_NORMAL
