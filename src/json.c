#include "json.h"
#include "utils.h"

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
                                (void) generate_json_request(json_value->list.values + i, false);
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

                                (void) generate_json_request(json_value->object.values + i, false);

                                if (i < json_value->list.len - 1) {
                                        list_append(*value, ',');
                                }
                        }
                        list_append(*value, '}');
                }
        }

        return value;
}
