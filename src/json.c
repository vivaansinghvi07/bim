#include "json.h"
#include "utils.h"

#include <ctype.h>

list_typedef(ssize_list, ssize_t);
dyn_str *generate_json_request(const json_value_t *json_value, const bool is_parent) {
        static dyn_str *value = NULL;
        if (is_parent) {
                value = malloc(sizeof(dyn_str));
                *value = list_init(dyn_str, 10000);
        }

        switch (json_value->type) {
                case JSON_NUM: {
                        const char *num = num_to_str(json_value->num);
                        ssize_t len = strlen(num);
                        list_create_space(*value, len);
                        memcpy(value->items + value->len - len, num, len);
                        free((void *) num);
                } break;
                case JSON_STR: {
                        list_append(*value, '"');
                        ssize_t len = strlen(json_value->str);
                        ssize_list quote_locations = list_init(ssize_list, 64);
                        for (ssize_t i = 0; i < len; ++i) {
                                if (json_value->str[i] == '"') {
                                        list_append(quote_locations, i);
                                }
                        }
                        for (ssize_t i = 0; i < quote_locations.len + 1; ++i) {
                                ssize_t start = i == 0 ? 0 : quote_locations.items[i - 1];
                                ssize_t end = i == quote_locations.len ? len : quote_locations.items[i];
                                list_create_space(*value, end - start);
                                memcpy(value->items + value->len - (end - start), json_value->str + start, end - start);
                                if (i < quote_locations.len) {
                                        list_append(*value, '\\');
                                }
                        }
                        list_append(*value, '"');
                } break;
                case JSON_LIST: {
                        list_append(*value, '[');
                        for (ssize_t i = 0; i < json_value->list.len; ++i) {
                                (void) generate_json_request(json_value->list.values[i], false);
                                if (i < json_value->list.len - 1) {
                                        list_append(*value, ',');
                                }
                        }
                        list_append(*value, ']');
                } break;
                case JSON_OBJ: {
                        list_append(*value, '{');
                        for (ssize_t i = 0; i < json_value->object.len; ++i) {

                                list_append(*value, '"');
                                ssize_t len = strlen(json_value->object.keys[i]);
                                list_create_space(*value, len);
                                memcpy(value->items + value->len - len, json_value->object.keys[i], len);
                                list_append(*value, '"');
                                list_append(*value, ':');

                                (void) generate_json_request(json_value->object.values[i], false);

                                if (i < json_value->list.len - 1) {
                                        list_append(*value, ',');
                                }
                        }
                        list_append(*value, '}');
                }
        }

        return value;
}

void skip_whitespace(const char *str, ssize_t *i) {
        while (str[*i] == ' ' || str[*i] == '\t' || str[*i] == '\r' || str[*i] == '\n') {
                ++(*i);
        }
}

json_value_type_t determine_json_type(char c) {
        if (c == '[') {
                return JSON_LIST;
        } else if (c == '{') {
                return JSON_OBJ;
        } else if (c == '"') {
                return JSON_STR;
        } else if (isdigit(c)) {
                return JSON_NUM;
        } else {
                exit_error("Invalid JSON received: unable to determine value type.\n");
                exit(1);  // already called by exit_error but makes clangd stop crying
        }
}

list_typedef(json_value_list, json_value_t *);
list_typedef(string_list, const char *);
json_value_t *load_json_value(const char *str, bool is_parent) {
        static ssize_t i;
        if (is_parent) {
                i = 0;
        }
        skip_whitespace(str, &i);
        json_value_t *ret = malloc(sizeof(json_value_t));
        ret->type = determine_json_type(str[i]);
        
        switch (ret->type) {
                case JSON_NUM: {
                        char *endptr;
                        ret->num = strtoll(str + i, &endptr, 0);
                        i = endptr - str;
                } break;
                case JSON_STR: {
                        ++i;
                        ssize_t start = i;
                        for (bool passed_backslash; str[i] != '"' || passed_backslash; ++i) {
                                passed_backslash = str[i] == '\\';
                                if (!str[i]) {
                                        exit_error("Invalid JSON received: unterminated string detected.\n");
                                }
                        }
                        ret->str = strndup(str + start, i - start);
                        ++i;
                } break;
                case JSON_LIST: {
                        ++i;
                        skip_whitespace(str, &i);
                        if (str[i] == ']') {
                                ret->list = (json_list_t) {.len = 0, .values = NULL};
                                ++i;
                                break;
                        }
                        json_value_list values = list_init(json_value_list, 4);
                        while (true) {
                                list_append(values, load_json_value(str, false));
                                skip_whitespace(str, &i);
                                if (str[i] == ']') {
                                        break;
                                } else if (str[i] != ',') {
                                        exit_error("Invalid JSON received: expected comma after list item.\n");
                                }
                                ++i;
                                skip_whitespace(str, &i);
                        }
                        ret->list = (json_list_t) {.len = values.len, .values = values.items};
                        ++i;
                } break;
                case JSON_OBJ: {
                        ++i;
                        skip_whitespace(str, &i);
                        if (str[i] == '}') {
                                ret->object = (json_object_t) {.len = 0, .values = NULL, .keys = NULL};
                                ++i;
                                break;
                        }
                        string_list keys = list_init(string_list, 4);
                        json_value_list values = list_init(json_value_list, 4);
                        while (true) {

                                // parse key
                                if (str[i] != '"') {
                                        exit_error("Invalid JSON received: expected double quote in start of object key.\n");
                                }
                                ++i;
                                ssize_t start = i;
                                for (; str[i] && str[i] != '"'; ++i);
                                if (str[i] != '"') {
                                        exit_error("Invalid JSON received: unterminated object key detected.\n");
                                }
                                list_append(keys, strndup(str + start, i - start));

                                ++i;
                                skip_whitespace(str, &i);
                                if (str[i] != ':') {
                                        exit_error("Invalid JSON received: expected colon after object key.\n");
                                }
                                ++i;
                                skip_whitespace(str, &i);
                                list_append(values, load_json_value(str, false));
                                skip_whitespace(str, &i);

                                if (str[i] == '}') {
                                        break;
                                } else if (str[i] != ',') {
                                        exit_error("Invalid JSON received: expected comma after object value.\n");
                                }
                        }
                        ret->object = (json_object_t) {.len = keys.len, .keys = keys.items, .values = values.items};
                        ++i;
                } break;
        }
        return ret;
}

// this function should be used with values from the above function
// it recursively frees values in a tree structure according to what needs to be stored as a pointer vs what doesn't
void free_json_value(json_value_t *json_value) {

}
