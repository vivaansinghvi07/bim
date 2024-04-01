#include "list.h"

// this file only exists because there were a lot of small files 
// and i just put everything together

#ifndef EDITOR_UTILS
#define EDITOR_UTILS

// i'm not good enough at C to understand why this typedef makes
// clangd warnings go away in the main.c file, but it does
typedef struct timespec _;

list_typedef(dyn_str, char);

// this makes the warnings from clangd about
// typedef struct timespec _;

void open_log_file();
void editor_log(const char *format, ...);

int input_set_tty_raw(void);
int input_restore_tty(void);

void clear_screen(void);
void hide_cursor(void);
void show_cursor(void);
void move_to_top_left(void);
void move_cursor_to(const int x, const int y);

dyn_str *dyn_str_from_string(const char *str);

void exit_error(const char *format, ...); 
size_t num_len(const int n);
const char *num_to_str(const int n);

void set_timer(struct timespec *timer);
double get_ms_elapsed(const struct timespec *start);

int store_cursor_pos(int *y, int *x);

#endif // !EDITOR_UTILS
