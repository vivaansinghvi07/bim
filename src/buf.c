#include "buf.h"
#include "state.h"
#include "list.h"
#include "utils.h"

#include <dirent.h>
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

bool is_valid_file(const char *filename) {
        FILE *read_handle = fopen(filename, "r");
        FILE *file = fopen(filename, "a");
        if (file == NULL) {
                return false;
        } else {
                fclose(file);
                if (read_handle == NULL) {
                        remove(filename);
                } else {
                        fclose(read_handle);
                }
                return true;
        }
}

void buf_init(buf_t *buffer, const char *filename) {
        *buffer = (buf_t) {
                .filename = filename,
                .cursor_col = 1,
                .cursor_line = 1,
                .screen_left_col = 1,
                .screen_top_line = 1,
                .lines = list_init(dyn_contents, MIN_NEW_LINE_LEN)
        };
}

int alphabetical_comparer(const dyn_str *first, const dyn_str *second) {
        int diff;
        for (size_t i = 0; i < first->len || i < second->len; ++i) {
                if (i == first->len) {
                        return -1;
                } else if (i == second->len) {
                        return 1;
                } else if ((diff = first->items[i] - second->items[i])) {
                        return diff;
                }
        }
        return 0;  // should never happen in the below use case
}

int file_comparer(const void *_first, const void *_second) {
        const dyn_str *first = _first, *second = _second;
        if (is_same_dir(first) || is_same_dir(second)) {
                return is_same_dir(second) - is_same_dir(first);
        } else if (is_parent_dir(first) || is_parent_dir(second)) {
                return is_parent_dir(second) - is_parent_dir(first);
        }
        
        bool first_is_dir = first->items[first->len - 1] == '/',
             second_is_dir = second->items[second->len - 1] == '/';
        if (first_is_dir ^ second_is_dir) {
                return second_is_dir - first_is_dir;
        }
        return alphabetical_comparer(first, second);
}

/*
 * Fills the file view buffer with directory information about the diretory 
 *  stored in the <filename> attribute.
 */
void buf_fill_files_view(buf_t *buf) {
        for (int i = 0; i < buf->lines.len; ++i) {
                free_list_items(1, buf->lines.items + i);
        }
        buf->lines.len = 0;
        DIR *dir;
        struct dirent *dirent;
        dir = opendir(buf->filename);
        if (dir) {
                while ((dirent = readdir(dir)) != NULL) {
                        if (dirent->d_type == DT_REG || dirent->d_type == DT_DIR) {
                                dyn_str s = *dyn_str_from_string(dirent->d_name);
                                if (dirent->d_type == DT_DIR) {
                                        list_append(s, '/');
                                }
                                list_append(buf->lines, s);
                        }
                }
                closedir(dir);
                qsort(buf->lines.items, buf->lines.len, sizeof(*buf->lines.items), &file_comparer);
        }
}

buf_t *buf_open(const char *filename, const int tab_width) {

        if (!is_valid_file(filename)) {
                return NULL;
        }

        FILE *file = fopen(filename, "r");
        buf_t *return_buffer = malloc(sizeof(buf_t));
        buf_init(return_buffer, filename);
        list_append(return_buffer->lines, list_init(dyn_str, MIN_NEW_LINE_LEN));

        if (file == NULL) {
                return return_buffer;
        }

        fseek(file, 0, SEEK_END);
        size_t file_length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *buf = malloc((file_length + 1) * sizeof(char));
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
        free(buf);
        return return_buffer;
}
