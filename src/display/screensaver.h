#include "../state.h"
#include "display.h"
#include <stdint.h>

/*
* A collection of cellular-automaton based screen-savers, inspired by:
* https://github.com/Eandrju/cellular-automaton.nvim
*/

#ifndef EDTIOR_SCREENSAVER
#define EDTIOR_SCREENSAVER

typedef struct {

        // a unique ID built from the color of the character
        uint32_t id;

        char c;
        const char *ansi_code;
} cell_t;

void run_screensaver(editor_state_t *state, void (*func)(cell_t *, int, int));
void game_of_life(cell_t *cells, const int W, const int H); 
void falling_sand(cell_t *cells, const int W, const int H); 
void left_slide(cell_t *cells, const int W, const int H); 
void right_slide(cell_t *cells, const int W, const int H); 
void top_slide(cell_t *cells, const int W, const int H); 
void bottom_slide(cell_t *cells, const int W, const int H); 
void rock_paper_scissors(cell_t *cells, const int W, const int H);

#endif
