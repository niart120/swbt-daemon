#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/input_report_scheduler.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_report.h"

typedef struct {
    int calls;
    int result;
    uint16_t hid_cid;
    uint8_t report[SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE];
    size_t report_size;
} send_capture_t;

static int expect_true(int actual) {
    return actual ? 0 : 1;
}

static int expect_false(int actual) {
    return actual ? 1 : 0;
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

static int expect_eq_u64(uint64_t actual, uint64_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_size(size_t actual, size_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_stick_bytes(const uint8_t *actual, uint8_t b0, uint8_t b1, uint8_t b2) {
    if (actual[0] != b0 || actual[1] != b1 || actual[2] != b2) {
        return 1;
    }
    return 0;
}

static int capture_send(void *context, uint16_t hid_cid, const uint8_t *report,
                        size_t report_size) {
    send_capture_t *capture = (send_capture_t *)context;
    capture->calls += 1;
    capture->hid_cid = hid_cid;
    capture->report_size = report_size;
    for (size_t index = 0; index < report_size && index < sizeof(capture->report); ++index) {
        capture->report[index] = report[index];
    }
    return capture->result;
}

static swbt_state_t sample_state(void) {
    swbt_state_t state = swbt_state_neutral();
    state.buttons = 0x00422408u;
    state.lx = 0x0123u;
    state.ly = 0x0ABCu;
    state.rx = 0x0FFFu;
    state.ry = 0x000Fu;
    return state;
}

static swbt_btstack_input_report_scheduler_config_t sample_config(uint32_t period_us) {
    swbt_btstack_input_report_scheduler_config_t config = {
        .report_period_us = period_us,
        .report_options =
            {
                .timer = 0x41u,
                .battery_connection = 0x8Eu,
                .vibrator_report = 0x80u,
            },
    };
    return config;
}

static int test_sends_standard_full_report_when_due(void) {
    send_capture_t capture = {0};
    swbt_btstack_input_report_scheduler_t scheduler;
    const swbt_btstack_input_report_scheduler_config_t config =
        sample_config(SWBT_BTSTACK_INPUT_REPORT_DEFAULT_PERIOD_US);
    const swbt_state_t state = sample_state();

    int failed = 0;
    failed += expect_eq_int(
        swbt_btstack_input_report_scheduler_init(&scheduler, capture_send, &capture, &config),
        SWBT_BTSTACK_INPUT_REPORT_OK);
    failed += expect_eq_int(swbt_btstack_input_report_scheduler_start(
                                &scheduler,
                                (swbt_btstack_input_report_scheduler_start_options_t){
                                    .hid_cid = 0x0042u,
                                    .now_us = 1000u,
                                }),
                            SWBT_BTSTACK_INPUT_REPORT_OK);
    failed += expect_true(swbt_btstack_input_report_scheduler_is_running(&scheduler));
    failed +=
        expect_eq_u64(swbt_btstack_input_report_scheduler_next_deadline_us(&scheduler), 9000u);
    failed += expect_eq_int(swbt_btstack_input_report_scheduler_tick(&scheduler, 8999u, &state),
                            SWBT_BTSTACK_INPUT_REPORT_NOT_DUE);
    failed += expect_eq_int(capture.calls, 0);

    failed += expect_eq_int(swbt_btstack_input_report_scheduler_tick(&scheduler, 9000u, &state),
                            SWBT_BTSTACK_INPUT_REPORT_OK);
    failed += expect_eq_int(capture.calls, 1);
    failed += expect_eq_u16(capture.hid_cid, 0x0042u);
    failed += expect_eq_size(capture.report_size, SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE);
    failed += expect_eq_u8(capture.report[0], SWBT_SWITCH_INPUT_REPORT_STANDARD_FULL);
    failed += expect_eq_u8(capture.report[1], 0x41u);
    failed += expect_eq_u8(capture.report[2], 0x8Eu);
    failed += expect_eq_u8(capture.report[3], 0x08u);
    failed += expect_eq_u8(capture.report[4], 0x24u);
    failed += expect_eq_u8(capture.report[5], 0x42u);
    failed += expect_stick_bytes(&capture.report[6], 0x23u, 0xC1u, 0xABu);
    failed += expect_stick_bytes(&capture.report[9], 0xFFu, 0xFFu, 0x00u);
    failed += expect_eq_u8(capture.report[12], 0x80u);
    failed +=
        expect_eq_u64(swbt_btstack_input_report_scheduler_next_deadline_us(&scheduler), 17000u);

    failed += expect_eq_int(swbt_btstack_input_report_scheduler_tick(&scheduler, 17000u, &state),
                            SWBT_BTSTACK_INPUT_REPORT_OK);
    failed += expect_eq_int(capture.calls, 2);
    failed += expect_eq_u8(capture.report[1], 0x42u);
    return failed;
}

static int test_late_tick_drops_catchup_reports(void) {
    send_capture_t capture = {0};
    swbt_btstack_input_report_scheduler_t scheduler;
    const swbt_btstack_input_report_scheduler_config_t config = sample_config(8000u);
    const swbt_state_t state = sample_state();

    int failed = 0;
    failed += expect_eq_int(
        swbt_btstack_input_report_scheduler_init(&scheduler, capture_send, &capture, &config),
        SWBT_BTSTACK_INPUT_REPORT_OK);
    failed += expect_eq_int(swbt_btstack_input_report_scheduler_start(
                                &scheduler,
                                (swbt_btstack_input_report_scheduler_start_options_t){
                                    .hid_cid = 0x0042u,
                                    .now_us = 0u,
                                }),
                            SWBT_BTSTACK_INPUT_REPORT_OK);
    failed += expect_eq_int(swbt_btstack_input_report_scheduler_tick(&scheduler, 25000u, &state),
                            SWBT_BTSTACK_INPUT_REPORT_OK);
    failed += expect_eq_int(capture.calls, 1);
    failed +=
        expect_eq_u64(swbt_btstack_input_report_scheduler_next_deadline_us(&scheduler), 33000u);
    failed += expect_eq_int(swbt_btstack_input_report_scheduler_tick(&scheduler, 25000u, &state),
                            SWBT_BTSTACK_INPUT_REPORT_NOT_DUE);
    failed += expect_eq_int(capture.calls, 1);
    return failed;
}

static int test_stop_invalid_arguments_and_send_failure(void) {
    send_capture_t capture = {0};
    swbt_btstack_input_report_scheduler_t scheduler;
    swbt_btstack_input_report_scheduler_config_t config = sample_config(8000u);
    const swbt_state_t state = sample_state();

    int failed = 0;
    failed += expect_eq_int(
        swbt_btstack_input_report_scheduler_init(NULL, capture_send, &capture, &config),
        SWBT_BTSTACK_INPUT_REPORT_ERROR_INVALID_ARGUMENT);
    failed +=
        expect_eq_int(swbt_btstack_input_report_scheduler_init(&scheduler, NULL, &capture, &config),
                      SWBT_BTSTACK_INPUT_REPORT_ERROR_INVALID_ARGUMENT);
    config.report_period_us = 0u;
    failed += expect_eq_int(
        swbt_btstack_input_report_scheduler_init(&scheduler, capture_send, &capture, &config),
        SWBT_BTSTACK_INPUT_REPORT_ERROR_INVALID_ARGUMENT);

    config = sample_config(8000u);
    failed += expect_eq_int(
        swbt_btstack_input_report_scheduler_init(&scheduler, capture_send, &capture, &config),
        SWBT_BTSTACK_INPUT_REPORT_OK);
    failed += expect_eq_int(swbt_btstack_input_report_scheduler_tick(&scheduler, 8000u, &state),
                            SWBT_BTSTACK_INPUT_REPORT_STOPPED);
    failed += expect_eq_int(swbt_btstack_input_report_scheduler_start(
                                &scheduler,
                                (swbt_btstack_input_report_scheduler_start_options_t){
                                    .hid_cid = 0x0042u,
                                    .now_us = 0u,
                                }),
                            SWBT_BTSTACK_INPUT_REPORT_OK);
    failed += expect_eq_int(swbt_btstack_input_report_scheduler_tick(&scheduler, 8000u, NULL),
                            SWBT_BTSTACK_INPUT_REPORT_ERROR_INVALID_ARGUMENT);

    capture.result = -7;
    failed += expect_eq_int(swbt_btstack_input_report_scheduler_tick(&scheduler, 8000u, &state),
                            SWBT_BTSTACK_INPUT_REPORT_ERROR_SEND_FAILED);
    failed += expect_eq_int(capture.calls, 1);
    failed +=
        expect_eq_u64(swbt_btstack_input_report_scheduler_next_deadline_us(&scheduler), 16000u);

    swbt_btstack_input_report_scheduler_stop(&scheduler);
    failed += expect_false(swbt_btstack_input_report_scheduler_is_running(&scheduler));
    failed += expect_eq_int(swbt_btstack_input_report_scheduler_tick(&scheduler, 16000u, &state),
                            SWBT_BTSTACK_INPUT_REPORT_STOPPED);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += test_sends_standard_full_report_when_due();
    failed += test_late_tick_drops_catchup_reports();
    failed += test_stop_invalid_arguments_and_send_failure();
    return failed == 0 ? 0 : 1;
}
