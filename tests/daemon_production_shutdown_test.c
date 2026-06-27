#include "daemon/production_shutdown.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    int execute_on_main_thread_calls;
    int finish_calls;
    int install_calls;
    int uninstall_calls;
    swbt_daemon_shutdown_request_t installed_request;
    void *installed_context;
} fake_shutdown_t;

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

static int expect_eq_int(int actual, int expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %d, got %d\n", label, expected, actual);
        return 1;
    }
    return 0;
}

static int fake_shutdown_install(void *context, swbt_daemon_shutdown_request_t request_shutdown,
                                 void *request_context) {
    fake_shutdown_t *fake = context;
    if (fake != NULL) {
        fake->install_calls += 1;
        fake->installed_request = request_shutdown;
        fake->installed_context = request_context;
    }
    return 0;
}

static void fake_shutdown_uninstall(void *context) {
    fake_shutdown_t *fake = context;
    if (fake != NULL) {
        fake->uninstall_calls += 1;
    }
}

static void
fake_execute_on_main_thread(void *context,
                            btstack_context_callback_registration_t *callback_registration) {
    fake_shutdown_t *fake = context;
    if (fake != NULL) {
        fake->execute_on_main_thread_calls += 1;
    }
    if (callback_registration != NULL && callback_registration->callback != NULL) {
        callback_registration->callback(callback_registration->context);
    }
}

static void fake_finish_shutdown(void *context) {
    fake_shutdown_t *fake = context;
    if (fake != NULL) {
        fake->finish_calls += 1;
    }
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

static int shutdown_request_schedules_main_thread_finish_once_without_host(void) {
    fake_shutdown_t fake = {0};
    const swbt_btstack_production_run_loop_port_t run_loop = {
        .execute_on_main_thread = fake_execute_on_main_thread,
    };
    swbt_daemon_production_shutdown_t shutdown;

    int failed = 0;
    failed += expect_true(
        swbt_daemon_production_shutdown_init(&shutdown,
                                             &(swbt_daemon_production_shutdown_config_t){
                                                 .run_loop = &run_loop,
                                                 .port_context = &fake,
                                                 .finish = fake_finish_shutdown,
                                                 .finish_context = &fake,
                                             }),
        "shutdown init");
    swbt_daemon_production_shutdown_prepare(&shutdown);
    swbt_daemon_production_shutdown_request(&shutdown);
    swbt_daemon_production_shutdown_request(&shutdown);

    failed += expect_eq_int(fake.execute_on_main_thread_calls, 1, "main thread calls");
    failed += expect_eq_int(fake.finish_calls, 1, "finish calls");
    failed += expect_false(shutdown.neutral_pending, "neutral pending");
    return failed;
}

static int shutdown_listener_install_passes_helper_request_context_and_uninstalls(void) {
    fake_shutdown_t fake = {0};
    const swbt_btstack_production_run_loop_port_t run_loop = {
        .execute_on_main_thread = fake_execute_on_main_thread,
    };
    const swbt_daemon_shutdown_listener_t listener = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_shutdown_t shutdown;

    int failed = 0;
    failed += expect_true(
        swbt_daemon_production_shutdown_init(&shutdown,
                                             &(swbt_daemon_production_shutdown_config_t){
                                                 .run_loop = &run_loop,
                                                 .port_context = &fake,
                                                 .finish = fake_finish_shutdown,
                                                 .finish_context = &fake,
                                             }),
        "shutdown init");
    failed +=
        expect_eq_int(swbt_daemon_production_shutdown_install_listener(&shutdown, &listener, &fake),
                      0, "listener install");
    failed += expect_eq_int(fake.install_calls, 1, "install calls");
    failed += expect_true(fake.installed_request == swbt_daemon_production_shutdown_request,
                          "installed request");
    failed += expect_true(fake.installed_context == &shutdown, "installed context");

    swbt_daemon_production_shutdown_uninstall_listener(&listener, &fake);
    failed += expect_eq_int(fake.uninstall_calls, 1, "uninstall calls");
    return failed;
}

int main(void) {
    int failed = 0;
    failed += shutdown_listener_validation_accepts_null_or_complete_listener();
    failed += shutdown_request_schedules_main_thread_finish_once_without_host();
    failed += shutdown_listener_install_passes_helper_request_context_and_uninstalls();
    return failed == 0 ? 0 : 1;
}
