#include <sys/ioctl.h>
#include "buf.h"

struct winsize get_window_size(void);
int display_buffer(file_buf *buffer);
