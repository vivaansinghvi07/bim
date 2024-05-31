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

static dyn_str value;
char *generate_json_request(json_value_t *json_value, bool is_parent) {
        if (is_parent) {
                value = list_init(dyn_str, 10000);
                curr = 0;
        }

        switch (json_value->type) {
                case JSON_NUM:
                case JSON_STR:
                case JSON_LIST: {
                        list_append(value, '[');
                        for (ssize_t i = 0; i < json_value->json_list.len; ++i) {
                                if (i < json_value->json_list.len - 1) {
                                        list_append(value, ',');
                                }
                                generate_json_request(json_value->json_list.values + i);
                        }
                        list_append(value, ']');
                }
                case JSON_OBJ: {
                        list_append(value, '{');
                        list_append(value, '}');
                }
        }

        return value;
}
