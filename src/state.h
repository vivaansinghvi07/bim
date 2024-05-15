#ifndef EDITOR_STATE
#define EDITOR_STATE

#include "utils.h"
#include "buf.h"

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
        uint8_t r, g, b;
} rgb_t;

typedef enum {
        NORMAL = 0, FILES, EDIT,
        CMD_SEARCH, CMD_FILE_SEARCH, CMD_OPEN, 
        CMD_RENAME, CMD_CREATE, CMD_DEL_CONFIRM
} editor_mode_type_t;
#define EDITOR_MODE_TYPE_COUNT 9   // keep in sync!!

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
        ANG_0, ANG_45, ANG_90, ANG_135,
        ANG_180, ANG_225, ANG_270, ANG_315
} angle_mode;

typedef enum {
        ESC_LEFT_ARROW, ESC_RIGHT_ARROW, ESC_UP_ARROW, ESC_DOWN_ARROW,
        ESC_DELETE_KEY, ESC_NONE
} escape_sequence;

typedef struct {
        highlighting_mode syntax_mode;
        text_style_mode text_style_mode;

        screensaver_mode screensaver_mode;
        ssize_t screensaver_frame_length_ms;
        ssize_t screensaver_ms_inactive;

        // controls the tilt of the gradient when using HIGH_GRADIENT or HIGH_RGB
        // possibly overengineering but its cool
        angle_mode angle;

        // when syntax_mode is HIGH_GRADIENT, these are the colors that are used
        struct {
                gradient_color_t gradient_color;  
                ssize_t gradient_cycle_duration_ms;
        };
        // when syntax_mode is HIGH_RGB, these are the things that are used
        struct {
                ssize_t rgb_cycle_duration_ms;
                ssize_t rgb_state;  // represents how far along we are in rgb process
        };

} display_state_t;

typedef struct {
        bool is_escape_sequence;
        union {
                char c;
                escape_sequence sequence;
        };
} input_t;  // only used in the macro register
list_typedef(dyn_input, input_t);

typedef struct {

        editor_mode_type_t mode;
        int tab_width;

        struct timespec timer; 
        struct timespec inactive_timer; 
        struct timespec rgb_cycle_timer;
        struct timespec gradient_rotating_timer;
        
        display_state_t display_state;

        size_t buf_curr;
        buf_list *buffers;

        dyn_str error_message;
        dyn_str copy_register;
        dyn_str command_target;

        bool tracking_macro;
        dyn_input macro_register;

        bool search_forwards;
        uint64_t number_repeat;        

        buf_t files_view_buf;
} editor_state_t;

bool is_command_mode(editor_mode_type_t mode);
void show_error(editor_state_t *state, const char *format, ...);
void clear_error_message(editor_state_t *state);
void setup_state(editor_state_t *state, const int argc, const char **argv);

#endif // !EDITOR_STATE
