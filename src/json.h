#ifndef EDITOR_JSON
#define EDITOR_JSON

#include "utils.h"

#include <stdbool.h>
#include <sys/types.h>

typedef enum {
        JSON_NUM, JSON_STR, JSON_LIST, JSON_OBJ
} json_value_type_t;

typedef struct _json_value_t json_value_t;

typedef struct {
        ssize_t len;
        json_value_t **values;
} json_list_t;

typedef struct {
        ssize_t len;
        const char **keys;
        json_value_t **values;
} json_object_t;

typedef struct _json_value_t {
        json_value_type_t type;
        union {
                ssize_t num;
                const char *str;
                json_list_t list;
                json_object_t object;
        };
} json_value_t;

dyn_str *generate_json_string(const json_value_t *json_value, const bool is_parent);
json_value_t *load_json_value(const char *str, bool is_parent);

#endif
