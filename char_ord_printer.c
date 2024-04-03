/*
 * Helper utility to determine what number represents a character.
 */

#include "src/utils.h"

#include <stdio.h>

int main(void) {
        input_set_tty_raw();
        while (1) {
                char c = getchar();
                if (c == 'q') {
                        return input_restore_tty();
                }
                printf("%d\n\r", c);
        }
}
