#include "daemon/production_process_backend.h"

#include <stdbool.h>
#include <stdio.h>

#include "domain/domain.h"

static int expect_true(bool value, const char *label) {
    if (!value) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "expected true: %s\n", label);
        return 1;
    }
    return 0;
}

static int expect_eq_int(int actual, int expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %d, got %d\n", label, expected, actual);
        return 1;
    }
    return 0;
}

static int process_backend_table_exposes_production_backend_status(void) {
    const swbt_daemon_process_backend_t *backend = swbt_daemon_production_process_backend();

    int failed = 0;
    failed += expect_true(backend != NULL, "backend");
    failed += expect_eq_int((int)backend->daemon_backend,
                            (int)SWBT_DOMAIN_DAEMON_BACKEND_PRODUCTION, "daemon backend");
    return failed;
}

int main(void) {
    int failed = 0;
    failed += process_backend_table_exposes_production_backend_status();
    return failed == 0 ? 0 : 1;
}
