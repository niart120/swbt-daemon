#include "btstack_bridge/hid_device_registration.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    STEP_SDP_INIT = 1,
    STEP_CREATE_HANDLE = 2,
    STEP_CREATE_RECORD = 3,
    STEP_RECORD_LEN = 4,
    STEP_REGISTER_SERVICE = 5,
    STEP_HID_INIT = 6,
    STEP_PACKET_HANDLER = 7,
};

typedef struct {
    int steps[8];
    size_t step_count;
    uint32_t next_handle;
    uint8_t register_status;
    size_t record_len;
    uint32_t captured_handle;
    swbt_btstack_hid_sdp_record_config_t captured_record;
    bool hid_init_called;
    bool packet_handler_called;
    const uint8_t *captured_descriptor;
    uint16_t captured_descriptor_size;
    bool captured_boot_protocol_mode_supported;
} fake_backend_t;

static void fake_record_step(fake_backend_t *backend, int step) {
    if (backend->step_count < (sizeof(backend->steps) / sizeof(backend->steps[0]))) {
        backend->steps[backend->step_count] = step;
    }
    backend->step_count++;
}

static void fake_sdp_init(void *context) {
    fake_record_step((fake_backend_t *)context, STEP_SDP_INIT);
}

static uint32_t fake_sdp_create_service_record_handle(void *context) {
    fake_backend_t *backend = (fake_backend_t *)context;
    fake_record_step(backend, STEP_CREATE_HANDLE);
    return backend->next_handle;
}

static void fake_hid_create_sdp_record(void *context, uint8_t *service,
                                       uint32_t service_record_handle,
                                       const swbt_btstack_hid_sdp_record_config_t *params) {
    fake_backend_t *backend = (fake_backend_t *)context;
    fake_record_step(backend, STEP_CREATE_RECORD);
    backend->captured_handle = service_record_handle;
    backend->captured_record = *params;
    service[0] = 0x35u;
}

static size_t fake_sdp_record_len(void *context, const uint8_t *service) {
    fake_backend_t *backend = (fake_backend_t *)context;
    fake_record_step(backend, STEP_RECORD_LEN);
    return service[0] == 0x35u ? backend->record_len : 0u;
}

static uint8_t fake_sdp_register_service(void *context, const uint8_t *service) {
    fake_backend_t *backend = (fake_backend_t *)context;
    fake_record_step(backend, STEP_REGISTER_SERVICE);
    return service[0] == 0x35u ? backend->register_status : 0xffu;
}

static void fake_hid_device_init(void *context, bool boot_protocol_mode_supported,
                                 uint16_t hid_descriptor_len, const uint8_t *hid_descriptor) {
    fake_backend_t *backend = (fake_backend_t *)context;
    fake_record_step(backend, STEP_HID_INIT);
    backend->hid_init_called = true;
    backend->captured_boot_protocol_mode_supported = boot_protocol_mode_supported;
    backend->captured_descriptor_size = hid_descriptor_len;
    backend->captured_descriptor = hid_descriptor;
}

static void fake_hid_device_register_packet_handler(void *context,
                                                    swbt_btstack_packet_handler_t handler) {
    fake_backend_t *backend = (fake_backend_t *)context;
    fake_record_step(backend, STEP_PACKET_HANDLER);
    backend->packet_handler_called = handler != NULL;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): BTstack packet handler ABI.
static void fake_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet,
                                uint16_t size) {
    (void)packet_type;
    (void)channel;
    (void)packet;
    (void)size;
}

static int expect_true(bool condition, const char *label) {
    if (!condition) {
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

static swbt_btstack_hid_registration_backend_t fake_backend_ops(void) {
    swbt_btstack_hid_registration_backend_t ops = {
        .sdp_init = fake_sdp_init,
        .sdp_create_service_record_handle = fake_sdp_create_service_record_handle,
        .hid_create_sdp_record = fake_hid_create_sdp_record,
        .sdp_record_len = fake_sdp_record_len,
        .sdp_register_service = fake_sdp_register_service,
        .hid_device_init = fake_hid_device_init,
        .hid_device_register_packet_handler = fake_hid_device_register_packet_handler,
    };
    return ops;
}

static swbt_btstack_hid_registration_config_t sample_config(const uint8_t *descriptor) {
    swbt_btstack_hid_registration_config_t config = {
        .hid_device_subclass = 0x1234u,
        .hid_country_code = 0u,
        .hid_virtual_cable = 0u,
        .hid_remote_wake = 1u,
        .hid_reconnect_initiate = 1u,
        .hid_normally_connectable = true,
        .hid_boot_device = false,
        .hid_ssr_host_max_latency = 0x1122u,
        .hid_ssr_host_min_timeout = 0x3344u,
        .hid_supervision_timeout = 0x5566u,
        .hid_descriptor = descriptor,
        .hid_descriptor_size = 4u,
        .device_name = "swbt test controller",
        .packet_handler = fake_packet_handler,
    };
    return config;
}

static int test_registers_hid_device_in_btstack_order(void) {
    uint8_t descriptor[] = {0xdeu, 0xadu, 0xbeu, 0xefu};
    uint8_t service_buffer[300];
    fake_backend_t backend = {
        .next_handle = 0x10001u,
        .register_status = 0u,
        .record_len = 128u,
    };
    swbt_btstack_hid_registration_backend_t ops = fake_backend_ops();
    swbt_btstack_hid_registration_config_t config = sample_config(descriptor);

    swbt_btstack_hid_registration_result_t result = swbt_btstack_hid_device_register(
        &ops, &backend, service_buffer, sizeof(service_buffer), &config);

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_HID_REGISTRATION_OK, "result");
    failed += expect_eq_int((int)backend.step_count, 7, "step count");
    failed += expect_eq_int(backend.steps[0], STEP_SDP_INIT, "step 0");
    failed += expect_eq_int(backend.steps[1], STEP_CREATE_HANDLE, "step 1");
    failed += expect_eq_int(backend.steps[2], STEP_CREATE_RECORD, "step 2");
    failed += expect_eq_int(backend.steps[3], STEP_RECORD_LEN, "step 3");
    failed += expect_eq_int(backend.steps[4], STEP_REGISTER_SERVICE, "step 4");
    failed += expect_eq_int(backend.steps[5], STEP_HID_INIT, "step 5");
    failed += expect_eq_int(backend.steps[6], STEP_PACKET_HANDLER, "step 6");
    failed += expect_true(backend.hid_init_called, "hid init called");
    failed += expect_true(backend.packet_handler_called, "packet handler called");
    failed += expect_eq_int((int)backend.captured_handle, 0x10001, "record handle");
    failed += expect_eq_u16(backend.captured_record.hid_device_subclass, 0x1234u, "hid subclass");
    failed += expect_eq_u16(backend.captured_record.hid_supervision_timeout, 0x5566u,
                            "supervision timeout");
    failed += expect_true(backend.captured_record.hid_descriptor == descriptor,
                          "record descriptor pointer");
    failed += expect_eq_u16(backend.captured_descriptor_size, 4u, "hid descriptor size");
    failed += expect_true(backend.captured_descriptor == descriptor, "hid init descriptor");
    failed +=
        expect_true(!backend.captured_boot_protocol_mode_supported, "boot protocol unsupported");
    return failed;
}

static int test_rejects_invalid_arguments(void) {
    uint8_t descriptor[] = {0xdeu, 0xadu, 0xbeu, 0xefu};
    uint8_t service_buffer[300];
    fake_backend_t backend = {
        .next_handle = 1u,
        .register_status = 0u,
        .record_len = 128u,
    };
    swbt_btstack_hid_registration_backend_t ops = fake_backend_ops();
    swbt_btstack_hid_registration_config_t config = sample_config(descriptor);

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_hid_device_register(NULL, &backend, service_buffer,
                                                             sizeof(service_buffer), &config),
                            SWBT_BTSTACK_HID_REGISTRATION_ERROR_INVALID_ARGUMENT, "null ops");
    failed += expect_eq_int(
        swbt_btstack_hid_device_register(&ops, &backend, NULL, sizeof(service_buffer), &config),
        SWBT_BTSTACK_HID_REGISTRATION_ERROR_INVALID_ARGUMENT, "null service buffer");
    config.hid_descriptor = NULL;
    failed +=
        expect_eq_int(swbt_btstack_hid_device_register(&ops, &backend, service_buffer,
                                                       sizeof(service_buffer), &config),
                      SWBT_BTSTACK_HID_REGISTRATION_ERROR_INVALID_ARGUMENT, "null descriptor");
    return failed;
}

static int test_rejects_oversized_sdp_record(void) {
    uint8_t descriptor[] = {0xdeu, 0xadu, 0xbeu, 0xefu};
    uint8_t service_buffer[16];
    fake_backend_t backend = {
        .next_handle = 1u,
        .register_status = 0u,
        .record_len = 17u,
    };
    swbt_btstack_hid_registration_backend_t ops = fake_backend_ops();
    swbt_btstack_hid_registration_config_t config = sample_config(descriptor);

    return expect_eq_int(swbt_btstack_hid_device_register(&ops, &backend, service_buffer,
                                                          sizeof(service_buffer), &config),
                         SWBT_BTSTACK_HID_REGISTRATION_ERROR_SDP_RECORD_TOO_LARGE,
                         "oversized record");
}

static int test_propagates_sdp_register_failure(void) {
    uint8_t descriptor[] = {0xdeu, 0xadu, 0xbeu, 0xefu};
    uint8_t service_buffer[300];
    fake_backend_t backend = {
        .next_handle = 1u,
        .register_status = 0x0cu,
        .record_len = 128u,
    };
    swbt_btstack_hid_registration_backend_t ops = fake_backend_ops();
    swbt_btstack_hid_registration_config_t config = sample_config(descriptor);

    return expect_eq_int(swbt_btstack_hid_device_register(&ops, &backend, service_buffer,
                                                          sizeof(service_buffer), &config),
                         SWBT_BTSTACK_HID_REGISTRATION_ERROR_SDP_REGISTER_FAILED,
                         "register failure");
}

int main(void) {
    int failed = 0;
    failed += test_registers_hid_device_in_btstack_order();
    failed += test_rejects_invalid_arguments();
    failed += test_rejects_oversized_sdp_record();
    failed += test_propagates_sdp_register_failure();
    return failed == 0 ? 0 : 1;
}
