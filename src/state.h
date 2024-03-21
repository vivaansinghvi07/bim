#include "str.h"

#include <stdbool.h>
#include <time.h>

#ifndef EDITOR_STATE
#define EDITOR_STATE

typedef struct {
        uint8_t r, g, b;
} rgb_t;

typedef enum {
        NORMAL, FILES, EDIT  // files is plural b/c FILE is a kw
} editor_mode;

typedef enum {
        HIGH_ALPHA, HIGH_RANDOM, HIGH_GRADIENT
} highlighting_mode;

typedef struct {
        rgb_t left, right;
} gradient_color_t;

typedef struct {
        highlighting_mode syntax_mode;

        // when syntax_mode is HIGH_GRADIENT, these are the colors that are used
        gradient_color_t gradient_color;  
} display_state_t;

typedef struct {
        size_t buf_curr;
        struct timespec timer; 
        dyn_str input_history;
        editor_mode mode;
        display_state_t display_state;
} editor_state_t;

void parse_config_file(editor_state_t *state);

#endif // !EDITOR_STATE
