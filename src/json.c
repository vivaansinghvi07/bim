#include "utils.h"

typedef enum {
        JSON_NUM, JSON_STR, JSON_LIST, JSON_OBJ
} json_value_type_t;

typedef struct {
        ssize_t len;
        json_value_t *values;
} json_list_t;

typedef struct {
        ssize_t len;
        const char **keys;
        json_value_t *values;
} json_object_t;

typedef struct {
        json_value_type_t type;
        union {
                ssize_t num;
                const char *str;
                json_list_t list;
                json_object_t object;
        };
} json_value_t;

list_typedef(ssize_list, ssize_t);

static dyn_str *value = NULL;
dyn_str *generate_json_request(const json_value_t *json_value, const bool is_parent) {
        if (is_parent) {
                if (value) {
                        free_list_items(1, value);
                        free(value);
                }
                value = malloc(sizeof(dyn_str));
                *value = list_init(dyn_str, 10000);
        }

        switch (json_value->type) {
                case JSON_NUM: {
                        const char *num = num_to_str(json_value->num);
                        ssize_t len = strlen(num);
                        list_create_space(value, len);
                        memcpy(value.items - len, num, len);
                        free(num);
                } break;
                case JSON_STR: {
                        list_append(value, '"');
                        ssize_t len = strlen(json_value->string);
                        ssize_list quote_locations = list_init(ssize_list, 64);
                        for (ssize_t i = 0; i < len; ++i) {
                                if (json_value->string[i] == '"') {
                                        list_append(quote_locations, i);
                                }
                        }
                        for (ssize_t i = 0; i < quote_locations.len + 1; ++i) {
                                ssize_t start = i == 0 ? 0 : quote_locations.items[i - 1];
                                ssize_t end = i == quote_locations.len ? len : quote_locations.items[i];
                                list_create_space(value, end - start);
                                memcpy(value.items - (end - start), json_value->string + start, end - start);
                                if (end < quote_locations.len) {
                                        list_append(value, '\\');
                                }
                        }
                        list_append(value, '"');
                } break;
                case JSON_LIST: {
                        list_append(value, '[');
                        for (ssize_t i = 0; i < json_value->list.len; ++i) {
                                (void) generate_json_request(json_value->list.values + i, false);
                                if (i < json_value->list.len - 1) {
                                        list_append(value, ',');
                                }
                        }
                        list_append(value, ']');
                } break;
                case JSON_OBJ: {
                        list_append(value, '{');
                        for (ssize_t i = 0; i < json_value->object.len; ++i) {

                                list_append(value, '"');
                                ssize_t len = strlen(json_value->object.keys[i]);
                                list_create_space(value, len);
                                memcpy(value.items - len, json_value->object.keys[i], len);
                                list_append(value, '"');
                                list_append(value, ':');

                                (void) generate_json_request(json_value->object.values[i], false);

                                if (i < json_value->list.len - 1) {
                                        list_append(value, ',');
                                }
                        }
                        list_append(value, '}');
                }
        }

        return value;
}
