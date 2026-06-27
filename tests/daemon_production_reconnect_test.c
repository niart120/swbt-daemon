#include "daemon/production_reconnect.h"

#include <stddef.h>

static int expect_eq_int(int actual, int expected, const char *label) {
    (void)label;
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected, const char *label) {
    (void)label;
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected, const char *label) {
    (void)label;
    return actual == expected ? 0 : 1;
}

static int effective_switch_address_builds_btstack_request(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    swbt_btstack_device_connect_request_t request;
    const uint8_t expected_address[] = {0x01u, 0x23u, 0x45u, 0x67u, 0x89u, 0xabu};
    int failed = 0;

    failed += expect_eq_int(swbt_daemon_config_set_active_reconnect_learned_switch_address(
                                &config, "01:23:45:67:89:ab"),
                            true, "set learned address");
    failed += expect_eq_int(swbt_daemon_production_reconnect_build_request(&config, &request),
                            SWBT_DAEMON_PRODUCTION_RECONNECT_REQUEST_READY, "build request");
    failed +=
        expect_eq_u16(request.control_psm, SWBT_BTSTACK_PRODUCTION_HID_CONTROL_PSM, "control psm");
    failed += expect_eq_u16(request.interrupt_psm, SWBT_BTSTACK_PRODUCTION_HID_INTERRUPT_PSM,
                            "interrupt psm");
    for (size_t index = 0u; index < sizeof(expected_address); ++index) {
        failed += expect_eq_u8(request.address[index], expected_address[index], "address byte");
    }
    return failed;
}

int main(void) {
    int failed = 0;
    failed += effective_switch_address_builds_btstack_request();
    return failed == 0 ? 0 : 1;
}
