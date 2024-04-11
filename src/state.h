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
        HIGH_ALPHA, HIGH_RANDOM, HIGH_GRADIENT, HIGH_NONE, HIGH_RGB
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

        // union here to get marginal memory gains lol
        union {

                // when syntax_mode is HIGH_GRADIENT, these are the colors that are used
                struct {
                        gradient_color_t gradient_color;  
                        gradient_angle_mode gradient_angle;
                        int gradient_cycle_duration_ms;
                };
                // when syntax_mode is HIGH_RGB, these are the things that are used
                struct {
                        int rgb_cycle_duration_ms;
                        int rgb_state;  // represents how far along we are in rgb process
                };
        };

} display_state_t;

typedef struct {

        struct timespec timer; 
        struct timespec inactive_timer; 
        struct timespec rgb_cycle_timer;
        struct timespec gradient_rotating_timer;

        int tab_width;
        
        size_t buf_curr;
        buf_list *buffers;

        dyn_str input_history;
        dyn_str copy_register;
        dyn_str search_register;
        editor_mode mode;
        char *cwd;

        display_state_t display_state;
} editor_state_t;

void load_config(editor_state_t *state);

#endif // !EDITOR_STATE
