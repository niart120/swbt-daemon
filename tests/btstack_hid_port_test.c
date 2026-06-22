#include "btstack_bridge/hid_port.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int request_can_send_calls;
    int send_interrupt_calls;
    uint16_t hid_cid;
    uint8_t report[8];
    uint16_t report_size;
} fake_btstack_t;

static fake_btstack_t g_fake_btstack;

void hid_device_request_can_send_now_event(uint16_t hid_cid) {
    g_fake_btstack.hid_cid = hid_cid;
    g_fake_btstack.request_can_send_calls += 1;
}

void hid_device_send_interrupt_message(uint16_t hid_cid, const uint8_t *message,
                                       uint16_t message_len) {
    g_fake_btstack.hid_cid = hid_cid;
    g_fake_btstack.report_size = message_len;
    g_fake_btstack.send_interrupt_calls += 1;
    for (uint16_t index = 0; index < message_len && index < sizeof(g_fake_btstack.report);
         ++index) {
        g_fake_btstack.report[index] = message[index];
    }
}

static void fake_reset(void) {
    g_fake_btstack = (fake_btstack_t){0};
}

static int expect_true(bool value) {
    return value ? 0 : 1;
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

static int send_report_forwards_unchanged_bytes(void) {
    const uint8_t report[] = {0xa1u, 0x30u, 0x41u, 0x8eu, 0x00u};
    const swbt_btstack_hid_port_t *port = swbt_btstack_hid_port_btstack();

    fake_reset();
    const swbt_btstack_hid_port_result_t result =
        swbt_btstack_hid_port_send_report(port, 0x0042u, report, sizeof(report));

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_HID_PORT_OK);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 1);
    failed += expect_eq_u16(g_fake_btstack.hid_cid, 0x0042u);
    failed += expect_eq_u16(g_fake_btstack.report_size, sizeof(report));
    for (size_t index = 0; index < sizeof(report); ++index) {
        failed += expect_eq_u8(g_fake_btstack.report[index], report[index]);
    }
    return failed;
}

static int request_can_send_forwards_hid_cid(void) {
    const swbt_btstack_hid_port_t *port = swbt_btstack_hid_port_btstack();

    fake_reset();
    const swbt_btstack_hid_port_result_t result =
        swbt_btstack_hid_port_request_can_send_now(port, 0x0043u);

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_HID_PORT_OK);
    failed += expect_eq_int(g_fake_btstack.request_can_send_calls, 1);
    failed += expect_eq_u16(g_fake_btstack.hid_cid, 0x0043u);
    return failed;
}

static int invalid_arguments_are_rejected_before_btstack_calls(void) {
    const uint8_t report[] = {0xa1u};
    fake_reset();
    int failed = 0;
    failed += expect_true(swbt_btstack_hid_port_btstack() != NULL);
    failed +=
        expect_eq_int(swbt_btstack_hid_port_send_report(NULL, 0x0042u, report, sizeof(report)),
                      SWBT_BTSTACK_HID_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_hid_port_send_report(swbt_btstack_hid_port_btstack(),
                                                              0x0042u, NULL, sizeof(report)),
                            SWBT_BTSTACK_HID_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(
        swbt_btstack_hid_port_send_report(swbt_btstack_hid_port_btstack(), 0x0042u, report, 0u),
        SWBT_BTSTACK_HID_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_hid_port_send_report(swbt_btstack_hid_port_btstack(),
                                                              0x0042u, report, 0x10000u),
                            SWBT_BTSTACK_HID_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_hid_port_request_can_send_now(NULL, 0x0042u),
                            SWBT_BTSTACK_HID_PORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(g_fake_btstack.send_interrupt_calls, 0);
    failed += expect_eq_int(g_fake_btstack.request_can_send_calls, 0);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += send_report_forwards_unchanged_bytes();
    failed += request_can_send_forwards_hid_cid();
    failed += invalid_arguments_are_rejected_before_btstack_calls();
    return failed == 0 ? 0 : 1;
}
