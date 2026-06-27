#include "daemon/production_process_backend.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "domain/domain.h"
#include "daemon/production_runner.h"

typedef struct {
    int output_handler_start_calls;
    int output_handler_stop_calls;
    int read_controller_address_calls;
    uint32_t now_ms;
    swbt_btstack_output_report_handler_t *output_handler;
    uint8_t controller_address[6];
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

static int expect_eq_u8(uint8_t actual, uint8_t expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %u, got %u\n", label, (unsigned)expected, (unsigned)actual);
        return 1;
    }
    return 0;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %u, got %u\n", label, (unsigned)expected, (unsigned)actual);
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

static int fake_read_controller_address(void *context, uint8_t address[6]) {
    fake_ops_t *fake = context;
    fake->read_controller_address_calls += 1;
    for (size_t index = 0; index < sizeof(fake->controller_address); ++index) {
        address[index] = fake->controller_address[index];
    }
    return 0;
}

static uint32_t fake_time_ms(void *context) {
    const fake_ops_t *fake = context;
    return fake->now_ms;
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
        .controller =
            {
                .read_controller_address = fake_read_controller_address,
            },
        .clock =
            {
                .time_ms = fake_time_ms,
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

static int process_backend_reads_configured_device_info_and_controller_address(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .controller_address = {0x01u, 0x23u, 0x45u, 0x67u, 0x89u, 0xABu},
    };
    const swbt_btstack_production_ports_t ports = fake_ports();
    const swbt_daemon_process_backend_t *process_backend = swbt_daemon_production_process_backend();
    swbt_daemon_production_runner_t runner;
    swbt_switch_device_info_t device_info = {0};

    config.device_info.firmware_version[0] = 0x12u;
    config.device_info.firmware_version[1] = 0x34u;
    config.device_info.controller_type = 0x03u;
    config.device_info.tail_unknown = 0x02u;
    config.device_info.color_source = 0x01u;

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&runner, &config, &ports, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "runner init");
    failed += expect_eq_int(process_backend->read_device_info(&runner, &device_info), 0,
                            "read device info");

    failed += expect_eq_int(fake.read_controller_address_calls, 1, "read controller address calls");
    failed += expect_eq_u8(device_info.firmware_version[0], 0x12u, "firmware version 0");
    failed += expect_eq_u8(device_info.firmware_version[1], 0x34u, "firmware version 1");
    failed += expect_eq_u8(device_info.controller_type, 0x03u, "controller type");
    failed += expect_eq_u8(device_info.tail_unknown, 0x02u, "tail unknown");
    failed += expect_eq_u8(device_info.color_source, 0x01u, "color source");
    for (size_t index = 0; index < sizeof(fake.controller_address); ++index) {
        failed += expect_eq_u8(device_info.bluetooth_address[index], fake.controller_address[index],
                               "controller address");
    }
    return failed;
}

static int process_backend_time_ms_delegates_to_clock_port(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .now_ms = 9876u,
    };
    const swbt_btstack_production_ports_t ports = fake_ports();
    const swbt_daemon_process_backend_t *process_backend = swbt_daemon_production_process_backend();
    swbt_daemon_production_runner_t runner;

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&runner, &config, &ports, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "runner init");
    failed += expect_eq_u32(process_backend->time_ms(&runner), 9876u, "time ms");
    return failed;
}

int main(void) {
    int failed = 0;
    failed += process_backend_table_exposes_production_backend_status();
    failed += process_backend_routes_output_handler_start_and_stop();
    failed += process_backend_reads_configured_device_info_and_controller_address();
    failed += process_backend_time_ms_delegates_to_clock_port();
    return failed == 0 ? 0 : 1;
}
