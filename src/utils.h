#include "list.h"

#ifndef EDITOR_UTILS
#define EDITOR_UTILS

list_typedef(dyn_str, char);

dyn_str *dyn_str_from_string(const char *str);
void exit_error(const char *msg); 
int num_len(int n);
const char *num_to_str(int n);

#endif // !EDITOR_UTILS
