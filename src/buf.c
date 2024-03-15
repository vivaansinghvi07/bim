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
	list_append(return_buffer->lines, list_init(dyn_str, 128));
        return_buffer->cursor_line = return_buffer->screen_top_line = 1;

        if (file == NULL) {
                return return_buffer;
        }

        fseek(file, 0, SEEK_END);
        size_t file_length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char buf[file_length + 1];
        fread(buf, sizeof(char), file_length, file);
        fclose(file);

        for (char *curr = buf; *curr; ++curr) {
                if (*curr == '\n') {
                        list_append(return_buffer->lines, list_init(dyn_str, 128));
                } else {
                        list_append(return_buffer->lines.items[return_buffer->lines.len - 1], *curr);
                }
        }
        return return_buffer;
}
