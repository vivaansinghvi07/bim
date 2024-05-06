#ifndef EDITOR_INPUT_ESCAPE
#define EDITOR_INPUT_ESCAPE

#include "../state.h"

#include <poll.h>
#include <stddef.h>

escape_sequence parse_escape_sequence(const char *sequence, const size_t len);
void recognize_escape_sequences(editor_state_t *state, struct pollfd *in);

#endif
