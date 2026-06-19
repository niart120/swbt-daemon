#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/output_report_callbacks.h"
#include "btstack_bridge/output_report_handler.h"
#include "classic/hid_device.h"
#include "switch/switch_subcommand.h"

typedef void (*fake_report_data_callback_t)(uint16_t hid_cid, hid_report_type_t report_type,
                                            uint16_t report_id, int report_size, uint8_t *report);
typedef void (*fake_set_report_callback_t)(uint16_t hid_cid, hid_report_type_t report_type,
                                           int report_size, uint8_t *report);

typedef struct {
    fake_report_data_callback_t report_data_callback;
    fake_set_report_callback_t set_report_callback;
} fake_btstack_t;

typedef struct {
    int calls;
    uint16_t hid_cid;
    swbt_switch_output_report_t report;
    uint8_t subcommand_data[4];
    size_t subcommand_data_len;
} capture_t;

static fake_btstack_t g_fake_btstack;

void hid_device_register_report_data_callback(fake_report_data_callback_t callback) {
    g_fake_btstack.report_data_callback = callback;
}

void hid_device_register_set_report_callback(fake_set_report_callback_t callback) {
    g_fake_btstack.set_report_callback = callback;
}

static void fake_reset(void) {
    g_fake_btstack = (fake_btstack_t){0};
}

static void capture_report(void *context, uint16_t hid_cid,
                           const swbt_switch_output_report_t *report) {
    capture_t *capture = (capture_t *)context;
    capture->calls++;
    capture->hid_cid = hid_cid;
    capture->report = *report;
    capture->subcommand_data_len = report->subcommand_data_len;
    for (size_t index = 0;
         index < report->subcommand_data_len &&
         index < (sizeof(capture->subcommand_data) / sizeof(capture->subcommand_data[0]));
         ++index) {
        capture->subcommand_data[index] = report->subcommand_data[index];
    }
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

static int expect_eq_size(size_t actual, size_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_false(bool value) {
    return value ? 1 : 0;
}

static void init_handler(swbt_btstack_output_report_handler_t *handler, capture_t *capture) {
    swbt_btstack_output_report_handler_init(handler, capture_report, capture);
}

static int test_registers_btstack_callbacks(void) {
    capture_t capture = {0};
    swbt_btstack_output_report_handler_t handler;
    init_handler(&handler, &capture);
    fake_reset();

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_output_report_callbacks_register(&handler),
                            SWBT_BTSTACK_OUTPUT_REPORT_CALLBACKS_OK);
    failed += expect_true(g_fake_btstack.report_data_callback != NULL);
    failed += expect_true(g_fake_btstack.set_report_callback != NULL);

    swbt_btstack_output_report_callbacks_unregister();
    failed += expect_true(g_fake_btstack.report_data_callback == NULL);
    failed += expect_true(g_fake_btstack.set_report_callback == NULL);
    return failed;
}

static int test_data_callback_dispatches_output_reports(void) {
    const uint8_t payload_without_report_id[] = {
        0x0Au, 0x10u, 0x11u, 0x12u, 0x13u,
        0x14u, 0x15u, 0x16u, 0x17u, SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE,
        0x30u,
    };
    capture_t capture = {0};
    swbt_btstack_output_report_handler_t handler;
    init_handler(&handler, &capture);
    fake_reset();

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_output_report_callbacks_register(&handler),
                            SWBT_BTSTACK_OUTPUT_REPORT_CALLBACKS_OK);
    g_fake_btstack.report_data_callback(
        0x0047u, HID_REPORT_TYPE_OUTPUT, SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND,
        (int)sizeof(payload_without_report_id), (uint8_t *)payload_without_report_id);

    failed += expect_eq_int(capture.calls, 1);
    failed += expect_eq_u16(capture.hid_cid, 0x0047u);
    failed += expect_eq_u8(capture.report.output_report_id,
                           SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND);
    failed += expect_true(capture.report.has_subcommand);
    failed += expect_eq_u8(capture.report.subcommand_id, SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE);
    failed += expect_eq_size(capture.subcommand_data_len, 1u);
    failed += expect_eq_u8(capture.subcommand_data[0], 0x30u);
    swbt_btstack_output_report_callbacks_unregister();
    return failed;
}

static int test_set_report_callback_dispatches_full_report_payload(void) {
    uint8_t full_report[] = {
        SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY,
        0x0Bu,
        0x20u,
        0x21u,
        0x22u,
        0x23u,
        0x24u,
        0x25u,
        0x26u,
        0x27u,
    };
    capture_t capture = {0};
    swbt_btstack_output_report_handler_t handler;
    init_handler(&handler, &capture);
    fake_reset();

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_output_report_callbacks_register(&handler),
                            SWBT_BTSTACK_OUTPUT_REPORT_CALLBACKS_OK);
    g_fake_btstack.set_report_callback(0x0048u, HID_REPORT_TYPE_OUTPUT, (int)sizeof(full_report),
                                       full_report);

    failed += expect_eq_int(capture.calls, 1);
    failed += expect_eq_u16(capture.hid_cid, 0x0048u);
    failed += expect_eq_u8(capture.report.output_report_id, SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY);
    failed += expect_false(capture.report.has_subcommand);
    swbt_btstack_output_report_callbacks_unregister();
    return failed;
}

static int test_non_output_and_parse_failure_do_not_dispatch(void) {
    const uint8_t invalid_payload[] = {0x00u};
    capture_t capture = {0};
    swbt_btstack_output_report_handler_t handler;
    init_handler(&handler, &capture);
    fake_reset();

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_output_report_callbacks_register(&handler),
                            SWBT_BTSTACK_OUTPUT_REPORT_CALLBACKS_OK);
    g_fake_btstack.report_data_callback(0x0049u, HID_REPORT_TYPE_INPUT, 0u,
                                        (int)sizeof(invalid_payload), (uint8_t *)invalid_payload);
    g_fake_btstack.report_data_callback(0x0049u, HID_REPORT_TYPE_OUTPUT,
                                        SWBT_SWITCH_OUTPUT_REPORT_NFC_IR_MCU,
                                        (int)sizeof(invalid_payload), (uint8_t *)invalid_payload);

    failed += expect_eq_int(capture.calls, 0);
    swbt_btstack_output_report_callbacks_unregister();
    return failed;
}

static int test_rejects_missing_handler_without_registering_callbacks(void) {
    fake_reset();

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_output_report_callbacks_register(NULL),
                            SWBT_BTSTACK_OUTPUT_REPORT_CALLBACKS_ERROR_INVALID_ARGUMENT);
    failed += expect_true(g_fake_btstack.report_data_callback == NULL);
    failed += expect_true(g_fake_btstack.set_report_callback == NULL);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += test_registers_btstack_callbacks();
    failed += test_data_callback_dispatches_output_reports();
    failed += test_set_report_callback_dispatches_full_report_payload();
    failed += test_non_output_and_parse_failure_do_not_dispatch();
    failed += test_rejects_missing_handler_without_registering_callbacks();
    return failed == 0 ? 0 : 1;
}
