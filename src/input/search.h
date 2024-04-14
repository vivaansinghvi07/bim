#include "../state.h"
#include "esc.h"

#ifndef SEARCH_INPUT_EDIT
#define SEARCH_INPUT_EDIT

void handle_search_input(editor_state_t *state, char c);
void handle_search_escape_sequence_input(editor_state_t *state, escape_sequence sequence);

#endif // !SEARCH_INPUT_EDIT

