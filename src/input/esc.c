/*
 * Small file for dealing with escape sequences
 */

#include "../../include/input/esc.h"

#include <stdlib.h>

// i think it's fine to completely hard code this
// it parses the sequence after the \e, so the sequence for an arrow key:
// \e[A, would only parse [A
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
