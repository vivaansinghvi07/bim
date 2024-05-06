/*
 * Small file for dealing with escape sequences
 */

#include "esc.h"
#include "normal.h"
#include "edit.h"
#include "files.h"
#include "../mode.h"
#include "../display/display.h"

#include <unistd.h>
#include <stdlib.h>

void handle_escape_sequence(editor_state_t *state, dyn_str *sequence_str) {
        escape_sequence sequence = parse_escape_sequence(sequence_str->items, sequence_str->len);
        list_append(state->macro_register, ((input_t) {.is_escape_sequence = true, .sequence = sequence}));
        void (*handler)(editor_state_t *, escape_sequence) = mode_from(state->mode)->escape_sequence_handler;
        if (handler) {
                handler(state, sequence);
        }
}

void recognize_escape_sequences(editor_state_t *state, struct pollfd *in) {
        dyn_str sequence_str = list_init(dyn_str, 20);
        char c;
        do {
                read(0, &c, 1);
                if (c == '\033') {
                        handle_escape_sequence(state, &sequence_str);
                        sequence_str.len = 0;
                } else {
                        list_append(sequence_str, c);
                }
        } while (poll(in, 1, 0));
        
        handle_escape_sequence(state, &sequence_str);
        free_list_items(1, &sequence_str);
        display_buffer(state);
        clear_error_message(state);
}



// i think it's fine to completely hard code this
// it parses the sequence after the \033, so the sequence for an arrow key:
// \033[A, would only parse [A
escape_sequence parse_escape_sequence(const char *sequence, const size_t len) {
        switch (len) {
                case 2: {
                        if (sequence[0] == '[' && sequence[1] == 'A') {
                                return ESC_UP_ARROW;
                        } else if (sequence[0] == '[' && sequence[1] == 'B') {
                                return ESC_DOWN_ARROW;
                        } else if (sequence[0] == '[' && sequence[1] == 'C') {
                                return ESC_RIGHT_ARROW;
                        } else if (sequence[0] == '[' && sequence[1] == 'D') {
                                return ESC_LEFT_ARROW;
                        }
                } break;
                case 3: {
                        if (sequence[0] == '[' && sequence[1] == '3' && sequence[2] == '~') {
                                return ESC_DELETE_KEY; 
                        }
                } break;
        }
        return ESC_NONE;
}
