#include "daemon/production_hid_session.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "btstack_bridge/device.h"
#include "btstack_bridge/production_ports.h"
#include "daemon/config.h"

typedef struct {
    int platform_start_calls;
    int hid_register_calls;
    int hid_stop_calls;
    int platform_stop_calls;
    int timer_start_calls;
    int timer_can_send_calls;
    int timer_stop_calls;
    int finish_shutdown_calls;
    uint16_t timer_hid_cid;
    uint64_t timer_now_us;
    uint32_t clock_time_ms;
    uint8_t *captured_service_buffer;
    size_t captured_service_buffer_size;
    swbt_btstack_hid_registration_config_t captured_registration;
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

static int expect_eq_u16(uint16_t actual, uint16_t expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %u, got %u\n", label, (unsigned)expected, (unsigned)actual);
        return 1;
    }
    return 0;
}

static int expect_eq_u64(uint64_t actual, uint64_t expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %llu, got %llu\n", label, (unsigned long long)expected,
                (unsigned long long)actual);
        return 1;
    }
    return 0;
}

static int fake_platform_start(void *context) {
    fake_ops_t *fake = context;
    fake->platform_start_calls += 1;
    return 0;
}

static void fake_platform_stop(void *context) {
    fake_ops_t *fake = context;
    fake->platform_stop_calls += 1;
}

static int fake_hid_register(void *context, uint8_t *service_buffer, size_t service_buffer_size,
                             const swbt_btstack_hid_registration_config_t *config) {
    fake_ops_t *fake = context;
    fake->hid_register_calls += 1;
    fake->captured_service_buffer = service_buffer;
    fake->captured_service_buffer_size = service_buffer_size;
    fake->captured_registration = *config;
    return 0;
}

static void fake_hid_stop(void *context) {
    fake_ops_t *fake = context;
    fake->hid_stop_calls += 1;
}

static int fake_connect(void *context, const swbt_btstack_device_connect_request_t *request,
                        uint16_t *out_hid_cid) {
    (void)context;
    (void)request;
    (void)out_hid_cid;
    return -1;
}

static int fake_send(void *context, uint16_t hid_cid, const uint8_t *message, size_t message_size) {
    (void)context;
    (void)hid_cid;
    (void)message;
    (void)message_size;
    return -1;
}

static swbt_btstack_device_port_t fake_device_port(void) {
    return (swbt_btstack_device_port_t){
        .platform_start = fake_platform_start,
        .platform_stop = fake_platform_stop,
        .hid_register = fake_hid_register,
        .hid_stop = fake_hid_stop,
        .connect = fake_connect,
        .send = fake_send,
    };
}

static int fake_timer_start(void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
                            swbt_btstack_input_report_timer_start_options_t options) {
    fake_ops_t *fake = context;
    fake->timer_start_calls += 1;
    fake->timer_hid_cid = options.hid_cid;
    fake->timer_now_us = options.now_us;
    if (adapter != NULL) {
        adapter->hid_cid = options.hid_cid;
        adapter->running = true;
    }
    return 0;
}

static int fake_timer_on_can_send_now(void *context,
                                      swbt_btstack_input_report_timer_adapter_t *adapter) {
    fake_ops_t *fake = context;
    (void)adapter;
    fake->timer_can_send_calls += 1;
    return 0;
}

static void fake_timer_stop(void *context, swbt_btstack_input_report_timer_adapter_t *adapter) {
    fake_ops_t *fake = context;
    fake->timer_stop_calls += 1;
    if (adapter != NULL) {
        adapter->running = false;
    }
}

static swbt_btstack_production_report_timer_port_t fake_report_timer_port(void) {
    return (swbt_btstack_production_report_timer_port_t){
        .start = fake_timer_start,
        .on_can_send_now = fake_timer_on_can_send_now,
        .stop = fake_timer_stop,
    };
}

static uint32_t fake_time_ms(void *context) {
    const fake_ops_t *fake = context;
    return fake->clock_time_ms;
}

static swbt_btstack_production_clock_port_t fake_clock_port(void) {
    return (swbt_btstack_production_clock_port_t){
        .time_ms = fake_time_ms,
    };
}

static void fake_finish_shutdown(void *context) {
    fake_ops_t *fake = context;
    fake->finish_shutdown_calls += 1;
}

static int hid_session_register_opens_and_stop_closes_device(void) {
    fake_ops_t fake = {0};
    const swbt_btstack_device_port_t device_port = fake_device_port();
    swbt_btstack_device_t device = {0};
    uint8_t service_buffer[512] = {0};
    swbt_daemon_production_hid_session_t session = {
        .device_port = &device_port,
        .port_context = &fake,
        .device = &device,
        .service_buffer = service_buffer,
        .service_buffer_size = sizeof(service_buffer),
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_hid_session_register(&session), 0, "register");
    failed += expect_eq_int(fake.platform_start_calls, 1, "platform start");
    failed += expect_eq_int(fake.hid_register_calls, 1, "hid register");
    failed += expect_true(fake.captured_service_buffer == service_buffer, "service buffer");
    failed += expect_eq_int((int)fake.captured_service_buffer_size, (int)sizeof(service_buffer),
                            "service buffer size");
    failed += expect_true(fake.captured_registration.packet_handler != NULL, "packet handler");
    failed += expect_true(swbt_btstack_device_is_open(&device), "device open");

    swbt_daemon_production_hid_session_stop(&session);
    failed += expect_eq_int(fake.hid_stop_calls, 1, "hid stop");
    failed += expect_eq_int(fake.platform_stop_calls, 1, "platform stop");
    failed += expect_true(!swbt_btstack_device_is_open(&device), "device closed");
    return failed;
}

static int hid_session_connection_opened_starts_timer_with_hid_cid(void) {
    fake_ops_t fake = {
        .clock_time_ms = 123u,
    };
    const swbt_btstack_device_port_t device_port = fake_device_port();
    const swbt_btstack_production_report_timer_port_t timer_port = fake_report_timer_port();
    const swbt_btstack_production_clock_port_t clock_port = fake_clock_port();
    swbt_daemon_config_t config = swbt_daemon_config_default();
    swbt_btstack_device_t device = {0};
    swbt_btstack_input_report_timer_adapter_t timer = {0};
    uint8_t service_buffer[512] = {0};
    bool timer_initialized = true;
    bool shutdown_pending = false;
    uint8_t opened_event[] = {0xefu, 13u, 0x02u, 0x42u, 0x00u, 0x00u, 0u, 0u,
                              0u,    0u,  0u,    0u,    0u,    0u,    1u};
    swbt_daemon_production_hid_session_t session = {
        .config = &config,
        .device_port = &device_port,
        .report_timer_port = &timer_port,
        .clock_port = &clock_port,
        .port_context = &fake,
        .device = &device,
        .report_timer = &timer,
        .report_timer_initialized = &timer_initialized,
        .shutdown_neutral_pending = &shutdown_pending,
        .service_buffer = service_buffer,
        .service_buffer_size = sizeof(service_buffer),
        .finish_shutdown = fake_finish_shutdown,
        .finish_shutdown_context = &fake,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_hid_session_register(&session), 0, "register");
    fake.captured_registration.packet_handler(0x04u, 0x0042u, opened_event, sizeof(opened_event));

    failed += expect_eq_int(fake.timer_start_calls, 1, "timer start calls");
    failed += expect_eq_u16(fake.timer_hid_cid, 0x0042u, "timer hid cid");
    failed += expect_eq_u64(fake.timer_now_us, 123000u, "timer now us");
    failed += expect_eq_int(fake.timer_can_send_calls, 0, "timer can send calls");
    failed += expect_eq_int(fake.timer_stop_calls, 0, "timer stop calls");
    failed += expect_eq_int(fake.finish_shutdown_calls, 0, "finish shutdown calls");

    swbt_daemon_production_hid_session_stop(&session);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += hid_session_register_opens_and_stop_closes_device();
    failed += hid_session_connection_opened_starts_timer_with_hid_cid();
    return failed == 0 ? 0 : 1;
}
