#ifndef SWBT_CORE_DIAGNOSTICS_H
#define SWBT_CORE_DIAGNOSTICS_H

#include <stdbool.h>

bool swbt_diagnostic_path_is_enabled(const char *path);

void swbt_diagnostic_trace_to_path(const char *path, const char *message);

void swbt_diagnostic_trace(const char *message);

#endif
