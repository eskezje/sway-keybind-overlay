#include "read_sway.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_lines(char **lines, size_t n) {
    for (size_t i = 0; i < n; i++) {
        free(lines[i]);
    }
    free(lines);
}

char **get_sway_config_lines(size_t *out_count) {
    *out_count = 0;

    const char *home = getenv("HOME");
    if (!home) {
        return NULL;
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/.config/sway/config", home);

    FILE *f = fopen(path, "r");
    if (!f) {
        return NULL;
    }

    const char *key = "bindsym";
    size_t key_len = strlen(key);

    size_t cap = 16;
    size_t count = 0;
    char **lines = malloc(cap * sizeof(*lines));
    if (!lines) {
        fclose(f);
        return NULL;
    }

    char line[512];

    while (fgets(line, sizeof(line), f)) {
        char *p = line;

        // skip whitespace
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (strncmp(p, key, key_len) == 0) {
            char *copy = malloc(strlen(p) + 1);
            if (!copy) {
                free_lines(lines, count);
                fclose(f);
                return NULL;
            }

            strcpy(copy, p);

            if (count == cap) {
                cap *= 2;
                char **tmp = realloc(lines, cap * sizeof(*lines));
                if (!tmp) {
                    free(copy);
                    free_lines(lines, count);
                    fclose(f);
                    return NULL;
                }
                lines = tmp;
            }

            lines[count++] = copy;
        }
    }

    fclose(f);

    *out_count = count;
    return lines;
}

void free_sway_config_lines(char **lines, size_t count) {
    free_lines(lines, count);
}
