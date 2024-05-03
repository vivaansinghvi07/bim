#ifndef SEARCH_INPUT_EDIT
#define SEARCH_INPUT_EDIT

#include "../state.h"

void handle_command_input(editor_state_t *state, char c);
void enter_command_mode(editor_state_t *state, const editor_mode_type_t mode);

void handle_delete_confirm_command(editor_state_t *state);
void handle_create_command(editor_state_t *state);
void handle_open_command(editor_state_t *state);
void handle_rename_command(editor_state_t *state);
void handle_search_command(editor_state_t *state);
void handle_file_search_command(editor_state_t *state);

#endif // !SEARCH_INPUT_EDIT
