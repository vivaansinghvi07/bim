#ifndef EDITOR_INPUT_EDIT
#define EDITOR_INPUT_EDIT

#include "../state.h"
#include "esc.h"

void handle_edit_input(editor_state_t *state, char c);
void handle_edit_escape_sequence_input(editor_state_t *state, escape_sequence sequence);

#endif // !EDITOR_INPUT_EDIT
