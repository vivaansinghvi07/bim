#include "utils.h"
#include "list.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <termios.h>
#include <stdarg.h>

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
        printf("\033[1;1H"); 
}

void clear_screen(void) {
#ifdef _WIN32
        system("cls");
#else 
        system("clear");
#endif
}

void hide_cursor(void) {
        printf("\x1b[?25l");
}

void show_cursor(void) {
        printf("\x1b[?25h");
}

dyn_str *dyn_str_from_string(const char *str) {
        uint64_t len = strlen(str);
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

const char *num_to_str(const int n) {
        char *buf = malloc(num_len(n) * sizeof(char) + 1);
        sprintf(buf, "%d", n);
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

