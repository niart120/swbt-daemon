#include "btstack_bridge/output_report_handler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "switch/switch_subcommand.h"

typedef struct {
    int calls;
    uint16_t hid_cid;
    swbt_switch_output_report_t report;
    uint8_t subcommand_data[4];
    size_t subcommand_data_len;
} capture_t;

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

static int test_recombines_btstack_report_id_with_payload(void) {
    const uint8_t payload_without_report_id[] = {
        0x0Au, 0x10u, 0x11u, 0x12u, 0x13u,
        0x14u, 0x15u, 0x16u, 0x17u, SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE,
        0x30u,
    };
    capture_t capture = {0};
    swbt_btstack_output_report_handler_t handler;
    swbt_btstack_output_report_handler_init(&handler, capture_report, &capture);

    const swbt_btstack_output_report_result_t result = swbt_btstack_output_report_handler_handle(
        &handler, 0x0042u, SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
        SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND, payload_without_report_id,
        sizeof(payload_without_report_id));

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_OUTPUT_REPORT_OK);
    failed += expect_eq_int(capture.calls, 1);
    failed += expect_eq_u16(capture.hid_cid, 0x0042u);
    failed += expect_eq_u8(capture.report.output_report_id,
                           SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND);
    failed += expect_eq_u8(capture.report.packet_counter, 0x0Au);
    failed += expect_true(capture.report.has_subcommand);
    failed += expect_eq_u8(capture.report.subcommand_id, SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE);
    failed += expect_eq_size(capture.subcommand_data_len, 1u);
    failed += expect_eq_u8(capture.subcommand_data[0], 0x30u);
    return failed;
}

static int test_uses_payload_as_full_report_when_report_id_is_zero(void) {
    const uint8_t full_report[] = {
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
    swbt_btstack_output_report_handler_init(&handler, capture_report, &capture);

    const swbt_btstack_output_report_result_t result = swbt_btstack_output_report_handler_handle(
        &handler, 0x0043u, SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT, 0u, full_report,
        sizeof(full_report));

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_OUTPUT_REPORT_OK);
    failed += expect_eq_int(capture.calls, 1);
    failed += expect_eq_u8(capture.report.output_report_id, SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY);
    failed += expect_eq_u8(capture.report.packet_counter, 0x0Bu);
    failed += expect_false(capture.report.has_subcommand);
    return failed;
}

static int test_ignores_non_output_reports(void) {
    const uint8_t report[] = {
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
    swbt_btstack_output_report_handler_init(&handler, capture_report, &capture);

    const swbt_btstack_output_report_result_t result = swbt_btstack_output_report_handler_handle(
        &handler, 0x0044u, SWBT_BTSTACK_HID_REPORT_TYPE_INPUT, 0u, report, sizeof(report));

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_OUTPUT_REPORT_IGNORED_REPORT_TYPE);
    failed += expect_eq_int(capture.calls, 0);
    return failed;
}

static int test_reports_parse_failure_without_dispatch(void) {
    const uint8_t payload[] = {
        0x00u,
    };
    capture_t capture = {0};
    swbt_btstack_output_report_handler_t handler;
    swbt_btstack_output_report_handler_init(&handler, capture_report, &capture);

    const swbt_btstack_output_report_result_t result = swbt_btstack_output_report_handler_handle(
        &handler, 0x0045u, SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
        SWBT_SWITCH_OUTPUT_REPORT_NFC_IR_MCU, payload, sizeof(payload));

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_OUTPUT_REPORT_ERROR_PARSE_FAILED);
    failed += expect_eq_int(capture.calls, 0);
    return failed;
}

static int test_rejects_invalid_arguments_and_oversized_reconstruction(void) {
    uint8_t large_payload[SWBT_BTSTACK_OUTPUT_REPORT_MAX_SIZE];
    const uint8_t payload[] = {
        0x00u,
    };
    capture_t capture = {0};
    swbt_btstack_output_report_handler_t handler;
    swbt_btstack_output_report_handler_init(&handler, capture_report, &capture);

    int failed = 0;
    failed += expect_eq_int(swbt_btstack_output_report_handler_handle(
                                NULL, 0x0046u, SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                                SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY, payload, sizeof(payload)),
                            SWBT_BTSTACK_OUTPUT_REPORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_output_report_handler_handle(
                                &handler, 0x0046u, SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                                SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY, NULL, sizeof(payload)),
                            SWBT_BTSTACK_OUTPUT_REPORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_output_report_handler_handle(
                                &handler, 0x0046u, SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT, 0x0100u,
                                payload, sizeof(payload)),
                            SWBT_BTSTACK_OUTPUT_REPORT_ERROR_REPORT_ID_TOO_LARGE);
    failed += expect_eq_int(swbt_btstack_output_report_handler_handle(
                                &handler, 0x0046u, SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                                SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY, large_payload,
                                sizeof(large_payload)),
                            SWBT_BTSTACK_OUTPUT_REPORT_ERROR_BUFFER_TOO_SMALL);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += test_recombines_btstack_report_id_with_payload();
    failed += test_uses_payload_as_full_report_when_report_id_is_zero();
    failed += test_ignores_non_output_reports();
    failed += test_reports_parse_failure_without_dispatch();
    failed += test_rejects_invalid_arguments_and_oversized_reconstruction();
    return failed == 0 ? 0 : 1;
}
