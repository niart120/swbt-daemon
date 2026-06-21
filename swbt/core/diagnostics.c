#include "core/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>

void swbt_diagnostic_trace_to_path(const char *path, const char *message) {
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

void swbt_diagnostic_trace(const char *message) {
    swbt_diagnostic_trace_to_path(getenv("SWBT_DIAGNOSTIC_TRACE_PATH"), message);
}
