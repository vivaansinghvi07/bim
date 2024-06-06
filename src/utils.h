// this file only exists because there were a lot of small files 
// and i just put everything together

#ifndef EDITOR_UTILS
#define EDITOR_UTILS

#include "list.h"

#include <stdint.h>
#include <stdbool.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// i'm not good enough at C to understand why this typedef makes
// clangd warnings go away in the main.c file, but it does
typedef struct timespec _;

list_typedef(dyn_str, char);

const int H(void);
const int W(void);

bool resize_detected(const int sW, const int sH);
struct winsize get_window_size(void);
void update_screen_dimensions(void);

char *append_slash(char *path);
void strip_whitespace(dyn_str *target);
const char *fill_file_name(const char *dirname, const dyn_str *filename);

bool file_exists(const char *path);
bool is_curr_dir(const dyn_str *path);
bool is_parent_dir(const dyn_str *path);
bool is_same_file(const char *file, const char *other);

void open_log_file();
void editor_log(const char *format, ...);

int input_set_tty_raw(void);
int input_restore_tty(void);

void clear_screen(void);
void hide_cursor(void);
void show_cursor(void);
void set_cursor_block(void);
void set_cursor_bar(void);
void enter_altscr(void);
void exit_altscr(void);
void move_to_top_left(void);
void move_cursor_to(const int y, const int x);

dyn_str *dyn_str_from_string(const char *str);

void exit_error(const char *format, ...); 
size_t num_len(const int n);
const char *num_to_str(const ssize_t n);

void set_timer(struct timespec *timer);
double get_ms_elapsed(const struct timespec *start);

uint8_t get_hex_value(const char c);
bool is_name_char(const char c);
bool is_dir(const char *path);

#endif // !EDITOR_UTILS
