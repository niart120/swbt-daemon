#include "daemon/production_hid_session.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "btstack_bridge/device.h"
#include "btstack_bridge/production_ports.h"

typedef struct {
    int platform_start_calls;
    int hid_register_calls;
    int hid_stop_calls;
    int platform_stop_calls;
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

int main(void) {
    int failed = 0;
    failed += hid_session_register_opens_and_stop_closes_device();
    return failed == 0 ? 0 : 1;
}
