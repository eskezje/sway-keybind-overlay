
#ifndef READ_SWAY_H
#define READ_SWAY_H

#include <stddef.h>

char **get_sway_config_lines(size_t *out_count);

void free_sway_config_lines(char **lines, size_t count);

#endif
