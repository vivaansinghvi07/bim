#include "src/list.h"
#include "src/utils.h"
#include "src/buf.h"

#include <stdint.h>
#include <stdio.h>
#include <Carbon/Carbon.h>

typedef enum {
        NORMAL, FILES, EDIT  // files is plural b/c FILE is a kw
} editor_mode;

typedef enum {
        CTRL, CMD, ALT, SHIFT
} modifier;

typedef struct {
        char key;
        modifier mod;
} input;

input get_input() {
        GetCurrentKeyModifiers();
        return (input) {0};
}

int main(int argc, char **argv) {

        // open all the buffers
        if (argc == 1) {
                exit_error("Must pass in a filename to run this editor.");
        }
        buf_list buffers = list_init(buf_list, argc); 
        for (uint8_t i = 1; i < argc; ++i) {
                list_append(buffers, buf_open(argv[i]));
        }

        // main loop, handling inputs and etc
        editor_mode mode = NORMAL;
        input in;
        while (1) {
                in = get_input();
                switch (mode) {
                        case NORMAL: 
                        case FILES:
                        case EDIT: break;
                }
        }
}
