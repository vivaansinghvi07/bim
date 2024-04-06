#include "screensaver.h"
#include "display.h"
#include "../utils.h"

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

// in the rock-paper-scissors mode, needs at least this many winning neighbors to change
#define RPS_LOSING_THRESH 3

// in the falling sand mode, this is how many spaces the program is allowed to look in horizontal
#define SAND_HORIZONTAL_SEEK 12

// i hate function pointer syntax
void (*get_ss_func(editor_state_t *state))(cell_t *, const int, const int) {
        switch (state->display_state.screensaver_mode) {
                case SS_LEFT: return &left_slide;
                case SS_RIGHT: return &right_slide;
                case SS_BOTTOM: return &bottom_slide;
                case SS_TOP: return &top_slide;
                case SS_RPS: return &rock_paper_scissors;
                case SS_SAND: return &falling_sand;
                case SS_LIFE: return &game_of_life;
        }
}

typedef struct {
    uint8_t h;
    uint8_t s;
    uint8_t v;
} hsv_t;

// https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
hsv_t rgb_to_hsv(uint8_t r, uint8_t g, uint8_t b) {
        hsv_t hsv;
        uint8_t rgb_min = min(min(r, b), g),
                rgb_max = max(max(r, b), g);
    
        hsv.v = rgb_max;
        if (hsv.v == 0) {
                hsv.h = 0;
                hsv.s = 0;
                return hsv;
        }

        hsv.s = 255 * (rgb_max - rgb_min) / hsv.v;
        if (hsv.s == 0) {
                hsv.h = 0;
                return hsv;
        }

        if (rgb_max == r) {
                hsv.h = 0 + 43 * (g - b) / (rgb_max - rgb_min);
        } else if (rgb_max == g) {
                hsv.h = 85 + 43 * (b - r) / (rgb_max - rgb_min);
        } else {
                hsv.h = 171 + 43 * (r - g) / (rgb_max - rgb_min);
        }

        return hsv;
}

/*
 * <code> is in the ANSI_COLOR_FORMAT, so it has a length of ANSI_ESCAPE_LEN
 * returns a unique ID made from R << 16 + G << 8 + B
 * each R, G, and B are guaranteed by the format to have strlen 3
 */
#define ANSI_R_INDEX 9
#define ANSI_B_INDEX 13
#define ANSI_G_INDEX 17
color_group determine_color_group(const char *code) {
        uint8_t r = strtol(code + ANSI_R_INDEX, NULL, 10);
        uint8_t g = strtol(code + ANSI_G_INDEX, NULL, 10);
        uint8_t b = strtol(code + ANSI_B_INDEX, NULL, 10);
        hsv_t hsv = rgb_to_hsv(r, g, b);

        // these are all magic numbers picked using an HSV color picker
        // i basically eyeballed what the color groups are 
        // the first if check sees if it is unsaturated enough to consider white
        if (hsv.s < 80) {
                return CG_W;
        } else if (hsv.h < 32) {
                return CG_R;
        } else if (hsv.h < 52) {
                return CG_RG;
        } else if (hsv.h < 118) {
                return CG_G;
        } else if (hsv.h < 149) {
                return CG_BG;
        } else if (hsv.h < 193) {
                return CG_B;
        } else if (hsv.h < 223) {
                return CG_RB;
        } else {
                return CG_R;
        }
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
        for (size_t i = 0; i < H - 1; ++i) {
                for (size_t j = 0; j < W; ++j) {
                        cell_t *cell = cells + i * W + j;
                        cell->c = buf_str[(i * W + j) * (ANSI_ESCAPE_LEN + 1) + ANSI_ESCAPE_LEN];
                        cell->ansi_code = (const char *) buf_str + (i * W + j) * (ANSI_ESCAPE_LEN + 1);
                        cell->group = determine_color_group(cell->ansi_code);
                }
        }
        return cells;      
} 

void display_cells(cell_t *cells, const int W, const int H) {
        size_t len = 0;
        char *output = malloc(H * (W + 1) * (ANSI_ESCAPE_LEN + 1) * sizeof(char));  

        for (int y = 0; y < H - 1; ++y) {
                for (int x = 0; x < W; ++x, ++len) {
                        cell_t *cell = cells + y * W + x;
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

void run_screensaver(editor_state_t *state) {

        void (*func)(cell_t *, const int, const int) = get_ss_func(state);
        hide_cursor();
        struct winsize window_size = get_window_size();
        const int W = window_size.ws_col, H = window_size.ws_row;

        // this is not getting freed after build_cells is called because 
        // pieces of it are pointed to in order to signify the ANSI escape 
        const char *buf_str = get_displayed_buffer_string(state);
        cell_t *const cells = build_cells(buf_str);

        struct pollfd in = {.fd = 0, .events = POLLIN};
        while (true) {
                if (poll(&in, 1, state->display_state.screensaver_frame_length_ms)) {
                        getchar(); // this is here to get rid of what's in the poll

                        show_cursor();
                        display_buffer(state);

                        free((void *) buf_str);
                        free(cells);

                        break;
                }
                func(cells, W, H);
                move_to_top_left();
                display_cells(cells, W, H);
        }
}

// i could have combined two of these into one but i think its much more
// readable and managable if its all seperate
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

list_typedef(dyn_cells, const cell_t *);

// i am aware this has a lot of arguments and yes, I could make a 2d point struct but i dont wanna
void life_iterate_cell_at(const cell_t *cells, cell_t *target_cells, const int x,
                          const int y, const int W, const int H) { 
        int total_alive = 0;
        const cell_t *cell = cells + y * W + x;
        cell_t *target_cell = target_cells + y * W + x;
        dyn_cells alive_neighbors = list_init(dyn_cells, 9);  // NOLINT
        for (int i = -1; i < 2; ++i) {
                for (int j = -1; j < 2; ++j) {
                        if (y + i < 0 || y + i > H - 2
                            || x + j < 0 || x + j > W - 1
                            || i == 0 && j == 0) {
                                continue;
                        }
                        const cell_t *neighbor = cells + (y + i) * W + (x + j);
                        total_alive += is_alive(neighbor);
                        if (!is_alive(cell) && is_alive(neighbor)) {
                                list_append(alive_neighbors, neighbor);  // NOLINT
                        }
                }
        }
        
        if (!is_alive(cell) && total_alive == 3) {
                *target_cell = *alive_neighbors.items[arc4random_uniform(alive_neighbors.len)];
        } else if (is_alive(cell) && (total_alive < 2 || total_alive > 3)) {
                *target_cell = (cell_t) {0};
        } else {
                *target_cell = *cell;
        }
        free_list_items(1, &alive_neighbors);
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
                        life_iterate_cell_at(cells, target_cells, x, y, W, H);
                }
        }
        memcpy(cells, target_cells, (H - 1) * W * sizeof(cell_t));
        free(target_cells);
}

// this is how it is. i don't feel like explaining it
bool is_winning_color_group(color_group a, color_group b) {
        switch (a) {
                case CG_W:  return b == CG_B  || b == CG_R  || b == CG_G;
                case CG_R:  return b == CG_RB || b == CG_BG || b == CG_B;
                case CG_G:  return b == CG_RB || b == CG_RG || b == CG_R;
                case CG_B:  return b == CG_RG || b == CG_BG || b == CG_G;
                case CG_RG: return b == CG_R  || b == CG_RB || b == CG_W;
                case CG_RB: return b == CG_B  || b == CG_BG || b == CG_W;
                case CG_BG: return b == CG_G  || b == CG_RG || b == CG_W;
        }
}

list_typedef(group_choices, color_group);

void rps_iterate_cell_at(const cell_t *cells, cell_t *target_cells, const int x,
                         const int y, const int W, const int H) {

        const cell_t *cell = cells + y * W + x;
        cell_t *target_cell = target_cells + y * W + x;
        dyn_cells winners_by_group[COLOR_GROUP_COUNT];
        for (uint8_t g = 0; g < COLOR_GROUP_COUNT; ++g) { 
                winners_by_group[g] = list_init(dyn_cells, 9);  // NOLINT
        }
        uint8_t losses = 0;

        for (int i = -1; i < 2; ++i) {
                for (int j = -1; j < 2; ++j) {
                        if (y + i < 0 || y + i > H - 2
                            || x + j < 0 || x + j > W - 1 
                            || i == 0 && j == 0) {
                                continue;
                        }
                        const cell_t *neighbor = cells + (y + i) * W + (x + j);
                        if (!is_alive(neighbor)) {
                                continue;
                        }
                        
                        bool is_win = (is_winning_color_group(neighbor->group, cell->group)
                                       || !is_alive(cell));
                        if (is_win || is_alive(cell) && neighbor->group == cell->group) {
                                list_append(winners_by_group[neighbor->group], neighbor);  // NOLINT
                                if (is_win) {
                                        ++losses;
                                }
                        } 
                }
        }

        group_choices winning_groups = list_init(group_choices, 4);
        if (!is_alive(cell) && losses >= 1 || losses >= RPS_LOSING_THRESH) {
                uint8_t max_frequency = 0;
                for (int g = 0; g < COLOR_GROUP_COUNT; ++g) {
                        if (cell->group == g && is_alive(cell)) {
                                continue;
                        }
                        uint8_t l = winners_by_group[g].len;
                        if (l > max_frequency) {
                                max_frequency = l;
                        }
                }

                for (int g = 0; g < COLOR_GROUP_COUNT; ++g) {
                        if (winners_by_group[g].len == max_frequency) {
                                list_append(winning_groups, g);
                        }    
                }

                color_group winner = winning_groups.items[arc4random_uniform(winning_groups.len)];
                dyn_cells *possible_cells = winners_by_group + winner;
                *target_cell = *possible_cells->items[arc4random_uniform(possible_cells->len)];
        } else if (is_alive(cell) && winners_by_group[cell->group].len) { 
                dyn_cells *possible_cells = winners_by_group + cell->group;
                *target_cell = *possible_cells->items[arc4random_uniform(possible_cells->len)];
        }

        for (int g = 0; g < COLOR_GROUP_COUNT; ++g) {
                free_list_items(1, winners_by_group + g);
        }
        free_list_items(1, &winning_groups);
}

void rock_paper_scissors(cell_t *cells, const int W, const int H) {
        cell_t *target_cells = malloc((H - 1) * W * sizeof(cell_t));
        memcpy(target_cells, cells, (H - 1) * W * sizeof(cell_t));
        for (int y = 0; y < H - 1; ++y) {
                for (int x = 0; x < W; ++x) {
                        rps_iterate_cell_at(cells, target_cells, x, y, W, H);
                }
        }
        memcpy(cells, target_cells, (H - 1) * W * sizeof(cell_t));
        free(target_cells);
}

void sand_iterate_cell_at(const cell_t *cells, cell_t *target_cells, const int x,
                          const int y, const int W, const int H) {

        cell_t *cell = target_cells + y * W + x; 
        if (!is_alive(cell)) {
                return;
        }
        
        // allow cell to fall if there is space right below
        cell_t *below = target_cells + (y + 1) * W + x;
        if (!is_alive(below)) {
                *below = *cell;
                *cell = (cell_t) {0};
                return;
        }

        // allow cell to fall if there is space right under it
        cell_t *left_below  = target_cells + (y + 1) * W + x - 1,
               *right_below = target_cells + (y + 1) * W + x + 1;
        bool can_go_left = x > 0 && !is_alive(left_below),
             can_go_right = x < W - 1 && !is_alive(right_below);
        if (can_go_left || can_go_right) {
                if (can_go_left && can_go_right) {
                        if (arc4random_uniform(2)) {
                                *right_below = *cell;
                        } else {
                                *left_below = *cell;
                        }
                } else if (can_go_right) {
                        *right_below = *cell;
                } else if (can_go_left) {
                        *left_below = *cell;
                }

                *cell = (cell_t) {0};
                return;
        }

        // iterate cells in either direction if there are any present 
        int l = x - 1;
        int r = x + 1;
        while (l >= 0 || r < W) {
                if (r < W && x < W - 1 && r - x <= SAND_HORIZONTAL_SEEK) {
                        right_below = target_cells + (y + 1) * W + r;
                        if (!is_alive(right_below) && !is_alive(target_cells + y * W + x + 1)) {
                                *(target_cells + y * W + x + 1) = *cell;
                                *cell = (cell_t) {0};
                                return;
                        }
                }
                if (l >= 0 && x > 0 && x - l <= SAND_HORIZONTAL_SEEK) {
                        left_below = target_cells + (y + 1) * W + l;
                        if (!is_alive(left_below) && !is_alive(target_cells + y * W + x - 1)) {
                                *(target_cells + y * W + x - 1) = *cell;
                                *cell = (cell_t) {0};
                                return;
                        }
                } 
                ++r, --l;
        }
}


/*
 * The loop goes from bottom to top to allow cells to fall one layer and leave their spot open.
 * Then it goes from right to left to allow cells to go to the right and leave theirs.
 * This is based on the assumption that code is more frequent on the left of the screen.
 */ 
void falling_sand(cell_t *cells, const int W, const int H) {
        cell_t *target_cells = malloc((H - 1) * W * sizeof(cell_t));
        memcpy(target_cells, cells, (H - 1) * W * sizeof(cell_t));
        for (int y = H - 3; y >= 0; --y) {  // skipping bottom layer
                for (int x = W - 1; x >= 0; --x) { 
                        sand_iterate_cell_at(cells, target_cells, x, y, W, H);
                }
        }
        memcpy(cells, target_cells, (H - 1) * W * sizeof(cell_t));
        free(target_cells);
}
