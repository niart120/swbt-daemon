#include "core/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>

void swbt_diagnostic_trace(const char *message) {
    const char *path = getenv("SWBT_DIAGNOSTIC_TRACE_PATH");
    FILE *file = NULL;

    if (path == NULL || path[0] == '\0' || message == NULL) {
        return;
    }

    file = fopen(path, "ab");
    if (file == NULL) {
        return;
    }
    fputs(message, file);
    fputc('\n', file);
    fclose(file);
}
