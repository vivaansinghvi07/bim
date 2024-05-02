#ifndef SEARCH_INPUT_EDIT
#define SEARCH_INPUT_EDIT

#include "../state.h"

void handle_command_input(editor_state_t *state, char c);

void handle_open_command(editor_state_t *state);
void handle_rename_command(editor_state_t *state);
void handle_search_command(editor_state_t *state);
void handle_file_search_command(editor_state_t *state);

#endif // !SEARCH_INPUT_EDIT
