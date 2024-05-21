#ifndef EDITOR_SYNTAX
#define EDITOR_SYNTAX

#include <stdio.h>

#include "../buf.h"
#include "display.h"

typedef enum {
        KW_DECL,
        KW_TYPE,
        KW_CTRL_FLOW,
        KW_MODIFIER,
        KW_CONST,
        KW_SPECIAL_FUNC
} keyword_type_t;

typedef struct {
        const char *kw;
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
        keyword_list_t kw_by_letter[27];  // includes _, and capital letters share
} language_keywords_t;

/*
 * Keywords for the C language, capturing file extensions ".c" and ".h"
 */

keyword_t FT_C_A[] = {{"alignas", KW_MODIFIER}, {"alignof", KW_SPECIAL_FUNC}, {"auto", KW_TYPE}};
keyword_t FT_C_B[] = {{"bool", KW_TYPE}, {"break", KW_CTRL_FLOW}};
keyword_t FT_C_C[] = {{"case", KW_CTRL_FLOW}, {"char", KW_TYPE}, {"const", KW_MODIFIER}, {"constexpr", KW_MODIFIER}, {"continue", KW_CTRL_FLOW}};
keyword_t FT_C_D[] = {{"default", KW_CTRL_FLOW}, {"do", KW_CTRL_FLOW}, {"double", KW_TYPE}};
keyword_t FT_C_E[] = {{"else", KW_CTRL_FLOW}, {"enum", KW_DECL}, {"extern", KW_MODIFIER}};
keyword_t FT_C_F[] = {{"false", KW_CONST}, {"float", KW_TYPE}, {"for", KW_CTRL_FLOW}};
keyword_t FT_C_G[] = {{"goto", KW_CTRL_FLOW}};
keyword_t FT_C_I[] = {{"if", KW_CTRL_FLOW}, {"inline", KW_MODIFIER}, {"int", KW_TYPE}};
keyword_t FT_C_L[] = {{"long", KW_TYPE}};
keyword_t FT_C_N[] = {{"nullptr", KW_CONST}, {"NULL", KW_CONST}};
keyword_t FT_C_R[] = {{"register", KW_MODIFIER}, {"restrict", KW_MODIFIER}, {"return", KW_CTRL_FLOW}};
keyword_t FT_C_S[] = {{"short", KW_TYPE}, {"signed", KW_TYPE}, {"sizeof", KW_SPECIAL_FUNC}, {"static", KW_MODIFIER},
                      {"static_assert", KW_SPECIAL_FUNC}, {"struct", KW_DECL}, {"switch", KW_CTRL_FLOW}};
keyword_t FT_C_T[] = {{"thread_local", KW_MODIFIER}, {"true", KW_CONST}, {"typedef", KW_DECL}, 
                      {"typeof", KW_SPECIAL_FUNC}, {"typeof_unqual", KW_SPECIAL_FUNC}};
keyword_t FT_C_U[] = {{"union", KW_DECL}, {"unsigned", KW_TYPE}};
keyword_t FT_C_V[] = {{"void", KW_TYPE}, {"volatile", KW_MODIFIER}};
keyword_t FT_C_W[] = {{"while", KW_CTRL_FLOW}};
keyword_t FT_C__[] = {{"_Atomic", KW_MODIFIER}, {"_BitInt", KW_TYPE}};

language_keywords_t C_KEYWORDS = {"//", "/*", "*/", {
        // i know alignof and sizeof are operators but im calling them functions
        {3, FT_C_A}, {1, FT_C_B}, {5, FT_C_C}, {3, FT_C_D}, {3, FT_C_E}, {3, FT_C_F}, {1, FT_C_G},
        {0, NULL},   {3, FT_C_I}, {0, NULL},   {0, NULL},   {1, FT_C_L}, {0, NULL},   {2, FT_C_N}, 
        {0, NULL},   {0, NULL},   {0, NULL},   {3, FT_C_R}, {7, FT_C_S}, {5, FT_C_T}, {2, FT_C_S},
        {2, FT_C_V}, {1, FT_C_W}, {0, NULL},   {0, NULL},   {0, NULL},   {2, FT_C__}
}};

language_keywords_t *get_keywords_from_filename(const char *filename);
void setup_syntax_highlighting(buf_t *buf);
ansi_code_t get_syntax_highlighting(ssize_t line_index, dyn_str *line, token_t *token);

#endif
