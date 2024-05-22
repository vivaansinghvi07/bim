#ifndef EDITOR_SYNTAX
#define EDITOR_SYNTAX

#include <stdio.h>

#include "../buf.h"
#include "display.h"

typedef enum {
        KW_DECL = 0,
        KW_TYPE,
        KW_CTRL_FLOW,
        KW_MODIFIER,
        KW_CONST,
        KW_SPECIAL_FUNC
} keyword_type_t;
#define KEYWORD_TYPE_COUNT 6

typedef struct {
        const char *word;
        keyword_type_t type;
} keyword_t;

typedef struct {
        int len;
        keyword_t *keywords;
} keyword_list_t;

typedef struct {
        const char *short_comment;
        const char *long_comment_start;
        const char *long_comment_end;
        keyword_list_t keywords_by_letter[27];  // includes _, and capital letters share
} syntax_rules_t;

/*
 * Keywords for the C language, capturing file extensions ".c" and ".h"
 */

syntax_rules_t *get_keywords_from_filename(const char *filename);
void setup_syntax_highlighting(const buf_t *buf);
void fill_token_type_color_table(void);
void fill_special_token_type_color_table(void);
void fill_keyword_type_color_table(void);
ansi_code_t get_syntax_highlighting(const ssize_t line_index, const dyn_str *line, const token_t *token);

#endif
