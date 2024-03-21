#include "state.h"
#include "buf.h"
#include "utils.h"

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#define GRADIENT_LEFT_SETTING "gradient_left"
#define GRADIENT_RIGHT_SETTING "gradient_right"
#define HIGHLIGHT_MODE_SETTING "highlight_mode"

// source: https://stackoverflow.com/a/78195956
char *get_config_path(void) {
        const char *home_path;
        const char *config_name = ".stupid_editor_rc";
#ifdef _WIN32
        home_path = getenv("UserProfile");
#else
        home_path = getenv("HOME");
#endif
        char *config_path = malloc(strlen(home_path) + strlen(config_name));
        sprintf(config_path, "%s/%s", home_path, config_name);
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
 
rgb_t parse_color(struct parse_info *info) {
        if (info->line->items[info->equal_index + 1] != '#') {
                exit_error("Color must begin with a hex code.");
        } else if (info->line->len - info->equal_index - 2 < 6) {
                exit_error("Color in hexadecimal must contain 6 characters.");
        }

        rgb_t ret = {0};
        for (size_t i = info->equal_index + 2, j = 0; i < info->line->len && j < 6; ++i, ++j) {
                char c = info->line->items[i];
                if (!is_valid_color_char(info->line->items[i])) {
                        exit_error("Invalid charatcer detected in line");
                }
                switch (j) {
                        case 0: ret.r += get_hex_value(info->line->items[i]) << 4;
                        case 1: ret.r += get_hex_value(info->line->items[i]);
                        case 2: ret.g += get_hex_value(info->line->items[i]) << 4;
                        case 3: ret.g += get_hex_value(info->line->items[i]);
                        case 4: ret.b += get_hex_value(info->line->items[i]) << 4;
                        case 5: ret.b += get_hex_value(info->line->items[i]);
                }
        }
        return ret;
}

void parse_gradient_left(struct parse_info *info, editor_state_t *state) {
        rgb_t color = parse_color(info);
        state->display_state.gradient_color.left = color;
}

void parse_gradient_right(struct parse_info *info, editor_state_t *state) {
        rgb_t color = parse_color(info);
        state->display_state.gradient_color.right = color;
}

void parse_highlight_mode(struct parse_info *info, editor_state_t *state) {

}

void load_default_config(editor_state_t *state) {
        return;
}

void parse_config_file(editor_state_t *state) {

        char *config_path = get_config_path();
        if (!config_path) {
                load_default_config(state);
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
                if (strncmp(line->items, GRADIENT_LEFT_SETTING, key_len)) {
                        parse_gradient_left(&info, state);
                } else if (strncmp(line->items, GRADIENT_RIGHT_SETTING, key_len)) {
                        parse_gradient_right(&info, state);
                } else if (strncmp(line->items, HIGHLIGHT_MODE_SETTING, key_len)) {
                        parse_highlight_mode(&info, state);
                }

        next_line:;
        }
        free(buf);
}
