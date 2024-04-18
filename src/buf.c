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
void buf_free(buf_t *buf) {
        for (size_t i = 0; i < buf->lines.len; ++i) {
                free_list_items(1, buf->lines.items + i); 
        }
        free_list_items(1, &buf->lines);
        free(buf);
}

void buf_save(const buf_t *buf) {
        FILE *file = fopen(buf->filename, "w");
        const char newline = '\n';
        for (size_t i = 0; i < buf->lines.len; ++i) {
                dyn_str *line = buf->lines.items + i;
                fwrite(line->items, sizeof(char), line->len, file);
                if (i < buf->lines.len - 1) {
                        fwrite(&newline, sizeof(char), 1, file);
                }
        }
        fclose(file);
}

void assert_valid_file(const char *filename) {
        FILE *file = fopen(filename, "w");
        if (file == NULL) {
                // TODO: add error display feature
        }
}

buf_t *buf_open(const char *filename, const int tab_width) {

        assert_valid_file(filename);
        FILE *file = fopen(filename, "r");
        buf_t *return_buffer = malloc(sizeof(buf_t));
        *return_buffer = (buf_t) {
                .filename = filename, 
                .cursor_col = 1,
                .cursor_line = 1,
                .screen_left_col = 1,
                .screen_top_line = 1,
                .lines = list_init(dyn_contents, MIN_NEW_LINE_LEN)
        };
	list_append(return_buffer->lines, list_init(dyn_str, MIN_NEW_LINE_LEN));

        if (file == NULL) {
                return return_buffer;
        }

        fseek(file, 0, SEEK_END);
        size_t file_length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char buf[file_length + 1];
        fread(buf, sizeof(char), file_length, file);
        buf[file_length] = '\0';
        fclose(file);

        for (char *curr = buf; *curr; ++curr) {
                if (*curr == '\n') {
                        list_append(return_buffer->lines, list_init(dyn_str, 128));
                } else if (*curr == '\t') { 
                        for (int i = 0; i < tab_width; ++i) { 
                                list_append(return_buffer->lines.items[return_buffer->lines.len - 1], ' ');
                        }
                } else {
                        list_append(return_buffer->lines.items[return_buffer->lines.len - 1], *curr);
                }
        }
        return return_buffer;
}
