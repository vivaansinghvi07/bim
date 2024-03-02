#include "buf.h"
#include "list.h"
#include "utils.h"
#include "str.h" 

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

file_buf *buf_open(const char *filename) {

        FILE *file = fopen(filename, "r");
        file_buf *return_buffer = malloc(sizeof(file_buf));
        return_buffer->filename = *dyn_str_from_string(filename);
        return_buffer->lines = list_init(dyn_contents, 128);

        if (file == NULL) {
                return_buffer->lines.items[0] = list_init(dyn_str, 256);
                return return_buffer;
        }

        fseek(file, 0, SEEK_END);
        uint64_t file_length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char buf[file_length + 1];
        fread(buf, sizeof(char), file_length, file);
        fclose(file);

        char *curr = buf - 1;
        while (*(++curr) != '\0') {
                list_append(return_buffer->lines.items[return_buffer->lines.len - 1], *curr);
                if (*curr == '\n') {
                        list_append(return_buffer->lines, list_init(dyn_str, 128));
                }
        }
        return return_buffer;
}
