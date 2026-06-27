#include "daemon/production_shutdown.h"

#include <stdbool.h>
#include <stdio.h>

static int expect_true(bool value, const char *label) {
    if (!value) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "expected true: %s\n", label);
        return 1;
    }
    return 0;
}

static int expect_false(bool value, const char *label) {
    if (value) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "expected false: %s\n", label);
        return 1;
    }
    return 0;
}

static int fake_shutdown_install(void *context, swbt_daemon_shutdown_request_t request_shutdown,
                                 void *request_context) {
    (void)context;
    (void)request_shutdown;
    (void)request_context;
    return 0;
}

static void fake_shutdown_uninstall(void *context) {
    (void)context;
}

static int shutdown_listener_validation_accepts_null_or_complete_listener(void) {
    const swbt_daemon_shutdown_listener_t valid = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    const swbt_daemon_shutdown_listener_t missing_install = {
        .uninstall = fake_shutdown_uninstall,
    };
    const swbt_daemon_shutdown_listener_t missing_uninstall = {
        .install = fake_shutdown_install,
    };

    int failed = 0;
    failed += expect_true(swbt_daemon_production_shutdown_listener_is_valid(NULL), "null listener");
    failed +=
        expect_true(swbt_daemon_production_shutdown_listener_is_valid(&valid), "complete listener");
    failed += expect_false(swbt_daemon_production_shutdown_listener_is_valid(&missing_install),
                           "missing install");
    failed += expect_false(swbt_daemon_production_shutdown_listener_is_valid(&missing_uninstall),
                           "missing uninstall");
    return failed;
}

int main(void) {
    int failed = 0;
    failed += shutdown_listener_validation_accepts_null_or_complete_listener();
    return failed == 0 ? 0 : 1;
}
