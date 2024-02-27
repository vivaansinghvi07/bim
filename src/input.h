#ifndef EDITOR_INPUT
#define EDITOR_INPUT

/*
 *  Implementation for the two below functions taken from stty source:
 *    https://github.com/wertarbyte/coreutils/blob/master/src/stty.c#L1162
 *  And this reference:
 *    https://www.scosales.com/ta/kb/100487.html
 */
int input_set_tty_raw(void);
int input_restore_tty(void);

#endif
