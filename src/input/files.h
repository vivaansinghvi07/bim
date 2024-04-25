#include "../state.h"
#include "esc.h"

#ifndef EDITOR_FILES_EDIT
#define EDITOR_FILES_EDIT

void editor_open_new_buffer(editor_state_t *state, const char *filename);
void handle_files_input(editor_state_t *state, char c);
void handle_files_escape_sequence_input(editor_state_t *state, escape_sequence sequence);

#endif // !EDITOR_FILES_EDIT
