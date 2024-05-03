#include "state.h"
#include "buf.h"
#include "utils.h"
#include "mode.h"
#include "input/command.h"
#include "display/display.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

/* State-related functions */

bool is_command_mode(editor_mode_type_t mode) {
        return mode_from(mode)->input_handler == handle_command_input;
}

void show_error(editor_state_t *state, const char *format, ...) {
        clear_error_message(state);
        va_list arg_ptr, other_args;
        va_start(arg_ptr, format);
        va_copy(other_args, arg_ptr);
        size_t len = vsnprintf(NULL, 0, format, arg_ptr);
        list_create_space(state->error_message, len + 1);
        vsnprintf(state->error_message.items, len + 1, format, other_args);
        --state->error_message.len;  // removes the null terminator
        va_end(arg_ptr);
}

void clear_error_message(editor_state_t *state) {
        state->error_message.len = 0;
}

// forward decl, i want the state function on the top here
void load_config(editor_state_t *state);

void setup_state(editor_state_t *state, const int argc, const char **argv) {

        check_mode_array();
        fill_ansi_color_table();
        load_config(state);

        // open all buffers passed in cli
        buf_list *buffers = malloc(sizeof(buf_list));
        *buffers = list_init(buf_list, argc);   // NOLINT
        for (uint8_t i = 1; i < argc; ++i) {
                buf_t *buf = buf_open(argv[i], state->tab_width); 
                if (buf == NULL) {
                        exit_error("Invalid file name passed into editor: %s\n", argv[i]);
                }
                list_append(*buffers, buf);  // NOLINT
        }
        
        const char *cwd = append_slash(getcwd(NULL, 0));
        buf_init(&state->files_view_buf, cwd);
        buf_fill_files_view(&state->files_view_buf);                

        state->input_history = list_init(dyn_str, 128);
        state->command_target = list_init(dyn_str, 128);
        state->copy_register = list_init(dyn_str, 256);
        state->error_message = list_init(dyn_str, 128);
        state->buf_curr = buffers->len - 1;
        state->buffers = (buf_list *) buffers;
        state->mode = argc == 1 ? FILES : NORMAL;
        state->search_forwards = true;

        set_timer(&state->inactive_timer);
        set_timer(&state->gradient_rotating_timer);
        set_timer(&state->rgb_cycle_timer);
}

/* Config file-related functions */

#define CONFIG_FILE_NAME ".stupid_editor_rc"

#define GRADIENT_LEFT_SETTING "gradient_left"
#define GRADIENT_RIGHT_SETTING "gradient_right"
#define TEXT_STYLE_SETTING "text_style"
#define HIGHLIGHT_MODE_SETTING "highlight_mode"
#define SCREENSAVER_MODE_SETTING "screensaver_mode"
#define GRADIENT_ANGLE_SETTING "gradient_angle"
#define RGB_ANGLE_SETTING "rgb_angle"
#define SCREENSAVER_MS_INACTIVE "screensaver_ms_inactive"
#define SCREENSAVER_FRAME_LENGTH_SETTING "screensaver_frame_length_ms"
#define GRADIENT_CYCLE_DURATION_MS "gradient_cycle_duration_ms"
#define RGB_CYCLE_DURATION_MS "rgb_cycle_duration_ms"
#define TAB_WIDTH_SETTING "tab_width"

const char *HIGH_STR_OPTS[] = {"GRADIENT", "LEXICAL", "RANDOM", "NONE", "RGB"};
const highlighting_mode HIGH_ENUM_OPTS[] = {HIGH_GRADIENT, HIGH_ALPHA, HIGH_RANDOM, HIGH_NONE, HIGH_RGB};

const char *STYLE_STR_OPTS[] = {"BOLD", "NORMAL", "ITALIC"};
const text_style_mode STYLE_ENUM_OPTS[] = {STYLE_BOLD, STYLE_NORMAL, STYLE_ITALIC};

const char *SS_STR_OPTS[] = {"LEFT_SLIDE", "RIGHT_SLIDE", "TOP_SLIDE", "BOTTOM_SLIDE",
                            "ROCK_PAPER_SCISSORS", "GAME_OF_LIFE", "FALLING_SAND"};
const screensaver_mode SS_ENUM_OPTS[] = {SS_LEFT, SS_RIGHT, SS_TOP, SS_BOTTOM,
                                         SS_RPS, SS_LIFE, SS_SAND};

const char *ANG_STR_OPTS[] = {"0", "45", "90", "135", "180", "225", "270", "315"};
const angle_mode ANG_ENUM_OPTS[] = {ANG_0, ANG_45, ANG_90, ANG_135,
                                         ANG_180, ANG_225, ANG_270, ANG_315};

#define DEFAULT_GRAD_LEFT {255, 255, 0}
#define DEFAULT_GRAD_RIGHT {0, 255, 255}

#define parse_text_opts(setting, val_to_set, str_opts_arr, enum_opts_arr, info)                     \
        do {                                                                                        \
                const char *ending_str = (info).line->items + (info).equal_index + 1;               \
                size_t len = (info).line->len - (info).equal_index - 1;                             \
                strip_whitespace((info).line);                                                      \
                                                                                                    \
                int i;                                                                              \
                for (i = 0; i < sizeof(str_opts_arr) / sizeof(*str_opts_arr); ++i) {                \
                        if (!strncmp(ending_str, str_opts_arr[i], len)) {                           \
                                val_to_set = enum_opts_arr[i];                                      \
                                break;                                                              \
                        }                                                                           \
                }                                                                                   \
                                                                                                    \
                if (i == sizeof(str_opts_arr) / sizeof(*str_opts_arr)) {                            \
                        char invalid[len + 1];                                                      \
                        memcpy(invalid, ending_str, len);                                           \
                        invalid[len] = '\0';                                                        \
                        printf("Invalid setting for '%s' detected: %s.\n\r", setting, invalid);     \
                        printf("Possible options include:\n\r");                                    \
                        for (i = 0; i < sizeof(str_opts_arr) / sizeof(*str_opts_arr); ++i) {        \
                                printf(" - %s\n\r", str_opts_arr[i]);                               \
                        }                                                                           \
                        input_restore_tty();                                                        \
                        exit(1);                                                                    \
                }                                                                                   \
        } while (0)

// source: https://stackoverflow.com/a/78195956
char *get_config_path(void) {
        char seperator;
        const char *home_path;
        const char *config_name = CONFIG_FILE_NAME;
#ifdef _WIN32
        home_path = getenv("USERPROFILE");
        seperator = '\\'
#else
        home_path = getenv("HOME");
        seperator = '/';
#endif
        char *config_path = malloc((strlen(home_path) + strlen(config_name) + 1) * sizeof(char));
        sprintf(config_path, "%s%c%s", home_path, seperator, config_name);
        FILE *config_file = fopen(config_path, "r");

        if (config_file) {
                fclose(config_file);
                return config_path;
        }
        return NULL;
}

// making this a struct so that i can change it if needed
typedef struct {
        dyn_str *line; 
        size_t equal_index;
} parse_info_t;

bool is_valid_color_char(const char c) {
        return isdigit(c) || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F';
}

rgb_t parse_color(const parse_info_t *info) {
        if (info->line->items[info->equal_index + 1] != '#') {
                exit_error("Color must begin with a hex code.");
        } else if (info->line->len - info->equal_index - 2 < 6) {
                exit_error("Color in hexadecimal must contain 6 characters.");
        }

        rgb_t ret = {0};
        for (size_t i = info->equal_index + 2, j = 0; i < info->line->len && j < 6; ++i, ++j) {
                char c = info->line->items[i];
                if (!is_valid_color_char(c)) {
                        exit_error("Invalid character detected in hexadecimal color.");
                }
                switch (j) {
                        case 0: ret.r = get_hex_value(c) << 4; break;
                        case 1: ret.r += get_hex_value(c); break;
                        case 2: ret.g = get_hex_value(c) << 4; break;
                        case 3: ret.g += get_hex_value(c); break;
                        case 4: ret.b = get_hex_value(c) << 4; break;
                        case 5: ret.b += get_hex_value(c); break;
                }
        }
        return ret;
}

int parse_number(const parse_info_t *info) {

        // creating a new buffer here to that the strtol function doesn't overflow
        size_t len = info->line->len - info->equal_index - 1;
        char buf[len + 1];
        buf[len] = '\0';
        memcpy(buf, info->line->items + info->equal_index + 1, len);
        return strtol(buf, NULL, 10);
}

void parse_gradient_left(const parse_info_t *info, editor_state_t *state) {
        rgb_t color = parse_color(info);
        state->display_state.gradient_color.left = color;
}

void parse_gradient_right(const parse_info_t *info, editor_state_t *state) {
        rgb_t color = parse_color(info);
        state->display_state.gradient_color.right = color;
}

void parse_screensaver_frame_length(const parse_info_t *info, editor_state_t *state) {
        state->display_state.screensaver_frame_length_ms = parse_number(info);
}

void parse_screensaver_ms_inactive(const parse_info_t *info, editor_state_t *state) {
        state->display_state.screensaver_ms_inactive = parse_number(info);
}

void parse_gradient_cycle_duration_ms(const parse_info_t *info, editor_state_t *state) {
        state->display_state.gradient_cycle_duration_ms = parse_number(info);
}

void parse_rgb_cycle_duration_ms(const parse_info_t *info, editor_state_t *state) {
        state->display_state.rgb_cycle_duration_ms = parse_number(info);
}

void parse_tab_width(const parse_info_t *info, editor_state_t *state) {
        state->tab_width = parse_number(info);
}

// :)
void load_default_config(editor_state_t *state) {

        state->tab_width = 4;
        state->display_state.syntax_mode = HIGH_NONE;
        state->display_state.text_style_mode = STYLE_NORMAL;

        state->display_state.gradient_color.left = (rgb_t) DEFAULT_GRAD_LEFT;
        state->display_state.gradient_color.right = (rgb_t) DEFAULT_GRAD_RIGHT;
        state->display_state.angle = ANG_0;
        state->display_state.gradient_cycle_duration_ms = 0;

        state->display_state.screensaver_mode = SS_RPS;
        state->display_state.screensaver_ms_inactive = 5000;
        state->display_state.screensaver_frame_length_ms = 20;

        state->display_state.rgb_cycle_duration_ms = 20;
        state->display_state.rgb_state = 0;
}

void load_config(editor_state_t *state) {

        char *config_path = get_config_path();
        load_default_config(state);
        if (!config_path) {
                return;
        }
        buf_t *buf = buf_open(config_path, state->tab_width);  // tab_width here doesn't really matter
        free(config_path);

        if (buf == NULL) {
                exit_error("Invalid config file path. This is an error with the program itself.\n");
        }

        for (size_t l = 0; l < buf->lines.len; ++l) {

                dyn_str *line = buf->lines.items + l;
                if (line->len == 0 || line->items[0] == '#') {
                        goto next_line;
                }

                size_t key_len = -1;
                while (line->items[++key_len] != '=') {
                        if (key_len == line->len) {
                                goto next_line;
                        }
                }

                // key_len is now the index of the '='
                // literal spaghetti code lmao
                parse_info_t info = {line, key_len};
                if (!strncmp(line->items, HIGHLIGHT_MODE_SETTING, key_len)) {
                        parse_text_opts(HIGHLIGHT_MODE_SETTING, state->display_state.syntax_mode,
                                        HIGH_STR_OPTS, HIGH_ENUM_OPTS, info);
                }

                // case-wise - this is done to ignore settings for which there is no use
                if (state->display_state.syntax_mode == HIGH_GRADIENT) {
                        if (!strncmp(line->items, GRADIENT_LEFT_SETTING, key_len)) {
                                parse_gradient_left(&info, state);
                        } else if (!strncmp(line->items, GRADIENT_RIGHT_SETTING, key_len)) {
                                parse_gradient_right(&info, state);
                        } else if (!strncmp(line->items, GRADIENT_CYCLE_DURATION_MS, key_len)) {
                                parse_gradient_cycle_duration_ms(&info, state);
                        } else if (!strncmp(line->items, GRADIENT_ANGLE_SETTING, key_len)) {
                                parse_text_opts(GRADIENT_ANGLE_SETTING, state->display_state.angle,
                                                ANG_STR_OPTS, ANG_ENUM_OPTS, info);
                        }
                } else if (state->display_state.syntax_mode == HIGH_RGB) {
                        if (!strncmp(line->items, RGB_CYCLE_DURATION_MS, key_len)) {
                                parse_rgb_cycle_duration_ms(&info, state);
                        } else if (!strncmp(line->items, RGB_ANGLE_SETTING, key_len)) {
                                parse_text_opts(RGB_ANGLE_SETTING, state->display_state.angle,
                                                ANG_STR_OPTS, ANG_ENUM_OPTS, info);
                        }
                }

                if (!strncmp(line->items, TEXT_STYLE_SETTING, key_len)) {
                        parse_text_opts(TEXT_STYLE_SETTING, state->display_state.text_style_mode,
                                        STYLE_STR_OPTS, STYLE_ENUM_OPTS, info);
                } else if (!strncmp(line->items, SCREENSAVER_MODE_SETTING, key_len)) {
                        parse_text_opts(SCREENSAVER_MODE_SETTING, state->display_state.screensaver_mode,
                                        SS_STR_OPTS, SS_ENUM_OPTS, info);
                } else if (!strncmp(line->items, SCREENSAVER_FRAME_LENGTH_SETTING, key_len)) {
                        parse_screensaver_frame_length(&info, state);
                } else if (!strncmp(line->items, SCREENSAVER_MS_INACTIVE, key_len)) {
                        parse_screensaver_ms_inactive(&info, state);
                } else if (!strncmp(line->items, TAB_WIDTH_SETTING, key_len)) {
                        parse_tab_width(&info, state);
                }

        next_line:;
        }

        buf_free(buf);
}
