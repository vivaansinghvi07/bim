#ifndef EDITOR_INPUT_ESCAPE
#define EDITOR_INPUT_ESCAPE

#include "../state.h"

#include <poll.h>
#include <stddef.h>

typedef enum {
        ESC_LEFT_ARROW, ESC_RIGHT_ARROW, ESC_UP_ARROW, ESC_DOWN_ARROW,
        ESC_DELETE_KEY, ESC_NONE
} escape_sequence;

escape_sequence parse_escape_sequence(const char *sequence, const size_t len);
void recognize_escape_sequences(editor_state_t *state, struct pollfd *in);

#endif
