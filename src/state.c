#include "state.h"
#include "buf.h"
#include "utils.h"

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#define CONFIG_FILE_NAME ".stupid_editor_rc"

#define GRADIENT_LEFT_SETTING "gradient_left"
#define GRADIENT_RIGHT_SETTING "gradient_right"
#define TEXT_STYLE_SETTING "text_style"
#define HIGHLIGHT_MODE_SETTING "highlight_mode"
#define SCREENSAVER_MODE_SETTING "screensaver_mode"

const char *HIGH_STR_OPTS[] = {"GRADIENT", "LEXICAL", "RANDOM", "NONE"};
const highlighting_mode HIGH_ENUM_OPTS[] = {HIGH_GRADIENT, HIGH_ALPHA, HIGH_RANDOM, HIGH_NONE};

const char *STYLE_STR_OPTS[] = {"BOLD", "NORMAL", "ITALIC"};
const text_style_mode STYLE_ENUM_OPTS[] = {STYLE_BOLD, STYLE_NORMAL, STYLE_ITALIC};

const char *SS_STR_OPTS[] = {"LEFT_SLIDE", "RIGHT_SLIDE", "TOP_SLIDE", "BOTTOM_SLIDE",
                            "ROCK_PAPER_SCISSORS", "GAME_OF_LIFE", "FALLING_SAND"};
const screensaver_mode SS_ENUM_OPTS[] = {SS_LEFT, SS_RIGHT, SS_TOP, SS_BOTTOM,
                                         SS_RPS, SS_LIFE, SS_SAND};

#define DEFAULT_GRAD_LEFT {255, 0, 0}
#define DEFAULT_GRAD_RIGHT {0, 0, 255}

#define parse_text_opts(val_to_set, str_opts_arr, enum_opts_arr, info)                              \
        do {                                                                                        \
                const char *ending_str = (info).line->items + (info).equal_index + 1;               \
                size_t len = (info).line->len - (info).equal_index - 1;                             \
                for (size_t i = (info).line->len - 1; (info).line->items[i] == ' '; --i, --len) {}  \
                                                                                                    \
                for (int i = 0; i < sizeof(str_opts_arr) / sizeof(*str_opts_arr); ++i) {            \
                        if (!strncmp(ending_str, str_opts_arr[i], len)) {                           \
                                val_to_set = enum_opts_arr[i];                                      \
                        }                                                                           \
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
struct parse_info {
        dyn_str *line; 
        size_t equal_index;
};

bool is_valid_color_char(const char c) {
        return isdigit(c) || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F';
}

// assumes c matches [A-Fa-z0-9]
uint8_t get_hex_value(const char c) {
        if (isdigit(c)) {
                return c - '0';
        } else if (islower(c)) {
                return c - 'a' + 10;
        } else {
                return c - 'A' + 10;
        }
}
 
rgb_t parse_color(const struct parse_info *info) {
        if (info->line->items[info->equal_index + 1] != '#') {
                exit_error("Color must begin with a hex code.");
        } else if (info->line->len - info->equal_index - 2 < 6) {
                exit_error("Color in hexadecimal must contain 6 characters.");
        }

        rgb_t ret = {0};
        for (size_t i = info->equal_index + 2, j = 0; i < info->line->len && j < 6; ++i, ++j) {
                char c = info->line->items[i];
                if (!is_valid_color_char(c)) {
                        exit_error("Invalid charatcer detected in line");
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


void parse_gradient_left(const struct parse_info *info, editor_state_t *state) {
        rgb_t color = parse_color(info);
        state->display_state.gradient_color.left = color;
}

void parse_gradient_right(const struct parse_info *info, editor_state_t *state) {
        rgb_t color = parse_color(info);
        state->display_state.gradient_color.right = color;
}

void load_default_config(editor_state_t *state) {
        state->display_state.syntax_mode = HIGH_NONE;
        state->display_state.text_style_mode = STYLE_NORMAL;
        state->display_state.gradient_color.left = (rgb_t) DEFAULT_GRAD_LEFT;
        state->display_state.gradient_color.right = (rgb_t) DEFAULT_GRAD_RIGHT;
        state->display_state.screensaver_mode = SS_RPS;
}

void parse_config_file(editor_state_t *state) {

        char *config_path = get_config_path();
        load_default_config(state);
        if (!config_path) {
                return;
        }
        file_buf *buf = buf_open(config_path);
        free(config_path);

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
                struct parse_info info = {line, key_len};
                if (!strncmp(line->items, GRADIENT_LEFT_SETTING, key_len)) {
                        parse_gradient_left(&info, state);
                } else if (!strncmp(line->items, GRADIENT_RIGHT_SETTING, key_len)) {
                        parse_gradient_right(&info, state);
                } else if (!strncmp(line->items, HIGHLIGHT_MODE_SETTING, key_len)) {
                        parse_text_opts(state->display_state.syntax_mode,
                                        HIGH_STR_OPTS, HIGH_ENUM_OPTS, info);
                } else if (!strncmp(line->items, TEXT_STYLE_SETTING, key_len)) {
                        parse_text_opts(state->display_state.text_style_mode,
                                        STYLE_STR_OPTS, STYLE_ENUM_OPTS, info);
                } else if (!strncmp(line->items, SCREENSAVER_MODE_SETTING, key_len)) {
                        parse_text_opts(state->display_state.screensaver_mode,
                                        SS_STR_OPTS, SS_ENUM_OPTS, info);
                }

        next_line:;
        }
        free(buf);
}
