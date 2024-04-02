#include "utils.h"
#include "buf.h"

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
        HIGH_ALPHA, HIGH_RANDOM, HIGH_GRADIENT, HIGH_NONE
} highlighting_mode;

// this will only be used in certain scenarios, such as when 
// HIGH_GRADIENT is selected
typedef enum {
        STYLE_BOLD, STYLE_NORMAL, STYLE_ITALIC
} text_style_mode;

typedef enum {
        SS_LEFT, SS_RIGHT, SS_TOP, SS_BOTTOM,
        SS_LIFE, SS_RPS, SS_SAND
} screensaver_mode;

typedef struct {
        rgb_t left, right;
} gradient_color_t;

typedef enum {
        GRAD_ANG_0, GRAD_ANG_45, GRAD_ANG_90, GRAD_ANG_135,
        GRAD_ANG_180, GRAD_ANG_225, GRAD_ANG_270, GRAD_ANG_315
} gradient_angle_mode;

typedef struct {
        highlighting_mode syntax_mode;
        text_style_mode text_style_mode;

        screensaver_mode screensaver_mode;
        int screensaver_frame_length_ms;
        int screensaver_ms_inactive;

        // when syntax_mode is HIGH_GRADIENT, these are the colors that are used
        gradient_color_t gradient_color;  
        gradient_angle_mode gradient_angle;
} display_state_t;

typedef struct {

        struct timespec timer; 
        struct timespec inactive_timer; 

        size_t buf_curr;
        buf_list *buffers;

        dyn_str input_history;
        editor_mode mode;
        char *cwd;

        display_state_t display_state;
} editor_state_t;

uint8_t get_hex_value(char c);
void parse_config_file(editor_state_t *state);

#endif // !EDITOR_STATE
