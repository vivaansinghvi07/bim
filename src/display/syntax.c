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

static syntax_rules_t C_RULES = {"'\"", "//", "/*", "*/", {
        // i know alignof and sizeof are operators but im calling them functions
        {3, FT_C_A}, {1, FT_C_B}, {5, FT_C_C}, {3, FT_C_D}, {3, FT_C_E}, {3, FT_C_F}, {1, FT_C_G},
        {0, NULL},   {3, FT_C_I}, {0, NULL},   {0, NULL},   {1, FT_C_L}, {0, NULL},   {2, FT_C_N}, 
        {0, NULL},   {0, NULL},   {0, NULL},   {3, FT_C_R}, {7, FT_C_S}, {5, FT_C_T}, {2, FT_C_U},
        {2, FT_C_V}, {1, FT_C_W}, {0, NULL},   {0, NULL},   {0, NULL},   {18, FT_C__}
}};

static syntax_rules_t MARKDOWN_RULES = {"`", NULL, "```", "```", {
        {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, 
        {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, 
        {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, 
        {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL},
}};

static keyword_t FT_PY_A[] = {{"and", KW_SPECIAL_FUNC}, {"as", KW_DECL}, {"assert", KW_DECL},
                              {"async", KW_MODIFIER}, {"await", KW_DECL}};
static keyword_t FT_PY_B[] = {{"break", KW_CTRL_FLOW}};
static keyword_t FT_PY_C[] = {{"class", KW_DECL}, {"continue", KW_CTRL_FLOW}, {"case", KW_CTRL_FLOW}};
static keyword_t FT_PY_D[] = {{"def", KW_DECL}, {"del", KW_DECL}};
static keyword_t FT_PY_E[] = {{"elif", KW_CTRL_FLOW}, {"else", KW_CTRL_FLOW}, {"except", KW_CTRL_FLOW}};
static keyword_t FT_PY_F[] = {{"False", KW_CONST}, {"finally", KW_CTRL_FLOW}, {"for", KW_CTRL_FLOW}, {"from", KW_DECL}};
static keyword_t FT_PY_G[] = {{"global", KW_DECL}};
static keyword_t FT_PY_I[] = {{"if", KW_CTRL_FLOW}, {"import", KW_DECL}, {"in", KW_SPECIAL_FUNC}, {"is", KW_SPECIAL_FUNC}};
static keyword_t FT_PY_L[] = {{"lambda", KW_DECL}};
static keyword_t FT_PY_M[] = {{"match", KW_CTRL_FLOW}};
static keyword_t FT_PY_N[] = {{"None", KW_CONST}, {"nonlocal", KW_DECL}, {"not", KW_SPECIAL_FUNC}};
static keyword_t FT_PY_O[] = {{"or", KW_SPECIAL_FUNC}};
static keyword_t FT_PY_P[] = {{"pass", KW_CTRL_FLOW}};
static keyword_t FT_PY_R[] = {{"raise", KW_CTRL_FLOW}, {"return", KW_CTRL_FLOW}};
static keyword_t FT_PY_T[] = {{"True", KW_CONST}, {"try", KW_CTRL_FLOW}, {"type", KW_DECL}};
static keyword_t FT_PY_W[] = {{"while", KW_CTRL_FLOW}, {"with", KW_DECL}};
static keyword_t FT_PY_Y[] = {{"yield", KW_CTRL_FLOW}};

static syntax_rules_t PYTHON_RULES = {"'\"", "#", "\"\"\"", "\"\"\"", {
        {5, FT_PY_A}, {1, FT_PY_B}, {3, FT_PY_C}, {2, FT_PY_D}, {3, FT_PY_E}, {4, FT_PY_F},
        {1, FT_PY_G}, {0, NULL},    {4, FT_PY_I}, {0, NULL},    {0, NULL},    {1, FT_PY_L},
        {1, FT_PY_M}, {3, FT_PY_N}, {1, FT_PY_O}, {1, FT_PY_P}, {0, NULL},    {2, FT_PY_R}, 
        {0, NULL},    {3, FT_PY_T}, {0, NULL},    {0, NULL},    {2, FT_PY_W}, {0, NULL},
        {1, FT_PY_Y}, {0, NULL},    {0, NULL}
}};

// using javascript rules, reference here: https://www.w3schools.com/js/js_reserved.asp
static keyword_t FT_TS_A[] = {{"abstract", KW_DECL}, {"arguments", KW_CONST}, {"await", KW_CTRL_FLOW}};
static keyword_t FT_TS_B[] = {{"boolean", KW_TYPE}, {"break", KW_CTRL_FLOW}, {"byte", KW_TYPE}};
static keyword_t FT_TS_C[] = {{"case", KW_CTRL_FLOW}, {"catch", KW_CTRL_FLOW}, {"char", KW_TYPE}, {"class", KW_DECL}, {"const", KW_DECL}, {"continue", KW_CTRL_FLOW}};
static keyword_t FT_TS_D[] = {{"debugger", KW_CTRL_FLOW}, {"default", KW_CTRL_FLOW}, {"delete", KW_DECL}, {"do", KW_CTRL_FLOW}, {"double", KW_TYPE}};
static keyword_t FT_TS_E[] = {{"else", KW_CTRL_FLOW}, {"enum", KW_DECL}, {"eval", KW_SPECIAL_FUNC}, {"export", KW_DECL}, {"extends", KW_DECL}};
static keyword_t FT_TS_F[] = {{"false", KW_CONST}, {"final", KW_DECL}, {"finally", KW_CTRL_FLOW}, {"float", KW_TYPE}, {"for", KW_CTRL_FLOW}, {"function", KW_DECL}};
static keyword_t FT_TS_G[] = {{"goto", KW_CTRL_FLOW}};
static keyword_t FT_TS_I[] = {{"if", KW_CTRL_FLOW}, {"implements", KW_DECL}, {"import", KW_DECL}, {"in", KW_SPECIAL_FUNC}, {"instanceof", KW_SPECIAL_FUNC}, {"int", KW_TYPE}, {"interface", KW_DECL}};
static keyword_t FT_TS_L[] = {{"let", KW_DECL}, {"long", KW_TYPE}};
static keyword_t FT_TS_N[] = {{"native", KW_MODIFIER}, {"new", KW_SPECIAL_FUNC}, {"null", KW_CONST}};
static keyword_t FT_TS_P[] = {{"package", KW_DECL}, {"private", KW_MODIFIER}, {"protected", KW_MODIFIER}, {"public", KW_MODIFIER}};
static keyword_t FT_TS_R[] = {{"return", KW_CTRL_FLOW}};
static keyword_t FT_TS_S[] = {{"short", KW_TYPE}, {"static", KW_MODIFIER}, {"super", KW_SPECIAL_FUNC}, {"switch", KW_CTRL_FLOW}, {"synchronized", KW_MODIFIER}};
static keyword_t FT_TS_T[] = {{"this", KW_CONST}, {"throw", KW_CTRL_FLOW}, {"throws", KW_DECL}, {"transient", KW_MODIFIER}, {"true", KW_CONST}, {"try", KW_CTRL_FLOW}, {"typeof", KW_SPECIAL_FUNC}};
static keyword_t FT_TS_V[] = {{"var", KW_DECL}, {"void", KW_TYPE}, {"volatile", KW_MODIFIER}};
static keyword_t FT_TS_W[] = {{"while", KW_CTRL_FLOW}, {"with", KW_DECL}};
static keyword_t FT_TS_Y[] = {{"yield", KW_CTRL_FLOW}};

static syntax_rules_t TYPESCRIPT_RULES = {"'\"`", "//", "/*", "*/", {
        {3, FT_TS_A}, {3, FT_TS_B}, {6, FT_TS_C}, {5, FT_TS_D}, {5, FT_TS_E}, {6, FT_TS_F}, {1, FT_TS_G},
        {0, NULL}, {7, FT_TS_I}, {0, NULL}, {0, NULL}, {2, FT_TS_L}, {0, NULL}, {3, FT_TS_N},
        {0, NULL}, {4, FT_TS_P}, {0, NULL}, {1, FT_TS_R}, {5, FT_TS_S}, {7, FT_TS_T}, {0, NULL},
        {3, FT_TS_V}, {2, FT_TS_W}, {0, NULL}, {1, FT_TS_Y}, {0, NULL}, {0, NULL},
}};

static keyword_t FT_HTML_A[] = {{"a", KW_DECL}, {"abbr", KW_DECL}, {"acronym", KW_DECL}, {"address", KW_DECL}, {"applet", KW_DECL}, {"area", KW_DECL}, {"article", KW_DECL}, {"aside", KW_DECL}, {"audio", KW_DECL}, {"accept-charset", KW_MODIFIER}, {"accesskey", KW_MODIFIER}, {"action", KW_MODIFIER}, {"align", KW_MODIFIER}, {"alt", KW_MODIFIER}, {"async", KW_MODIFIER}, {"autocomplete", KW_MODIFIER}, {"autofocus", KW_MODIFIER}, {"autoplay", KW_MODIFIER}};
static keyword_t FT_HTML_B[] = {{"b", KW_DECL}, {"base", KW_DECL}, {"basefont", KW_DECL}, {"bdi", KW_DECL}, {"bdo", KW_DECL}, {"big", KW_DECL}, {"blockquote", KW_DECL}, {"body", KW_DECL}, {"br", KW_DECL}, {"button", KW_DECL}, {"bgcolor", KW_MODIFIER}, {"border", KW_MODIFIER}};
static keyword_t FT_HTML_C[] = {{"canvas", KW_DECL}, {"caption", KW_DECL}, {"center", KW_DECL}, {"cite", KW_DECL}, {"code", KW_DECL}, {"col", KW_DECL}, {"colgroup", KW_DECL}, {"charset", KW_MODIFIER}, {"checked", KW_MODIFIER}, {"cite", KW_MODIFIER}, {"class", KW_MODIFIER}, {"color", KW_MODIFIER}, {"cols", KW_MODIFIER}, {"colspan", KW_MODIFIER}, {"content", KW_MODIFIER}, {"contenteditable", KW_MODIFIER}, {"controls", KW_MODIFIER}, {"coords", KW_MODIFIER}};
static keyword_t FT_HTML_D[] = {{"DOCTYPE", KW_DECL}, {"data", KW_DECL}, {"datalist", KW_DECL}, {"dd", KW_DECL}, {"del", KW_DECL}, {"details", KW_DECL}, {"dfn", KW_DECL}, {"dialog", KW_DECL}, {"dir", KW_DECL}, {"div", KW_DECL}, {"dl", KW_DECL}, {"dt", KW_DECL}, {"data", KW_MODIFIER}, {"data-*", KW_MODIFIER}, {"datetime", KW_MODIFIER}, {"default", KW_MODIFIER}, {"defer", KW_MODIFIER}, {"dir", KW_MODIFIER}, {"dirname", KW_MODIFIER}, {"disabled", KW_MODIFIER}, {"download", KW_MODIFIER}, {"draggable", KW_MODIFIER}};
static keyword_t FT_HTML_E[] = {{"em", KW_DECL}, {"embed", KW_DECL}, {"enctype", KW_MODIFIER}, {"enterkeyhint", KW_MODIFIER}};
static keyword_t FT_HTML_F[] = {{"fieldset", KW_DECL}, {"figcaption", KW_DECL}, {"figure", KW_DECL}, {"font", KW_DECL}, {"footer", KW_DECL}, {"form", KW_DECL}, {"frame", KW_DECL}, {"frameset", KW_DECL}, {"for", KW_MODIFIER}, {"form", KW_MODIFIER}, {"formaction", KW_MODIFIER}};
static keyword_t FT_HTML_H[] = {{"h1", KW_DECL}, {"h2", KW_DECL}, {"h3", KW_DECL}, {"h4", KW_DECL}, {"h5", KW_DECL}, {"h6", KW_DECL}, {"head", KW_DECL}, {"header", KW_DECL}, {"hgroup", KW_DECL}, {"hr", KW_DECL}, {"html", KW_DECL}, {"headers", KW_MODIFIER}, {"height", KW_MODIFIER}, {"hidden", KW_MODIFIER}, {"high", KW_MODIFIER}, {"href", KW_MODIFIER}, {"hreflang", KW_MODIFIER}, {"http-equiv", KW_MODIFIER}};
static keyword_t FT_HTML_I[] = {{"i", KW_DECL}, {"iframe", KW_DECL}, {"img", KW_DECL}, {"input", KW_DECL}, {"ins", KW_DECL}, {"id", KW_MODIFIER}, {"inert", KW_MODIFIER}, {"inputmode", KW_MODIFIER}, {"ismap", KW_MODIFIER}};
static keyword_t FT_HTML_K[] = {{"kbd", KW_DECL}, {"kind", KW_MODIFIER}};
static keyword_t FT_HTML_L[] = {{"label", KW_DECL}, {"legend", KW_DECL}, {"li", KW_DECL}, {"link", KW_DECL}, {"label", KW_MODIFIER}, {"lang", KW_MODIFIER}, {"list", KW_MODIFIER}, {"loop", KW_MODIFIER}, {"low", KW_MODIFIER}};
static keyword_t FT_HTML_M[] = {{"main", KW_DECL}, {"map", KW_DECL}, {"mark", KW_DECL}, {"menu", KW_DECL}, {"meta", KW_DECL}, {"meter", KW_DECL}, {"max", KW_MODIFIER}, {"maxlength", KW_MODIFIER}, {"media", KW_MODIFIER}, {"method", KW_MODIFIER}, {"min", KW_MODIFIER}, {"multiple", KW_MODIFIER}, {"muted", KW_MODIFIER}};
static keyword_t FT_HTML_N[] = {{"nav", KW_DECL}, {"noframes", KW_DECL}, {"noscript", KW_DECL}, {"name", KW_MODIFIER}, {"novalidate", KW_MODIFIER}};
static keyword_t FT_HTML_O[] = {{"object", KW_DECL}, {"ol", KW_DECL}, {"optgroup", KW_DECL}, {"option", KW_DECL}, {"output", KW_DECL}, {"onabort", KW_MODIFIER}, {"onafterprint", KW_MODIFIER}, {"onbeforeprint", KW_MODIFIER}, {"onbeforeunload", KW_MODIFIER}, {"onblur", KW_MODIFIER}, {"oncanplay", KW_MODIFIER}, {"oncanplaythrough", KW_MODIFIER}, {"onchange", KW_MODIFIER}, {"onclick", KW_MODIFIER}, {"oncontextmenu", KW_MODIFIER}, {"oncopy", KW_MODIFIER}, {"oncuechange", KW_MODIFIER}, {"oncut", KW_MODIFIER}, {"ondblclick", KW_MODIFIER}, {"ondrag", KW_MODIFIER}, {"ondragend", KW_MODIFIER}, {"ondragenter", KW_MODIFIER}, {"ondragleave", KW_MODIFIER}, {"ondragover", KW_MODIFIER}, {"ondragstart", KW_MODIFIER}, {"ondrop", KW_MODIFIER}, {"ondurationchange", KW_MODIFIER}, {"onemptied", KW_MODIFIER}, {"onended", KW_MODIFIER}, {"onerror", KW_MODIFIER}, {"onfocus", KW_MODIFIER}, {"onhashchange", KW_MODIFIER}, {"oninput", KW_MODIFIER}, {"oninvalid", KW_MODIFIER}, {"onkeydown", KW_MODIFIER}, {"onkeypress", KW_MODIFIER}, {"onkeyup", KW_MODIFIER}, {"onload", KW_MODIFIER}, {"onloadeddata", KW_MODIFIER}, {"onloadedmetadata", KW_MODIFIER}, {"onloadstart", KW_MODIFIER}, {"onmousedown", KW_MODIFIER}, {"onmousemove", KW_MODIFIER}, {"onmouseout", KW_MODIFIER}, {"onmouseover", KW_MODIFIER}, {"onmouseup", KW_MODIFIER}, {"onmousewheel", KW_MODIFIER}, {"onoffline", KW_MODIFIER}, {"ononline", KW_MODIFIER}, {"onpagehide", KW_MODIFIER}, {"onpageshow", KW_MODIFIER}, {"onpaste", KW_MODIFIER}, {"onpause", KW_MODIFIER}, {"onplay", KW_MODIFIER}, {"onplaying", KW_MODIFIER}, {"onpopstate", KW_MODIFIER}, {"onprogress", KW_MODIFIER}, {"onratechange", KW_MODIFIER}, {"onreset", KW_MODIFIER}, {"onresize", KW_MODIFIER}, {"onscroll", KW_MODIFIER}, {"onsearch", KW_MODIFIER}, {"onseeked", KW_MODIFIER}, {"onseeking", KW_MODIFIER}, {"onselect", KW_MODIFIER}, {"onstalled", KW_MODIFIER}, {"onstorage", KW_MODIFIER}, {"onsubmit", KW_MODIFIER}, {"onsuspend", KW_MODIFIER}, {"ontimeupdate", KW_MODIFIER}, {"ontoggle", KW_MODIFIER}, {"onunload", KW_MODIFIER}, {"onvolumechange", KW_MODIFIER}, {"onwaiting", KW_MODIFIER}, {"onwheel", KW_MODIFIER}, {"open", KW_MODIFIER}, {"optimum", KW_MODIFIER}};
static keyword_t FT_HTML_P[] = {{"p", KW_DECL}, {"param", KW_DECL}, {"picture", KW_DECL}, {"pre", KW_DECL}, {"progress", KW_DECL}, {"pattern", KW_MODIFIER}, {"placeholder", KW_MODIFIER}, {"popover", KW_MODIFIER}, {"popovertarget", KW_MODIFIER}, {"popovertargetaction", KW_MODIFIER}, {"poster", KW_MODIFIER}, {"preload", KW_MODIFIER}};
static keyword_t FT_HTML_Q[] = {{"q", KW_DECL}};
static keyword_t FT_HTML_R[] = {{"rp", KW_DECL}, {"rt", KW_DECL}, {"ruby", KW_DECL}, {"readonly", KW_MODIFIER}, {"rel", KW_MODIFIER}, {"required", KW_MODIFIER}, {"reversed", KW_MODIFIER}, {"rows", KW_MODIFIER}, {"rowspan", KW_MODIFIER}};
static keyword_t FT_HTML_S[] = {{"s", KW_DECL}, {"samp", KW_DECL}, {"script", KW_DECL}, {"search", KW_DECL}, {"section", KW_DECL}, {"select", KW_DECL}, {"small", KW_DECL}, {"source", KW_DECL}, {"span", KW_DECL}, {"strike", KW_DECL}, {"strong", KW_DECL}, {"style", KW_DECL}, {"sub", KW_DECL}, {"summary", KW_DECL}, {"sup", KW_DECL}, {"svg", KW_DECL}, {"sandbox", KW_MODIFIER}, {"scope", KW_MODIFIER}, {"selected", KW_MODIFIER}, {"shape", KW_MODIFIER}, {"size", KW_MODIFIER}, {"sizes", KW_MODIFIER}, {"span", KW_MODIFIER}, {"spellcheck", KW_MODIFIER}, {"src", KW_MODIFIER}, {"srcdoc", KW_MODIFIER}, {"srclang", KW_MODIFIER}, {"srcset", KW_MODIFIER}, {"start", KW_MODIFIER}, {"step", KW_MODIFIER}, {"style", KW_MODIFIER}};
static keyword_t FT_HTML_T[] = {{"table", KW_DECL}, {"tbody", KW_DECL}, {"td", KW_DECL}, {"template", KW_DECL}, {"textarea", KW_DECL}, {"tfoot", KW_DECL}, {"th", KW_DECL}, {"thead", KW_DECL}, {"time", KW_DECL}, {"title", KW_DECL}, {"tr", KW_DECL}, {"track", KW_DECL}, {"tt", KW_DECL}, {"tabindex", KW_MODIFIER}, {"target", KW_MODIFIER}, {"title", KW_MODIFIER}, {"translate", KW_MODIFIER}, {"type", KW_MODIFIER}};
static keyword_t FT_HTML_U[] = {{"u", KW_DECL}, {"ul", KW_DECL}, {"usemap", KW_MODIFIER}};
static keyword_t FT_HTML_V[] = {{"var", KW_DECL}, {"video", KW_DECL}, {"value", KW_MODIFIER}};
static keyword_t FT_HTML_W[] = {{"wbraccept", KW_DECL}, {"width", KW_MODIFIER}, {"wrap", KW_MODIFIER}};

static syntax_rules_t HTML_RULES = {"'\"", NULL, "<!--", "-->", {
        {18, FT_HTML_A}, {12, FT_HTML_B}, {18, FT_HTML_C}, {22, FT_HTML_D}, {4, FT_HTML_E}, {11, FT_HTML_F}, {0, NULL},
        {18, FT_HTML_H}, {9, FT_HTML_I}, {0, NULL}, {2, FT_HTML_K}, {9, FT_HTML_L}, {13, FT_HTML_M}, {5, FT_HTML_N},
        {77, FT_HTML_O}, {12, FT_HTML_P}, {1, FT_HTML_Q}, {9, FT_HTML_R}, {31, FT_HTML_S}, {18, FT_HTML_T}, {3, FT_HTML_U},
        {3, FT_HTML_V}, {3, FT_HTML_W}, {0, NULL}, {0, NULL}, {0, NULL}, {0, NULL},
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
        } else if (n - i == 2 && !strncmp(filename + i, "md", 2)) {
                syntax_rules = &MARKDOWN_RULES;
        } else if (n - i == 2 && !strncmp(filename + i, "py", 2)
                   || n - i == 3 && !strncmp(filename + i, "pyi", 3)) {
                syntax_rules = &PYTHON_RULES;
        } else if (n - i == 2 && (!strncmp(filename + i, "ts", 2) || !strncmp(filename + i, "js", 2))) {
                syntax_rules = &TYPESCRIPT_RULES;
        } else if (n - i == 3 && !strncmp(filename + i, "htm", 3)
                   || n - i == 4 && !strncmp(filename + i, "html", 4)) {
                syntax_rules = &HTML_RULES;
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
                for (ssize_t j = 0; j < map.items[i].len; ++j) {
                        map.items[i].items[j] = STT_NONE;        
                }
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

        if (syntax_rules != NULL) {
                free_token_map();
        }
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
                                if (c == in_string) {
                                        ssize_t i = 1;
                                        for (; x >= i && line->items[x - i] == '\\'; ++i);
                                        if (i % 2 == 1) {
                                                in_string = 0;
                                        }
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
                        
                        // short comment starts
                        if (syntax_rules->short_comment != NULL) {
                                size_t sc_len = strlen(syntax_rules->short_comment);
                                if (line->len - x >= sc_len && !strncmp(syntax_rules->short_comment, line->items + x, sc_len)) {  // long comment ends
                                        map.items[y].items[x] = STT_COMMENT;
                                        for (; sc_len-->1; ++x) {
                                                map.items[y].items[x + 1] = STT_COMMENT;
                                        }
                                        in_short_comment = true;
                                        continue;
                                }
                        }

                        // long comment starts -- this is the same code as above consider abstraction
                        if (syntax_rules->long_comment_start != NULL) {
                                size_t lcs_len = strlen(syntax_rules->long_comment_start);
                                if (line->len - x >= lcs_len && !strncmp(syntax_rules->long_comment_start, line->items + x, lcs_len)) {  // long comment ends
                                        map.items[y].items[x] = STT_COMMENT;
                                        for (; lcs_len-->1; ++x) {
                                                map.items[y].items[x + 1] = STT_COMMENT;
                                        }
                                        in_long_comment = true;
                                        continue;
                                }
                        }

                        // string starts
                        for (const char *string_char = syntax_rules->string_chars; *string_char; ++string_char) {
                                if (c == *string_char) {
                                        map.items[y].items[x] = STT_STRING;
                                        in_string = c;
                                }
                        }
                        if (in_string) {
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
 *  - Check for a lone operator (not an alph character, one-wide token)
 *  - Check for a number (only containing digits)
 *  - POSSIBLE IN FUTURE: Check for a number in binary, octal, and hex form (0b, 0, and 0x respectively)
 *    - May not be done due to inconsistencies (with octals in Python being 0o and C just being 0)
 *  - Check for a constant value (all caps)
 *  - Check for a class name or something (starts with capital letter)
 *  - Check for a function call with TT_FUNCTION (so something like if (...) {...} isn't a function)
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
        
        if (!is_name_char(c) && token->start + 1 == token->end) {  // todo - prob modify to make this include $ and #
                return *get_highlighting_by_token_type(TT_OPERATOR);
        }

        if (isdigit(c)) {
                return *get_highlighting_by_token_type(TT_NUMBER);
        }

        if (syntax_rules != &HTML_RULES && syntax_rules != &MARKDOWN_RULES) {
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

                if (tt == STT_FUNCTION) {
                        return *get_highlighting_by_special_token_type(tt);
                }
        }

        return *get_highlighting_by_token_type(TT_NAME);
}
        