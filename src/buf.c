#include "buf.h"
#include "list.h"
#include "utils.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// in hindsight this whole function is sorta unncessesary because
// the OS automatically frees memory at the end of program execution
void buf_free_list(buf_list *buffers) {
        for (size_t i = 0; i < buffers->len; ++i) {
                buf_free(buffers->items[i]);
        }
        free_list_items(1, buffers);
}

// because buf_open always allocates memory, it is freed in this function
void buf_free(file_buf *buf) {
        for (size_t i = 0; i < buf->lines.len; ++i) {
                free_list_items(1, buf->lines.items + i); 
        }
        free_list_items(1, &buf->lines);
        free(buf);
}

void buf_save(const file_buf *buf) {
        FILE *file = fopen(buf->filename, "w");
        for (size_t i = 0; i < buf->lines.len; ++i) {
                dyn_str *line = buf->lines.items + i;
                fwrite(line->items, sizeof(char), line->len, file);
        }
        fclose(file);
}

file_buf *buf_open(const char *filename) {

        FILE *file = fopen(filename, "r");
        file_buf *return_buffer = malloc(sizeof(file_buf));
        return_buffer->filename = filename;
        return_buffer->lines = list_init(dyn_contents, 128);
	list_append(return_buffer->lines, list_init(dyn_str, 128));
        return_buffer->cursor_line = 
                return_buffer->screen_top_line = 
                        return_buffer->cursor_col = 1;

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
