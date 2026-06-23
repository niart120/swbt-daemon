#include "daemon/production_backend.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "classic/hid_device.h"
#include "classic/sdp_util.h"
#include "switch/switch_hid_descriptor.h"

static hid_sdp_record_t
btstack_record_from_config(const swbt_btstack_hid_registration_config_t *config) {
    return (hid_sdp_record_t){
        .hid_device_subclass = config->hid_device_subclass,
        .hid_country_code = config->hid_country_code,
        .hid_virtual_cable = config->hid_virtual_cable,
        .hid_remote_wake = config->hid_remote_wake,
        .hid_reconnect_initiate = config->hid_reconnect_initiate,
        .hid_normally_connectable = config->hid_normally_connectable,
        .hid_boot_device = config->hid_boot_device,
        .hid_ssr_host_max_latency = config->hid_ssr_host_max_latency,
        .hid_ssr_host_min_timeout = config->hid_ssr_host_min_timeout,
        .hid_supervision_timeout = config->hid_supervision_timeout,
        .hid_descriptor = config->hid_descriptor,
        .hid_descriptor_size = config->hid_descriptor_size,
        .device_name = config->device_name,
    };
}

static int expect_true(bool value, const char *label) {
    if (!value) {
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "expected true: %s\n", label);
        return 1;
    }
    return 0;
}

static int test_production_hid_service_buffer_fits_btstack_sdp_record(void) {
    uint8_t scratch[1024];
    const swbt_btstack_hid_registration_config_t config =
        swbt_btstack_production_hid_registration_config();
    const hid_sdp_record_t btstack_record = btstack_record_from_config(&config);
    const uint32_t service_record_handle = 0x10001u;
    uint32_t record_len = 0;

    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memset(scratch, 0, sizeof(scratch));
    hid_create_sdp_record(scratch, service_record_handle, &btstack_record);
    record_len = de_get_len(scratch);

    if (record_len > SWBT_DAEMON_PRODUCTION_HID_SERVICE_BUFFER_SIZE) {
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "production HID service buffer too small: required=%u capacity=%u\n",
                (unsigned)record_len, (unsigned)SWBT_DAEMON_PRODUCTION_HID_SERVICE_BUFFER_SIZE);
        return 1;
    }

    return expect_true(record_len > config.hid_descriptor_size,
                       "record includes HID descriptor and SDP attributes");
}

static int test_production_registration_config_references_switch_descriptor(void) {
    const swbt_btstack_hid_registration_config_t config =
        swbt_btstack_production_hid_registration_config();
    const size_t descriptor_size = swbt_switch_hid_descriptor_size();

    int failed = 0;
    failed += expect_true(descriptor_size <= UINT16_MAX, "descriptor fits uint16");
    failed += expect_true(config.hid_descriptor == swbt_switch_hid_descriptor_data(),
                          "registration descriptor pointer");
    failed +=
        expect_true(config.hid_descriptor_size == descriptor_size, "registration descriptor size");
    return failed;
}

int main(void) {
    int failed = 0;
    failed += test_production_hid_service_buffer_fits_btstack_sdp_record();
    failed += test_production_registration_config_references_switch_descriptor();
    return failed == 0 ? 0 : 1;
}
