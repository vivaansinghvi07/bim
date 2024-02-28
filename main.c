#include "src/list.h"
#include "src/utils.h"
#include "src/buf.h"
#include "src/input.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
typedef enum {
        NORMAL, FILES, EDIT  // files is plural b/c FILE is a kw
} editor_mode;

int main(void) {
    struct timespec start, end;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);
    sleep(2);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Calling the function %.9f seconds later.\n", elapsed);

    clock_gettime(CLOCK_MONOTONIC, &start);
    sleep(3);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Calling the function %.9f seconds later.\n", elapsed);

    clock_gettime(CLOCK_MONOTONIC, &start);
    sleep(4);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Calling the function %.9f seconds later.\n", elapsed);

    clock_gettime(CLOCK_MONOTONIC, &start);
    sleep(5);
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Calling the function %.9f seconds later.\n", elapsed);

    return 0;
}

int maisn(void) {
        input_set_tty_raw();
        int c;
        int counter = 1;
        double start;
        start = (double) clock();
        sleep(10);
        printf("Calling the function %g seconds later.\n\r", ((double) clock() - start) / CLOCKS_PER_SEC);
        while (1) {
                start = (double) clock();
                printf("Starting call to getchar();\n\r");
                c = (int) getchar();
                printf("Calling the function %g seconds later.\n\r", (((double) clock() - start) / CLOCKS_PER_SEC));
                if (counter++ == 100) {
                        break;
                }
                if ((int) (((double) clock() - start) * 1000 / CLOCKS_PER_SEC) < 2) {
                        continue;
                }
                if (c != 'q') {
                        printf("Entered char-code: %d\n\r", c);
                } else {
                        break;
                }
        }
        return input_restore_tty();
}

int chain(int argc, char **argv) {

        // open all the buffers
        if (argc == 1) {
                exit_error("Must pass in a filename to run this editor.");
        }
        buf_list buffers = list_init(buf_list, argc); 
        for (uint8_t i = 1; i < argc; ++i) {
                list_append(buffers, buf_open(argv[i]));
        }

        // main loop, handling inputs and etc
        editor_mode mode = EDIT;
        int buf_curr = buffers.len - 1;
        dyn_str input_history = list_init(dyn_str, 128);
        clock_t start;
        while (1) {
                
                // if the input was received soon enough after the previous one, 
                // it is reasonable to assume that it is the 27-91-XX combo from hitting arrow keys
                // or another possible input that could mess things up
                start = clock();
                char c = getchar();
                if ((clock() - start) * 1000 / CLOCKS_PER_SEC < 2) {
                        continue;
                }

                if (mode == NORMAL || mode == EDIT) {
                        // display_buffer(buffers.items[buf_curr]);
                }
                switch (mode) {
                        case NORMAL: 
                        case FILES:
                        case EDIT: break;
                }
        }
}
