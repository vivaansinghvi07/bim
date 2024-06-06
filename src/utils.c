#include "utils.h"
#include "state.h"
#include "list.h"

#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <termios.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

bool resize_detected(const int sW, const int sH) {
        return __W != sW || __H != sH;
}

struct winsize get_window_size(void) {
        struct winsize w;
        if (ioctl(0, TIOCGWINSZ, &w) == -1) {
                exit_error("Could not determine terminal size.\n");
        }
        return w;
}

void update_screen_dimensions(void) {
        struct winsize w = get_window_size();
        __H = w.ws_row, __W = w.ws_col;
}

/*
 * Appends a '/' to the end of <path>, frees <path> and returns the replacement.
 */
char *append_slash(char *path) {
        const size_t path_len = strlen(path);
        char *output = malloc((path_len + 2) * sizeof(char));
        memcpy(output, path, path_len);
        output[path_len] = '/';
        output[path_len + 1] = '\0';
        free(path);
        return output;
}

void strip_whitespace(dyn_str *target) {
        for (; target->len && target->items[target->len - 1] == ' '; --target->len);
}

const char *fill_file_name(const char *dirname, const dyn_str *filename) {
        size_t dirname_len = strlen(dirname);
        char *new_name_str = malloc((dirname_len + filename->len + 1) * sizeof(char));
        memcpy(new_name_str, dirname, dirname_len);
        memcpy(new_name_str + dirname_len, filename->items, filename->len); 
        new_name_str[dirname_len + filename->len] = '\0';
        return new_name_str;
}

bool file_exists(const char *path) {
        struct stat s;
        return stat(path, &s) == 0;
}

bool is_parent_dir(const dyn_str *path) {
        return path->len == 3 && path->items[0] == '.' && path->items[1] == '.' && path->items[2] == '/';
}

bool is_curr_dir(const dyn_str *path) {
        return path->len == 2 && path->items[0] == '.' && path->items[1] == '/';
}

// check if the file is effectively the same - uses stat(2) from the man pages
bool is_same_file(const char *file, const char *other) {
        struct stat file_stat, other_stat;
        stat(file, &file_stat);
        stat(other, &other_stat);
        return file_stat.st_ino == other_stat.st_ino;
}

static FILE *logger;

void open_log_file() {
        logger = fopen("editor_log.txt", "a");
        setbuf(logger, NULL);
}

void editor_log(const char *format, ...) {
        if (!logger) {
                open_log_file();
        }
        va_list arg_ptr;
        va_start(arg_ptr, format);
        vfprintf(logger, format, arg_ptr);
        va_end(arg_ptr);
}

/* https://stackoverflow.com/questions/1056411/how-to-pass-variable-number-of-arguments-to-printf-sprintf */
void exit_error(const char *format, ...) {
        input_restore_tty();
        exit_altscr();
        va_list arg_ptr;
        va_start(arg_ptr, format);
        vfprintf(stderr, format, arg_ptr);
        va_end(arg_ptr);
        exit(1);
}

static struct termios tbufsave;

/*
 *  Implementation for the two below functions taken from stty source:
 *    https://github.com/wertarbyte/coreutils/blob/master/src/stty.c#L1162
 *  And this reference:
 *    https://www.scosales.com/ta/kb/100487.html
 */
int input_set_tty_raw(void) {
        struct termios tbuf;
        if (tcgetattr(0, &tbufsave) == -1) {
                return 1;
        }

        tbuf = tbufsave;
        tbuf.c_iflag &= ~(INLCR | ICRNL | ISTRIP | IXON | BRKINT);
        tbuf.c_oflag &= ~OPOST;
        tbuf.c_lflag &= ~(ICANON | ISIG | ECHO);
        tbuf.c_cc[VMIN] = 1;
        tbuf.c_cc[VTIME] = 0;

        return tcsetattr(0, TCSANOW, &tbuf) == -1;   // 1 for error
}

int input_restore_tty(void) {
        return tcsetattr(0, TCSANOW, &tbufsave) == -1;   // 1 for error
}

void move_to_top_left(void) {
        move_cursor_to(1, 1);
}

void move_cursor_to(const int y, const int x) {
        printf("\033[%d;%dH", y, x);
}

void clear_screen(void) {
#ifdef _WIN32
        system("cls");
#else 
        system("clear");
#endif
}

void enter_altscr(void) {
        printf("\033[?1049h");
}

void exit_altscr(void) {
        printf("\033[?1049l");
}

void hide_cursor(void) {
        printf("\033[?25l");
}

void show_cursor(void) {
        printf("\033[?25h");
}

void set_cursor_block(void) {
        printf("\033[2 q");
}

void set_cursor_bar(void) {
        printf("\033[6 q");
}

dyn_str *dyn_str_from_string(const char *str) {
        size_t len = strlen(str);
        char *target_str = malloc(len * sizeof(char));
        memcpy(target_str, str, len);
        dyn_str *retval = malloc(sizeof(dyn_str));
        retval->cap = retval->len = len;
        retval->items = target_str;
        return retval;
}

size_t num_len(const int n) {
        size_t len = snprintf(NULL, 0, "%d", n);
        return len;
}

const char *num_to_str(const ssize_t n) {
        char *buf = malloc(num_len(n) * sizeof(char) + 1);
        sprintf(buf, "%ld", n);
        return buf;
}

void set_timer(struct timespec *timer) {
        clock_gettime(CLOCK_MONOTONIC, timer);
}
double get_ms_elapsed(const struct timespec *start) {
        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);
        return (end.tv_sec - start->tv_sec) * 1e3 + (end.tv_nsec - start->tv_nsec) / 1e6;
}

// assumes c matches [A-Fa-f0-9]
uint8_t get_hex_value(const char c) {
        if (isdigit(c)) {
                return c - '0';
        } else if (islower(c)) {
                return c - 'a' + 10;
        } else {
                return c - 'A' + 10;
        }
}
 
// check if [_a-zA-Z0-9#$] matches <c>
bool is_name_char(const char c) {
        return c >= 'a' && c <= 'z' ||
               c >= 'A' && c <= 'Z' || 
               c >= '0' && c <= '9' ||
               c == '_' || c == '#' || 
               c == '$';
}

bool is_dir(const char *path) {
        struct stat path_stat;
        stat(path, &path_stat);
        return (bool) S_ISDIR(path_stat.st_mode);
}
