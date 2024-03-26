#include "screensaver.h"
#include "display.h"

#include <unistd.h>
#include <poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Note: BECAUSE W AND H REPRESENT THE DIMENSIONS OF THE SCREEN, 
 * AND THERE EXISTS A BOTTOM BAR, THE DIMENSIONS OF <cells> IS ALWAYS W BY H - 1
 */

// TODO: make this a setting too prob
#define FRAME_LENGTH_MS 20

/*
 * <code> is in the ANSI_CODE_FORMAT, so it has a length of ANSI_CODE_FORMAT
 * returns a unique ID made from R << 16 + G << 8 + B
 * each R, G, and B are guaranteed by the format to have strlen 3
 */
#define ANSI_R_INDEX 9
#define ANSI_B_INDEX 13
#define ANSI_G_INDEX 17
uint32_t get_id_from_color(const char *code) {
        int r = strtol(code + ANSI_R_INDEX, NULL, 10);
        int g = strtol(code + ANSI_G_INDEX, NULL, 10);
        int b = strtol(code + ANSI_B_INDEX, NULL, 10);
        return (r << 16) + (b << 8) + g;
}

cell_t *build_cells(const char *buf_str) {
        struct winsize window_size = get_window_size();
        const int W = window_size.ws_col, H = window_size.ws_row;
        cell_t *cells = malloc((H - 1) * W * sizeof(cell_t));
        bzero(cells, (H - 1) * W * sizeof(cell_t));

        // i am about to write the most god-awful code in order to traverse this string 
        // it will use so many assumptions about string structure, so changing something 
        // in how the `get_displayed_buffer_string` function works prob breaks this
        size_t line_count = 0, col_count = 0, curr = 0;
        while (line_count < H - 1) {

                // line breaks are always in the form of '\n\r', so skip curr by two
                if (buf_str[curr] == '\n') {
                        ++line_count;
                        col_count = 0;
                        curr += 2;
                        continue;
                }

                cell_t *cell = cells + line_count * W + col_count;
                cell->c = buf_str[curr + ANSI_ESCAPE_LEN];
                cell->ansi_code = (const char *) buf_str + curr;
                cell->id = get_id_from_color(cell->ansi_code);

                ++col_count;
                curr += ANSI_ESCAPE_LEN + 1;
        }
        return cells;      
} 

void display_cells(cell_t *cells, const int W, const int H) {
        size_t len = 0;
        char *output = malloc(H * (W + 1) * (ANSI_ESCAPE_LEN + 1) * sizeof(char));  

        for (int i = 0; i < H - 1; ++i) {
                for (int j = 0; j < W; ++j, ++len) {
                        cell_t *cell = cells + i * W + j;
                        if (!cell->c) {
                                output[len] = ' ';
                                continue;
                        }
                        memcpy(output + len, cell->ansi_code, ANSI_ESCAPE_LEN);
                        output[len += ANSI_ESCAPE_LEN] = cell->c;
                }
                output[len++] = '\n';
                output[len++] = '\r';
        }
        output[len] = '\0';
        printf("%s", output);
        fflush(stdout);
        free(output);
}

void run_screensaver(editor_state_t *state, void (*func)(cell_t *, int, int)) {

        hide_cursor();
        struct winsize window_size = get_window_size();
        const int W = window_size.ws_col, H = window_size.ws_row;

        // this is not getting freed after build_cells is called because 
        // pieces of it are pointed to in order to signify the ANSI escape 
        const char *buf_str = get_displayed_buffer_string(state);
        cell_t *const cells = build_cells(buf_str);

        struct pollfd in = {.fd = 0, .events = POLLIN};
        while (true) {
                if (poll(&in, 1, FRAME_LENGTH_MS)) {
                        show_cursor();
                        clear_screen();
                        display_buffer(state);
                        free((char *) buf_str);
                        free(cells);
                        break;
                }
                func(cells, W, H);
                move_to_top_left();
                display_cells(cells, W, H);
        }
}

void left_slide(cell_t *cells, const int W, const int H) {
        for (int i = 0; i < H - 1; ++i) {
                cell_t saved = cells[i * W]; 
                for (int j = 1; j < W; ++j) {
                        cells[i * W + j - 1] = cells[i * W + j];
                }
                cells[(i + 1) * W - 1] = saved;
        }
}

void right_slide(cell_t *cells, const int W, const int H) {
        for (int i = 0; i < H - 1; ++i) {
                cell_t saved = cells[(i + 1) * W - 1]; 
                for (int j = W - 1; j > 0; --j) {
                        cells[i * W + j] = cells[i * W + j - 1];
                }
                cells[i * W] = saved;
        }
}

void bottom_slide(cell_t *cells, const int W, const int H) {
        for (int i = 0; i < W; ++i) {  // for every col
                cell_t saved = cells[(H - 2) * W + i]; 
                for (int j = H - 2; j > 0; --j) { 
                        cells[j * W + i] = cells[(j - 1) * W + i];
                }
                cells[i] = saved;
        }
}

void top_slide(cell_t *cells, const int W, const int H) {
        for (int i = 0; i < W; ++i) {
                cell_t saved = cells[i]; 
                for (int j = 1; j < H - 1; ++j) {
                        cells[(j - 1) * W + i] = cells[j * W + i];
                }
                cells[(H - 2) * W + i] = saved;
        }
}
