#include "syntax.h"
#include "../utils.h"

#include <ctype.h>

language_keywords_t *get_keywords_from_filename(const char *filename) {
        size_t n = strlen(filename);
        ssize_t i;
        for (i = n - 1; i >= 0 && filename[i] != '.' && filename[i] != '/'; --i);
        if (filename[i] == '/' || i == -1) {
                return NULL;
        }
        ++i;
        if (n - i == 1 && (filename[i] == 'c' || filename[i] == 'h')) {
                return &C_KEYWORDS;
        } else {
                return NULL;
        }
}

typedef enum {
        TT_NONE,
        TT_COMMENT,
        TT_STRING,
        TT_FUNCTION
} special_token_type_t;
list_typedef(special_token_str, special_token_type_t);
list_typedef(special_token_buf_map, special_token_str);

static special_token_buf_map map;
static bool current_file_valid;

void init_token_map(buf_t *buf) {
        map = list_init(special_token_buf_map, buf->lines.len);
        map.len = buf->lines.len;
        for (ssize_t i = 0; i < map.len; ++i) {
                map.items[i] = list_init(special_token_str, buf->lines.items[i].len);
                map.items[i].len = buf->lines.items[i].len;
                memset(map.items[i].items, TT_NONE, map.items[i].len);
        }
}

void free_token_map(void) {
        for (ssize_t i = 0; i < map.len; ++i) {
                free_list_items(1, map.items + i);
        }
        free_list_items(1, &map);
}

/* parses the buffer and determines where strings, comments, and long comments are
 * assumes the following:
 *   - short comments only affect the line they're placed on
 *   - long comments can affect multiple lines
 *   - strings cannot span multiple lines, and use ' or "
 *   - function calls involve a name token before a ( with optional whitespace
 *   - POSSIBLE TODO: add custom member highlighting by checking for preceding .
 */
void setup_syntax_highlighting(buf_t *buf) {

        init_token_map(buf);
        language_keywords_t *kw = get_keywords_from_filename(buf->filename);
        if (kw == NULL) {
                current_file_valid = false;
                return;
        }
        current_file_valid = true;
        
        char in_string = 0;
        bool in_short_comment = false, in_long_comment = false;
        dyn_str *line;
        char c;
        for (ssize_t y = 0; y < buf->lines.len; ++y) {
                line = buf->lines.items + y;
                for (ssize_t x = 0; x < line->len; ++x) {
                        c = line->items[x];
                        if (in_string) {
                                map.items[y].items[x] = TT_STRING;
                                if (in_string == c && line->items[x - 1] != '\\') {  // string ends
                                        in_string = 0;
                                }
                                continue;
                        } else if (in_long_comment) {
                                map.items[y].items[x] = TT_COMMENT;
                                size_t lce_len = strlen(kw->long_comment_end);
                                if (line->len - x >= lce_len && !strncmp(kw->long_comment_end, line->items + x, lce_len)) {  // long comment ends
                                        for (; lce_len-->1; ++x) {
                                                map.items[y].items[x + 1] = TT_COMMENT;
                                        }
                                        in_long_comment = false;
                                }
                                continue;
                        } else if (in_short_comment) {  // cannot end in the same line
                                map.items[y].items[x] = TT_COMMENT;
                                continue;
                        }
                        
                        // string starts
                        if (c == '"' || c == '\'') {
                                map.items[y].items[x] = TT_STRING;
                                in_string = c;
                                continue;
                        }

                        // short comment starts
                        size_t sc_len = strlen(kw->short_comment);
                        if (line->len - x >= sc_len && !strncmp(kw->short_comment, line->items + x, sc_len)) {  // long comment ends
                                map.items[y].items[x] = TT_COMMENT;
                                for (; sc_len-->1; ++x) {
                                        map.items[y].items[x + 1] = TT_COMMENT;
                                }
                                in_short_comment = true;
                                continue;
                        }

                        // long comment starts -- this is the same code as above consider abstraction
                        size_t lcs_len = strlen(kw->long_comment_start);
                        if (line->len - x >= lcs_len && !strncmp(kw->long_comment_start, line->items + x, lcs_len)) {  // long comment ends
                                map.items[y].items[x] = TT_COMMENT;
                                for (; lcs_len-->1; ++x) {
                                        map.items[y].items[x + 1] = TT_COMMENT;
                                }
                                in_long_comment = true;
                                continue;
                        }
        
                        // is function call here -- for now i'm not going to go all the way to making it look in previous lines
                        if (c == '(') {
                                ssize_t other_x = x - 1;
                                for (; other_x >= 0 && line->items[other_x] == ' '; --other_x);
                                if (other_x == -1) {
                                        continue;
                                }
                                for (; other_x >= 0 && is_name_char(line->items[other_x]); --other_x) {
                                        map.items[y].items[x] = TT_FUNCTION;
                                }
                        }
                }
                in_short_comment = false;
                in_string = 0;
        }
}

/*
 * Order of precedence:
 *  - Check for a comment or a string with TT_COMMENT and TT_STRING respectively.
 *  - Check for a keyword by checking against the list of keywords.
 *  - Check for a function call with TT_FUNCTION (so something like if (...) {...} isn't a function)
 *  - Check for a number (only containing digits)
 *  - POSSIBLE IN FUTURE: Check for a number in binary, octal, and hex form (0b, 0, and 0x respectively)
 *    - May not be done due to inconsistencies (with octals in Python being 0o and C just being 0)
 *  - Check for a constant value (all caps)
 *  - Check for a class name or something (starts with capital letter)
 *  - Treat as normal
 */
ansi_code_t get_syntax_highlighting(ssize_t line_index, dyn_str *line, token_t *token) {

        if (!current_file_valid) {
                return (ansi_code_t) { (rgb_t) {0, 0, 0}, .style = 0 };
        }

        special_token_type_t tt = map.items[line_index].items[token->start];
        if (tt == TT_COMMENT) {
                return (ansi_code_t) { (rgb_t) {125, 125, 125}, .style = 3 };
        } else if (tt == TT_STRING) {
                return (ansi_code_t) { (rgb_t) {123, 255, 123}, .style = 3 };
        }

        char c = line->items[token->start];
        if (is_name_char(c) && !isdigit(c)) {
                keyword_list_t *kw = kw->len + (c == '_' ? 26 : tolower(c) - 'a');
                for (int i = 0; i < kw->len; ++i) {
                        if (strncmp(kw->keywords[i].kw, line->items + token->start, token->end - token->start)) {
                                return // kw highlighting by kw;
                        }
                }
        }
}
