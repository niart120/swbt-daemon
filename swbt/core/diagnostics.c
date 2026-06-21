#include "core/diagnostics.h"

#include <stdio.h>
#include <stdlib.h>

bool swbt_diagnostic_path_is_enabled(const char *path) {
    return path != NULL && path[0] != '\0';
}

void swbt_diagnostic_trace_to_path(const char *path, const char *message) {
    FILE *file = NULL;

    if (!swbt_diagnostic_path_is_enabled(path) || message == NULL) {
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
