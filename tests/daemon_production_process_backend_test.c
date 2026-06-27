#include "daemon/production_process_backend.h"

#include <stdbool.h>
#include <stdio.h>

#include "domain/domain.h"
#include "daemon/production_runner.h"

typedef struct {
    int output_handler_start_calls;
    int output_handler_stop_calls;
    swbt_btstack_output_report_handler_t *output_handler;
} fake_ops_t;

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

static int fake_ipc_start(void *context, const swbt_btstack_production_ipc_pump_t *pump) {
    (void)context;
    (void)pump;
    return 0;
}

static void fake_ipc_stop(void *context) {
    (void)context;
}

static void fake_output_handler_start(void *context,
                                      swbt_btstack_output_report_handler_t *handler) {
    fake_ops_t *fake = context;
    fake->output_handler_start_calls += 1;
    fake->output_handler = handler;
}

static void fake_output_handler_stop(void *context) {
    fake_ops_t *fake = context;
    fake->output_handler_stop_calls += 1;
}

static swbt_btstack_production_ports_t fake_ports(void) {
    return (swbt_btstack_production_ports_t){
        .ipc_pump =
            {
                .start = fake_ipc_start,
                .stop = fake_ipc_stop,
            },
        .output_handler =
            {
                .start = fake_output_handler_start,
                .stop = fake_output_handler_stop,
            },
    };
}

static int process_backend_table_exposes_production_backend_status(void) {
    const swbt_daemon_process_backend_t *backend = swbt_daemon_production_process_backend();

    int failed = 0;
    failed += expect_true(backend != NULL, "backend");
    failed += expect_eq_int((int)backend->daemon_backend,
                            (int)SWBT_DOMAIN_DAEMON_BACKEND_PRODUCTION, "daemon backend");
    return failed;
}

static int process_backend_routes_output_handler_start_and_stop(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t ports = fake_ports();
    const swbt_daemon_process_backend_t *process_backend = swbt_daemon_production_process_backend();
    swbt_daemon_production_runner_t runner;
    swbt_btstack_output_report_handler_t output_handler = {0};

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&runner, &config, &ports, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "runner init");
    process_backend->output_handler_start(&runner, &output_handler);
    process_backend->output_handler_stop(&runner);

    failed += expect_eq_int(fake.output_handler_start_calls, 1, "output handler start calls");
    failed += expect_true(fake.output_handler == &output_handler, "output handler");
    failed += expect_eq_int(fake.output_handler_stop_calls, 1, "output handler stop calls");
    return failed;
}

int main(void) {
    int failed = 0;
    failed += process_backend_table_exposes_production_backend_status();
    failed += process_backend_routes_output_handler_start_and_stop();
    return failed == 0 ? 0 : 1;
}
