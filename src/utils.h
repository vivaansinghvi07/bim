#include "list.h"

// this file only exists because there were a lot of small files 
// and i just put everything together

#ifndef EDITOR_UTILS
#define EDITOR_UTILS

list_typedef(dyn_str, char);

void open_log_file();
void editor_log(const char *format, ...);

int input_set_tty_raw(void);
int input_restore_tty(void);

void clear_screen(void);
void hide_cursor(void);
void show_cursor(void);
void move_to_top_left(void);

dyn_str *dyn_str_from_string(const char *str);

void exit_error(const char *msg); 
size_t num_len(const int n);
const char *num_to_str(const int n);

#endif // !EDITOR_UTILS
