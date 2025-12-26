#include "read_sway.h"
#include <stdio.h>

int main(void) {
    size_t n = 0;
    char **lines = get_sway_config_lines(&n);
    if (!lines) {
        fprintf(stderr, "failed to read sway config\n");
        return 1;
    }

    for (size_t i = 0; i < n; i++) {
        printf("%s", lines[i]);
    }

    free_sway_config_lines(lines, n);
    return 0;
}
