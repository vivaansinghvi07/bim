#include "syntax.h"
#include "../utils.h"

#include <stdlib.h>
#include <ctype.h>

static keyword_t FT_C_A[] = {{"alignas", KW_MODIFIER}, {"alignof", KW_SPECIAL_FUNC}, {"auto", KW_TYPE}};
static keyword_t FT_C_B[] = {{"bool", KW_TYPE}, {"break", KW_CTRL_FLOW}};
static keyword_t FT_C_C[] = {{"case", KW_CTRL_FLOW}, {"char", KW_TYPE}, {"const", KW_MODIFIER}, {"constexpr", KW_MODIFIER}, {"continue", KW_CTRL_FLOW}};
static keyword_t FT_C_D[] = {{"default", KW_CTRL_FLOW}, {"do", KW_CTRL_FLOW}, {"double", KW_TYPE}};
static keyword_t FT_C_E[] = {{"else", KW_CTRL_FLOW}, {"enum", KW_DECL}, {"extern", KW_MODIFIER}};
static keyword_t FT_C_F[] = {{"false", KW_CONST}, {"float", KW_TYPE}, {"for", KW_CTRL_FLOW}};
static keyword_t FT_C_G[] = {{"goto", KW_CTRL_FLOW}};
static keyword_t FT_C_I[] = {{"if", KW_CTRL_FLOW}, {"inline", KW_MODIFIER}, {"int", KW_TYPE}};
static keyword_t FT_C_L[] = {{"long", KW_TYPE}};
static keyword_t FT_C_N[] = {{"nullptr", KW_CONST}, {"NULL", KW_CONST}};
static keyword_t FT_C_R[] = {{"register", KW_MODIFIER}, {"restrict", KW_MODIFIER}, {"return", KW_CTRL_FLOW}};
static keyword_t FT_C_S[] = {{"short", KW_TYPE}, {"signed", KW_TYPE}, {"sizeof", KW_SPECIAL_FUNC}, {"static", KW_MODIFIER},
                             {"static_assert", KW_SPECIAL_FUNC}, {"struct", KW_DECL}, {"switch", KW_CTRL_FLOW}};
static keyword_t FT_C_T[] = {{"thread_local", KW_MODIFIER}, {"true", KW_CONST}, {"typedef", KW_DECL}, 
                             {"typeof", KW_SPECIAL_FUNC}, {"typeof_unqual", KW_SPECIAL_FUNC}};
static keyword_t FT_C_U[] = {{"union", KW_DECL}, {"unsigned", KW_TYPE}};
static keyword_t FT_C_V[] = {{"void", KW_TYPE}, {"volatile", KW_MODIFIER}};
static keyword_t FT_C_W[] = {{"while", KW_CTRL_FLOW}};
static keyword_t FT_C__[] = {{"_Atomic", KW_MODIFIER}, {"_BitInt", KW_TYPE}, {"#if", KW_CTRL_FLOW}, {"#elif", KW_CTRL_FLOW},
                             {"#else", KW_CTRL_FLOW}, {"#endif", KW_CTRL_FLOW}, {"#ifdef", KW_CTRL_FLOW}, {"#ifndef", KW_CTRL_FLOW},
                             {"#elifdef", KW_CTRL_FLOW}, {"#elifndef", KW_CTRL_FLOW}, {"#define", KW_DECL}, {"#undef", KW_DECL},
                             {"#include", KW_DECL}, {"#embed", KW_DECL}, {"#line", KW_DECL}, {"#error", KW_DECL},
                             {"#warning", KW_DECL}, {"#pragma", KW_DECL}};

static syntax_rules_t C_RULES = {"//", "/*", "*/", {
        // i know alignof and sizeof are operators but im calling them functions
        {3, FT_C_A}, {1, FT_C_B}, {5, FT_C_C}, {3, FT_C_D}, {3, FT_C_E}, {3, FT_C_F}, {1, FT_C_G},
        {0, NULL},   {3, FT_C_I}, {0, NULL},   {0, NULL},   {1, FT_C_L}, {0, NULL},   {2, FT_C_N}, 
        {0, NULL},   {0, NULL},   {0, NULL},   {3, FT_C_R}, {7, FT_C_S}, {5, FT_C_T}, {2, FT_C_U},
        {2, FT_C_V}, {1, FT_C_W}, {0, NULL},   {0, NULL},   {0, NULL},   {18, FT_C__}
}};

// uhhh what am i doing
typedef enum {
        STT_NONE = 0,
        STT_COMMENT,
        STT_STRING,
        STT_FUNCTION
} special_token_type_t;
#define SPECIAL_TOKEN_TYPE_COUNT 4
list_typedef(special_token_str, special_token_type_t);
list_typedef(special_token_buf_map, special_token_str);

static special_token_buf_map map;
static syntax_rules_t *syntax_rules;

void store_syntax_rules_from_filename(const char *filename) {
        size_t n = strlen(filename);
        ssize_t i;
        for (i = n - 1; i >= 0 && filename[i] != '.' && filename[i] != '/'; --i);
        if (filename[i] == '/' || i == -1) {
                syntax_rules = NULL;
        }
        ++i;
        if (n - i == 1 && (filename[i] == 'c' || filename[i] == 'h')) {
                syntax_rules = &C_RULES;
        } else {
                syntax_rules = NULL;
        }
}

void init_token_map(const buf_t *buf) {
        map = list_init(special_token_buf_map, buf->lines.len);
        map.len = buf->lines.len;
        for (ssize_t i = 0; i < map.len; ++i) {
                map.items[i] = list_init(special_token_str, buf->lines.items[i].len);
                map.items[i].len = buf->lines.items[i].len;
                memset(map.items[i].items, STT_NONE, map.items[i].len);
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
void setup_syntax_highlighting(const buf_t *buf) {

        init_token_map(buf);
        store_syntax_rules_from_filename(buf->filename);
        if (syntax_rules == NULL) {
                return;
        }
        
        char in_string = 0;
        bool in_short_comment = false, in_long_comment = false;
        dyn_str *line;
        char c;
        for (ssize_t y = 0; y < buf->lines.len; ++y) {
                line = buf->lines.items + y;
                for (ssize_t x = 0; x < line->len; ++x) {
                        c = line->items[x];
                        if (in_string) {
                                map.items[y].items[x] = STT_STRING;
                                if (in_string == c && line->items[x - 1] != '\\') {  // string ends
                                        in_string = 0;
                                }
                                continue;
                        } else if (in_long_comment) {
                                map.items[y].items[x] = STT_COMMENT;
                                size_t lce_len = strlen(syntax_rules->long_comment_end);
                                if (line->len - x >= lce_len && !strncmp(syntax_rules->long_comment_end, line->items + x, lce_len)) {  // long comment ends
                                        for (; lce_len-->1; ++x) {
                                                map.items[y].items[x + 1] = STT_COMMENT;
                                        }
                                        in_long_comment = false;
                                }
                                continue;
                        } else if (in_short_comment) {  // cannot end in the same line
                                map.items[y].items[x] = STT_COMMENT;
                                continue;
                        }
                        
                        // string starts
                        if (c == '"' || c == '\'') {
                                map.items[y].items[x] = STT_STRING;
                                in_string = c;
                                continue;
                        }

                        // short comment starts
                        size_t sc_len = strlen(syntax_rules->short_comment);
                        if (line->len - x >= sc_len && !strncmp(syntax_rules->short_comment, line->items + x, sc_len)) {  // long comment ends
                                map.items[y].items[x] = STT_COMMENT;
                                for (; sc_len-->1; ++x) {
                                        map.items[y].items[x + 1] = STT_COMMENT;
                                }
                                in_short_comment = true;
                                continue;
                        }

                        // long comment starts -- this is the same code as above consider abstraction
                        size_t lcs_len = strlen(syntax_rules->long_comment_start);
                        if (line->len - x >= lcs_len && !strncmp(syntax_rules->long_comment_start, line->items + x, lcs_len)) {  // long comment ends
                                map.items[y].items[x] = STT_COMMENT;
                                for (; lcs_len-->1; ++x) {
                                        map.items[y].items[x + 1] = STT_COMMENT;
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
                                        map.items[y].items[other_x] = STT_FUNCTION;
                                }
                        }
                }
                in_short_comment = false;
                in_string = 0;
        }
}

static ansi_code_t SPECIAL_TOKEN_TYPE_COLOR_TABLE[SPECIAL_TOKEN_TYPE_COUNT];
void fill_special_token_type_color_table(void) {
        for (uint8_t i = 0; i < SPECIAL_TOKEN_TYPE_COUNT; ++i) {
                SPECIAL_TOKEN_TYPE_COLOR_TABLE[i].rgb.r = arc4random_uniform(156) + 100;
                SPECIAL_TOKEN_TYPE_COLOR_TABLE[i].rgb.g = arc4random_uniform(156) + 100;
                SPECIAL_TOKEN_TYPE_COLOR_TABLE[i].rgb.b = arc4random_uniform(156) + 100;
                SPECIAL_TOKEN_TYPE_COLOR_TABLE[i].style = arc4random_uniform(ANSI_STYLE_VARIATION);
        }
}

static ansi_code_t KEYWORD_TYPE_COLOR_TABLE[KEYWORD_TYPE_COUNT];
void fill_keyword_type_color_table(void) {
        for (uint8_t i = 0; i < KEYWORD_TYPE_COUNT; ++i) {
                KEYWORD_TYPE_COLOR_TABLE[i].rgb.r = arc4random_uniform(156) + 100;
                KEYWORD_TYPE_COLOR_TABLE[i].rgb.g = arc4random_uniform(156) + 100;
                KEYWORD_TYPE_COLOR_TABLE[i].rgb.b = arc4random_uniform(156) + 100;
                KEYWORD_TYPE_COLOR_TABLE[i].style = arc4random_uniform(ANSI_STYLE_VARIATION);
        }
}

typedef enum {
        TT_OPERATOR = 0,
        TT_NAME,
        TT_NUMBER,
        TT_START_CAPS,
        TT_ALL_CAPS,
} token_type_t;
#define TOKEN_TYPE_COUNT 5

static ansi_code_t TOKEN_TYPE_COLOR_TABLE[TOKEN_TYPE_COUNT];
void fill_token_type_color_table(void) {
        for (uint8_t i = 0; i < TOKEN_TYPE_COUNT; ++i) {
                TOKEN_TYPE_COLOR_TABLE[i].rgb.r = arc4random_uniform(156) + 100;
                TOKEN_TYPE_COLOR_TABLE[i].rgb.g = arc4random_uniform(156) + 100;
                TOKEN_TYPE_COLOR_TABLE[i].rgb.b = arc4random_uniform(156) + 100;
                TOKEN_TYPE_COLOR_TABLE[i].style = arc4random_uniform(ANSI_STYLE_VARIATION);
        }
}

ansi_code_t *get_highlighting_by_keyword_type(keyword_type_t type) {
        return (ansi_code_t *) KEYWORD_TYPE_COLOR_TABLE + type; 
}

ansi_code_t *get_highlighting_by_special_token_type(special_token_type_t type) {
        return (ansi_code_t *) SPECIAL_TOKEN_TYPE_COLOR_TABLE + type;
}

ansi_code_t *get_highlighting_by_token_type(token_type_t type) {
        return (ansi_code_t *) TOKEN_TYPE_COLOR_TABLE + type;
}

/*
 * Order of precedence:
 *  - Check for a comment or a string with TT_COMMENT and TT_STRING respectively.
 *  - Check for a keyword by checking against the list of keywords.
 *  - Check for a function call with TT_FUNCTION (so something like if (...) {...} isn't a function)
 *  - Check for a lone operator (not an alph character, one-wide token)
 *  - Check for a number (only containing digits)
 *  - POSSIBLE IN FUTURE: Check for a number in binary, octal, and hex form (0b, 0, and 0x respectively)
 *    - May not be done due to inconsistencies (with octals in Python being 0o and C just being 0)
 *  - Check for a constant value (all caps)
 *  - Check for a class name or something (starts with capital letter)
 *  - Treat as normal
 */
ansi_code_t get_syntax_highlighting(const ssize_t line_index, const dyn_str *line, const token_t *token) {

        if (syntax_rules == NULL) {
                return (ansi_code_t) { (rgb_t) {255, 255, 255}, .style = 0 };
        }

        special_token_type_t tt = map.items[line_index].items[token->start];
        if (tt == STT_COMMENT || tt == STT_STRING) {
                return *get_highlighting_by_special_token_type(tt);
        }

        char c = line->items[token->start];
        keyword_list_t *kw = syntax_rules->keywords_by_letter + (isalpha(c) ? tolower(c) - 'a' : 26);
        for (int i = 0; i < kw->len; ++i) {
                if (!strncmp(kw->keywords[i].word, line->items + token->start, token->end - token->start)
                    && token->end - token->start == strlen(kw->keywords[i].word)) {
                        return *get_highlighting_by_keyword_type(kw->keywords[i].type);
                }
        }
        
        if (tt == STT_FUNCTION) {
                return *get_highlighting_by_special_token_type(tt);
        }

        if (!is_name_char(c) && token->start + 1 == token->end) {  // todo - prob modify to make this include $ and #
                return *get_highlighting_by_token_type(TT_OPERATOR);
        }

        if (isdigit(c)) {
                return *get_highlighting_by_token_type(TT_NUMBER);
        }

        bool can_be_all_upper = true;
        for (ssize_t i = token->start; can_be_all_upper && i < token->end; ++i) {
                can_be_all_upper = isupper(line->items[i]) || !islower(line->items[i]);
        }
        if (can_be_all_upper) {
                return *get_highlighting_by_token_type(TT_ALL_CAPS);
        }

        if (isupper(c)) {
                return *get_highlighting_by_token_type(TT_START_CAPS);
        }
        
        return *get_highlighting_by_token_type(TT_NAME);
}
