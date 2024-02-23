#include "list.h"

#ifndef EDITOR_DYN_STR
#define EDITOR_DYN_STR

list_typedef(dyn_str, char);

dyn_str *dyn_str_from_string(const char *str);

#endif // !EDITOR_DYN_STR
