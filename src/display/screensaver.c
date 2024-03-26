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
 * <code> is in the ANSI_COLOR_FORMAT, so it has a length of ANSI_ESCAPE_LEN
 * returns a unique ID made from R << 16 + G << 8 + B
 * each R, G, and B are guaranteed by the format to have strlen 3
 */
#define ANSI_R_INDEX 9
#define ANSI_B_INDEX 13
#define ANSI_G_INDEX 17
#define ANSI_STYLE_INDEX 21 
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
                        getchar(); // this is here to get rid of what's in the poll

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
        for (int y = 0; y < H - 1; ++y) {
                cell_t saved = cells[y * W]; 
                for (int x = 1; x < W; ++x) {
                        cells[y * W + x - 1] = cells[y * W + x];
                }
                cells[(y + 1) * W - 1] = saved;
        }
}

void right_slide(cell_t *cells, const int W, const int H) {
        for (int y = 0; y < H - 1; ++y) {
                cell_t saved = cells[(y + 1) * W - 1]; 
                for (int x = W - 1; x > 0; --x) {
                        cells[y * W + x] = cells[y * W + x - 1];
                }
                cells[y * W] = saved;
        }
}

void bottom_slide(cell_t *cells, const int W, const int H) {
        for (int x = 0; x < W; ++x) {  // for every col
                cell_t saved = cells[(H - 2) * W + x]; 
                for (int y = H - 2; y > 0; --y) { 
                        cells[y * W + x] = cells[(y - 1) * W + x];
                }
                cells[x] = saved;
        }
}

void top_slide(cell_t *cells, const int W, const int H) {
        for (int x = 0; x < W; ++x) {
                cell_t saved = cells[x]; 
                for (int y = 1; y < H - 1; ++y) {
                        cells[(y - 1) * W + x] = cells[y * W + x];
                }
                cells[(H - 2) * W + x] = saved;
        }
}

bool is_alive(const cell_t *cell) {
        return !(cell->c == '\0'
                 || cell->c == ' '
                 || cell->c == '\n' 
                 || cell->c == '\t');
}

list_typedef(color_choices_t, const char *);
list_typedef(char_choices_t, char);

// i am aware this has a lot of arguments and yes, I could make a 2d point struct but i dont wanna
void iterate_cell_at(const cell_t *cells, cell_t *target_cells, const int x,
                     const int y, const int W, const int H) {
        int total_alive = 0;
        const cell_t *cell = cells + y * W + x;
        cell_t *target_cell = target_cells + y * W + x;
        color_choices_t colors = list_init(color_choices_t, 9);
        char_choices_t chars = list_init(char_choices_t, 9);
        for (int i = -1; i < 2; ++i) {
                for (int j = -1; j < 2; ++j) {
                        if (y + i < 0 || y + i > H - 2
                            || x + j < 0 || x + j > W - 1
                            || i == 0 && j == 0) {
                                continue;
                        }
                        const cell_t *neighbor = cells + (y + i) * W + (x + j);
                        total_alive += is_alive(neighbor);
                        if (!is_alive(cell) && is_alive(neighbor)) {   // is dead
                                list_append(chars, neighbor->c);
                                list_append(colors, neighbor->ansi_code);
                        }
                }
        }
        
        if (!is_alive(cell) && total_alive == 3) {   // new cell
                const char *color = colors.items[rand() % colors.len];
                *target_cell = (cell_t) {
                        .c = chars.items[rand() % chars.len],
                        .ansi_code = color,
                        .id = get_id_from_color(color)
                };
        } else if (is_alive(cell) && (total_alive < 2 || total_alive > 3)) {  // kill
                *target_cell = (cell_t) {0};
        } else {  // leave current state
                *target_cell = *cell;
        }
        free_list_items(2, &chars, &colors);
}

/*
 * This seems to be pretty computationally expensive because it needs to:
 *   1) Make about 9 comparisons per square on the board, making it O(9n^2)
 *   2) For dead cells, determine the new color traits using random selection,
 *      which happens for every dead cell that becomes alive.
 */
void game_of_life(cell_t *cells, const int W, const int H) {

        cell_t *target_cells = malloc((H - 1) * W * sizeof(cell_t));
        for (int y = 0; y < H - 1; ++y) {
                for (int x = 0; x < W; ++x) {
                        iterate_cell_at(cells, target_cells, x, y, W, H);
                }
        }
        memcpy(cells, target_cells, (H - 1) * W * sizeof(cell_t));
}
