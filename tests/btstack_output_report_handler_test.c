#include "btstack_bridge/output_report_handler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "application/app.h"
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

static int expect_eq_u64(uint64_t actual, uint64_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_payload(const uint8_t *actual, const uint8_t *expected) {
    for (size_t index = 0; index < SWBT_SWITCH_RUMBLE_DATA_SIZE; ++index) {
        if (actual[index] != expected[index]) {
            return 1;
        }
    }
    return 0;
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
        &handler, (swbt_btstack_output_report_handle_options_t){
                      .hid_cid = 0x0042u,
                      .report_type = SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                      .report_id = SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND,
                      .report = payload_without_report_id,
                      .report_size = sizeof(payload_without_report_id),
                  });

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
        &handler, (swbt_btstack_output_report_handle_options_t){
                      .hid_cid = 0x0043u,
                      .report_type = SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                      .report_id = 0u,
                      .report = full_report,
                      .report_size = sizeof(full_report),
                  });

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
        &handler, (swbt_btstack_output_report_handle_options_t){
                      .hid_cid = 0x0044u,
                      .report_type = SWBT_BTSTACK_HID_REPORT_TYPE_INPUT,
                      .report_id = 0u,
                      .report = report,
                      .report_size = sizeof(report),
                  });

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
        &handler, (swbt_btstack_output_report_handle_options_t){
                      .hid_cid = 0x0045u,
                      .report_type = SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                      .report_id = SWBT_SWITCH_OUTPUT_REPORT_NFC_IR_MCU,
                      .report = payload,
                      .report_size = sizeof(payload),
                  });

    int failed = 0;
    failed += expect_eq_int(result, SWBT_BTSTACK_OUTPUT_REPORT_ERROR_PARSE_FAILED);
    failed += expect_eq_int(capture.calls, 0);
    return failed;
}

typedef struct {
    swbt_app_t *app;
    uint64_t now_ms;
    int calls;
} rumble_status_capture_t;

static void record_rumble_status(void *context, uint16_t hid_cid,
                                 const swbt_switch_output_report_t *report) {
    rumble_status_capture_t *capture = context;
    (void)hid_cid;
    ++capture->calls;
    (void)swbt_app_record_rumble(capture->app, report->rumble, capture->now_ms);
}

static int test_parsed_reports_can_feed_rumble_status(void) {
    const uint8_t active_rumble[SWBT_SWITCH_RUMBLE_DATA_SIZE] = {
        0x04, 0x01, 0x80, 0x41, 0x08, 0x01, 0x80, 0x42,
    };
    const uint8_t rumble_only_report[] = {
        SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY,
        0x0Bu,
        0x04u,
        0x01u,
        0x80u,
        0x41u,
        0x08u,
        0x01u,
        0x80u,
        0x42u,
    };
    const uint8_t rumble_and_subcommand_report[] = {
        SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND,
        0x0Cu,
        0x00u,
        0x01u,
        0x40u,
        0x40u,
        0x00u,
        0x01u,
        0x40u,
        0x40u,
        SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE,
    };
    swbt_app_t *app = swbt_app_create();
    swbt_app_snapshot_t snapshot;
    rumble_status_capture_t capture = {
        .app = app,
        .now_ms = 100u,
    };
    swbt_btstack_output_report_handler_t handler;

    int failed = 0;
    failed += expect_true(app != NULL);
    swbt_btstack_output_report_handler_init(&handler, record_rumble_status, &capture);
    failed += expect_eq_int(swbt_btstack_output_report_handler_handle(
                                &handler,
                                (swbt_btstack_output_report_handle_options_t){
                                    .hid_cid = 0x0047u,
                                    .report_type = SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                                    .report_id = 0u,
                                    .report = rumble_only_report,
                                    .report_size = sizeof(rumble_only_report),
                                }),
                            SWBT_BTSTACK_OUTPUT_REPORT_OK);
    failed += expect_eq_int(swbt_app_snapshot(app, &snapshot), SWBT_APP_OK);
    failed += expect_eq_int(capture.calls, 1);
    failed += expect_true(snapshot.rumble.updated);
    failed += expect_eq_u64(snapshot.rumble.updated_at_ms, 100u);
    failed += expect_payload(snapshot.rumble.raw, active_rumble);

    capture.now_ms = 200u;
    failed += expect_eq_int(swbt_btstack_output_report_handler_handle(
                                &handler,
                                (swbt_btstack_output_report_handle_options_t){
                                    .hid_cid = 0x0047u,
                                    .report_type = SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                                    .report_id = 0u,
                                    .report = rumble_and_subcommand_report,
                                    .report_size = sizeof(rumble_and_subcommand_report),
                                }),
                            SWBT_BTSTACK_OUTPUT_REPORT_OK);
    failed += expect_eq_int(swbt_app_snapshot(app, &snapshot), SWBT_APP_OK);
    failed += expect_eq_int(capture.calls, 2);
    failed += expect_eq_u64(snapshot.rumble.updated_at_ms, 200u);
    failed += expect_payload(snapshot.rumble.raw, SWBT_SWITCH_RUMBLE_NEUTRAL_PAYLOAD);
    swbt_app_destroy(app);
    return failed;
}

static int test_invalid_output_report_does_not_change_rumble_status(void) {
    const uint8_t payload[] = {
        0x00u,
    };
    swbt_app_t *app = swbt_app_create();
    swbt_app_snapshot_t snapshot;
    rumble_status_capture_t capture = {
        .app = app,
        .now_ms = 300u,
    };
    swbt_btstack_output_report_handler_t handler;

    int failed = 0;
    failed += expect_true(app != NULL);
    swbt_btstack_output_report_handler_init(&handler, record_rumble_status, &capture);
    failed += expect_eq_int(swbt_btstack_output_report_handler_handle(
                                &handler,
                                (swbt_btstack_output_report_handle_options_t){
                                    .hid_cid = 0x0048u,
                                    .report_type = SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                                    .report_id = SWBT_SWITCH_OUTPUT_REPORT_NFC_IR_MCU,
                                    .report = payload,
                                    .report_size = sizeof(payload),
                                }),
                            SWBT_BTSTACK_OUTPUT_REPORT_ERROR_PARSE_FAILED);
    failed += expect_eq_int(swbt_app_snapshot(app, &snapshot), SWBT_APP_OK);
    failed += expect_eq_int(capture.calls, 0);
    failed += expect_false(snapshot.rumble.updated);
    failed += expect_payload(snapshot.rumble.raw, SWBT_SWITCH_RUMBLE_NEUTRAL_PAYLOAD);
    swbt_app_destroy(app);
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
                                NULL,
                                (swbt_btstack_output_report_handle_options_t){
                                    .hid_cid = 0x0046u,
                                    .report_type = SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                                    .report_id = SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY,
                                    .report = payload,
                                    .report_size = sizeof(payload),
                                }),
                            SWBT_BTSTACK_OUTPUT_REPORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_output_report_handler_handle(
                                &handler,
                                (swbt_btstack_output_report_handle_options_t){
                                    .hid_cid = 0x0046u,
                                    .report_type = SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                                    .report_id = SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY,
                                    .report = NULL,
                                    .report_size = sizeof(payload),
                                }),
                            SWBT_BTSTACK_OUTPUT_REPORT_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(swbt_btstack_output_report_handler_handle(
                                &handler,
                                (swbt_btstack_output_report_handle_options_t){
                                    .hid_cid = 0x0046u,
                                    .report_type = SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                                    .report_id = 0x0100u,
                                    .report = payload,
                                    .report_size = sizeof(payload),
                                }),
                            SWBT_BTSTACK_OUTPUT_REPORT_ERROR_REPORT_ID_TOO_LARGE);
    failed += expect_eq_int(swbt_btstack_output_report_handler_handle(
                                &handler,
                                (swbt_btstack_output_report_handle_options_t){
                                    .hid_cid = 0x0046u,
                                    .report_type = SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT,
                                    .report_id = SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY,
                                    .report = large_payload,
                                    .report_size = sizeof(large_payload),
                                }),
                            SWBT_BTSTACK_OUTPUT_REPORT_ERROR_BUFFER_TOO_SMALL);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += test_recombines_btstack_report_id_with_payload();
    failed += test_uses_payload_as_full_report_when_report_id_is_zero();
    failed += test_ignores_non_output_reports();
    failed += test_reports_parse_failure_without_dispatch();
    failed += test_parsed_reports_can_feed_rumble_status();
    failed += test_invalid_output_report_does_not_change_rumble_status();
    failed += test_rejects_invalid_arguments_and_oversized_reconstruction();
    return failed == 0 ? 0 : 1;
}
