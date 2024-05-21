/*
 * A file containing some convoluted approach to polymorphism to eliminate the
 * dozens of switch statements I have within my code that merely checks for 
 * `state->mode` and does something or assigns something based off that.
 */

#ifndef EDITOR_MODE
#define EDITOR_MODE

#include "state.h"
#include "input/esc.h"

typedef struct {
        editor_mode_type_t type;
        
        char *title_text;
        size_t title_len;
        
        void (*input_handler)(editor_state_t *state, char c);
        void (*escape_sequence_handler)(editor_state_t *state, escape_sequence sequence);

        bool displays_files;

        editor_mode_type_t command_destination;
        void (*command_enter_handler)(editor_state_t *state);
} editor_mode_t;

void check_mode_array(void);
bool is_command_mode(editor_mode_type_t mode);
const editor_mode_t *mode_from(const editor_mode_type_t mode);

#endif
