// NOLINTNEXTLINE(bugprone-reserved-identifier): POSIX feature test macro.
#define _POSIX_C_SOURCE 200112L

#include "core/diagnostics.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int set_trace_env(const char *path) {
#if defined(_WIN32)
    return SetEnvironmentVariableA("SWBT_DIAGNOSTIC_TRACE_PATH", path) ? 0 : 1;
#else
    return setenv("SWBT_DIAGNOSTIC_TRACE_PATH", path, 1);
#endif
}

static void clear_trace_env(void) {
#if defined(_WIN32)
    (void)SetEnvironmentVariableA("SWBT_DIAGNOSTIC_TRACE_PATH", NULL);
#else
    unsetenv("SWBT_DIAGNOSTIC_TRACE_PATH");
#endif
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): test helper names both arguments.
static int expect_contains(const char *path, const char *needle) {
    FILE *file = fopen(path, "rb");
    char buffer[256];
    size_t read = 0;
    int result = 1;

    if (file == NULL) {
        return 1;
    }
    read = fread(buffer, 1, sizeof(buffer) - 1u, file);
    fclose(file);
    buffer[read] = '\0';
    if (strstr(buffer, needle) != NULL) {
        result = 0;
    }
    remove(path);
    return result;
}

int main(void) {
    const char *path = "diagnostics-test-trace.log";

    remove(path);
    if (set_trace_env(path) != 0) {
        return 1;
    }
    swbt_diagnostic_trace("diagnostics_test marker");
    clear_trace_env();

    return expect_contains(path, "diagnostics_test marker");
}
