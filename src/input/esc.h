#ifndef EDITOR_INPUT_ESCAPE
#define EDITOR_INPUT_ESCAPE

#include <stddef.h>
typedef enum {
        ESC_LEFT_ARROW, ESC_RIGHT_ARROW, ESC_UP_ARROW, ESC_DOWN_ARROW,
        ESC_DELETE_KEY, ESC_NONE
} escape_sequence;

escape_sequence parse_escape_sequence(const char *sequence, const size_t len);

#endif
