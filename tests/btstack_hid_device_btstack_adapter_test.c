#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/hid_device_btstack_adapter.h"
#include "btstack_bridge/hid_device_registration.h"
#include "classic/hid_device.h"
#include "classic/sdp_server.h"
#include "classic/sdp_util.h"

typedef struct {
    bool sdp_init_called;
    bool hid_create_sdp_record_called;
    bool sdp_register_service_called;
    bool hid_device_init_called;
    bool hid_device_register_packet_handler_called;
    uint32_t service_record_handle;
    uint32_t record_len;
    uint8_t register_status;
    const uint8_t *service_pointer;
    hid_sdp_record_t sdp_record;
    bool boot_protocol_mode_supported;
    uint16_t hid_descriptor_len;
    const uint8_t *hid_descriptor;
    btstack_packet_handler_t packet_handler;
} fake_btstack_t;

static fake_btstack_t g_fake_btstack;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): must match BTstack callback ABI.
static void fake_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet,
                                uint16_t size) {
    (void)packet_type;
    (void)channel;
    (void)packet;
    (void)size;
}

typedef struct {
    uint32_t record_len;
    uint8_t register_status;
} fake_reset_options_t;

static void fake_reset(fake_reset_options_t options) {
    g_fake_btstack = (fake_btstack_t){0};
    g_fake_btstack.record_len = options.record_len;
    g_fake_btstack.register_status = options.register_status;
}

void sdp_init(void) {
    g_fake_btstack.sdp_init_called = true;
}

uint32_t sdp_create_service_record_handle(void) {
    g_fake_btstack.service_record_handle = 0x01020304u;
    return g_fake_btstack.service_record_handle;
}

void hid_create_sdp_record(uint8_t *service, uint32_t service_record_handle,
                           const hid_sdp_record_t *params) {
    g_fake_btstack.hid_create_sdp_record_called = true;
    g_fake_btstack.service_pointer = service;
    g_fake_btstack.service_record_handle = service_record_handle;
    g_fake_btstack.sdp_record = *params;
}

uint32_t de_get_len(const uint8_t *header) {
    g_fake_btstack.service_pointer = header;
    return g_fake_btstack.record_len;
}

uint8_t sdp_register_service(const uint8_t *record) {
    g_fake_btstack.sdp_register_service_called = true;
    g_fake_btstack.service_pointer = record;
    return g_fake_btstack.register_status;
}

void hid_device_init(bool boot_protocol_mode_supported, uint16_t hid_descriptor_len,
                     const uint8_t *hid_descriptor) {
    g_fake_btstack.hid_device_init_called = true;
    g_fake_btstack.boot_protocol_mode_supported = boot_protocol_mode_supported;
    g_fake_btstack.hid_descriptor_len = hid_descriptor_len;
    g_fake_btstack.hid_descriptor = hid_descriptor;
}

void hid_device_register_packet_handler(btstack_packet_handler_t callback) {
    g_fake_btstack.hid_device_register_packet_handler_called = true;
    g_fake_btstack.packet_handler = callback;
}

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_false(bool value) {
    return value ? 1 : 0;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static swbt_btstack_hid_registration_config_t sample_config(const uint8_t *descriptor) {
    swbt_btstack_hid_registration_config_t config = {
        .hid_device_subclass = 0x2508u,
        .hid_country_code = 0x21u,
        .hid_virtual_cable = 1u,
        .hid_remote_wake = 1u,
        .hid_reconnect_initiate = 1u,
        .hid_normally_connectable = true,
        .hid_boot_device = true,
        .hid_ssr_host_max_latency = 0x1122u,
        .hid_ssr_host_min_timeout = 0x3344u,
        .hid_supervision_timeout = 0x5566u,
        .hid_descriptor = descriptor,
        .hid_descriptor_size = 3u,
        .device_name = "Pro Controller",
        .packet_handler = fake_packet_handler,
    };
    return config;
}

static int test_backend_forwards_registration_calls_to_btstack(void) {
    uint8_t service_buffer[16] = {0};
    uint8_t descriptor[] = {0xdeu, 0xadu, 0xbeu};
    swbt_btstack_hid_registration_config_t config = sample_config(descriptor);
    const swbt_btstack_hid_registration_backend_t *backend =
        swbt_btstack_hid_registration_backend_btstack();

    fake_reset((fake_reset_options_t){
        .record_len = 4u,
        .register_status = 0u,
    });

    int failed = 0;
    failed += expect_true(backend != NULL);
    failed += expect_eq_int(swbt_btstack_hid_device_register(backend, NULL, service_buffer,
                                                             sizeof(service_buffer), &config),
                            SWBT_BTSTACK_HID_REGISTRATION_OK);
    failed += expect_true(g_fake_btstack.sdp_init_called);
    failed += expect_true(g_fake_btstack.hid_create_sdp_record_called);
    failed += expect_true(g_fake_btstack.sdp_register_service_called);
    failed += expect_true(g_fake_btstack.hid_device_init_called);
    failed += expect_true(g_fake_btstack.hid_device_register_packet_handler_called);
    failed += expect_eq_u32(g_fake_btstack.service_record_handle, 0x01020304u);
    failed += expect_true(g_fake_btstack.service_pointer == service_buffer);
    failed += expect_eq_u16(g_fake_btstack.sdp_record.hid_device_subclass, 0x2508u);
    failed += expect_eq_u8(g_fake_btstack.sdp_record.hid_country_code, 0x21u);
    failed += expect_true(g_fake_btstack.sdp_record.hid_normally_connectable);
    failed += expect_true(g_fake_btstack.sdp_record.hid_descriptor == descriptor);
    failed += expect_eq_u16(g_fake_btstack.sdp_record.hid_descriptor_size, 3u);
    failed += expect_true(g_fake_btstack.sdp_record.device_name == config.device_name);
    failed += expect_true(g_fake_btstack.boot_protocol_mode_supported);
    failed += expect_eq_u16(g_fake_btstack.hid_descriptor_len, 3u);
    failed += expect_true(g_fake_btstack.hid_descriptor == descriptor);
    failed += expect_true(g_fake_btstack.packet_handler == fake_packet_handler);
    return failed;
}

static int test_invalid_config_is_rejected_before_btstack_calls(void) {
    uint8_t service_buffer[16] = {0};
    swbt_btstack_hid_registration_config_t config = sample_config(NULL);
    const swbt_btstack_hid_registration_backend_t *backend =
        swbt_btstack_hid_registration_backend_btstack();

    fake_reset((fake_reset_options_t){
        .record_len = 4u,
        .register_status = 0u,
    });

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_hid_device_register(backend, NULL, service_buffer,
                                                             sizeof(service_buffer), &config),
                            SWBT_BTSTACK_HID_REGISTRATION_ERROR_INVALID_ARGUMENT);
    failed += expect_false(g_fake_btstack.sdp_init_called);
    failed += expect_false(g_fake_btstack.hid_create_sdp_record_called);
    return failed;
}

static int test_oversized_sdp_record_stops_before_registration(void) {
    uint8_t service_buffer[4] = {0};
    uint8_t descriptor[] = {0xdeu, 0xadu, 0xbeu};
    swbt_btstack_hid_registration_config_t config = sample_config(descriptor);
    const swbt_btstack_hid_registration_backend_t *backend =
        swbt_btstack_hid_registration_backend_btstack();

    fake_reset((fake_reset_options_t){
        .record_len = sizeof(service_buffer) + 1u,
        .register_status = 0u,
    });

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_hid_device_register(backend, NULL, service_buffer,
                                                             sizeof(service_buffer), &config),
                            SWBT_BTSTACK_HID_REGISTRATION_ERROR_SDP_RECORD_TOO_LARGE);
    failed += expect_true(g_fake_btstack.sdp_init_called);
    failed += expect_true(g_fake_btstack.hid_create_sdp_record_called);
    failed += expect_false(g_fake_btstack.sdp_register_service_called);
    failed += expect_false(g_fake_btstack.hid_device_init_called);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += test_backend_forwards_registration_calls_to_btstack();
    failed += test_invalid_config_is_rejected_before_btstack_calls();
    failed += test_oversized_sdp_record_stops_before_registration();
    return failed == 0 ? 0 : 1;
}
