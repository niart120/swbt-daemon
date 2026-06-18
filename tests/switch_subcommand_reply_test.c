#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "switch/switch_controller_state.h"
#include "switch/switch_report.h"
#include "switch/switch_subcommand.h"
#include "switch/switch_subcommand_reply.h"

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
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

static int expect_zero_range(const uint8_t *actual, size_t start, size_t end) {
    for (size_t index = start; index < end; ++index) {
        if (actual[index] != 0u) {
            return 1;
        }
    }
    return 0;
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

static swbt_switch_report_options_t sample_report_options(void) {
    swbt_switch_report_options_t options = {
        .timer = 0x42u,
        .battery_connection = 0x8Eu,
        .vibrator_report = 0x80u,
    };
    return options;
}

static int test_builds_subcommand_reply_report(void) {
    uint8_t report[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE];
    size_t written = 0;
    const uint8_t reply_data[] = {
        0x80u, 0x60u, 0x00u, 0x00u, 0x03u, 0xAAu, 0xBBu, 0xCCu,
    };
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    const swbt_switch_subcommand_reply_t reply = {
        .ack = SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SPI_READ,
        .subcommand_id = SWBT_SWITCH_SUBCOMMAND_SPI_FLASH_READ,
        .data = reply_data,
        .data_len = sizeof(reply_data),
    };

    if (swbt_switch_build_subcommand_reply(&state, &report_options, &reply, report, sizeof(report),
                                           &written) != SWBT_SWITCH_SUBCOMMAND_REPLY_OK) {
        return 1;
    }

    int failed = 0;
    failed += expect_eq_size(written, SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    failed += expect_eq_u8(report[0], SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY);
    failed += expect_eq_u8(report[1], 0x42u);
    failed += expect_eq_u8(report[2], 0x8Eu);
    failed += expect_eq_u8(report[3], 0x08u);
    failed += expect_eq_u8(report[4], 0x24u);
    failed += expect_eq_u8(report[5], 0x42u);
    failed += expect_stick_bytes(&report[6], 0x23u, 0xC1u, 0xABu);
    failed += expect_stick_bytes(&report[9], 0xFFu, 0xFFu, 0x00u);
    failed += expect_eq_u8(report[12], 0x80u);
    failed += expect_eq_u8(report[13], SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SPI_READ);
    failed += expect_eq_u8(report[14], SWBT_SWITCH_SUBCOMMAND_SPI_FLASH_READ);
    for (size_t index = 0; index < sizeof(reply_data); ++index) {
        failed += expect_eq_u8(report[SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET + index],
                               reply_data[index]);
    }
    failed +=
        expect_zero_range(report, SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET + sizeof(reply_data),
                          SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    return failed;
}

static int test_rejects_too_small_buffer(void) {
    uint8_t report[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE - 1u];
    size_t written = 0;
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    const swbt_switch_subcommand_reply_t reply = {
        .ack = SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SIMPLE,
        .subcommand_id = SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE,
        .data = NULL,
        .data_len = 0u,
    };

    if (swbt_switch_build_subcommand_reply(&state, &report_options, &reply, report, sizeof(report),
                                           &written) !=
        SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_BUFFER_TOO_SMALL) {
        return 1;
    }
    return expect_eq_size(written, SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
}

static int test_rejects_invalid_arguments_and_oversized_data(void) {
    uint8_t report[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE];
    size_t written = 99u;
    uint8_t oversized[SWBT_SWITCH_SUBCOMMAND_REPLY_MAX_DATA_SIZE + 1u] = {0};
    const swbt_state_t state = sample_state();
    const swbt_switch_report_options_t report_options = sample_report_options();
    swbt_switch_subcommand_reply_t reply = {
        .ack = SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SIMPLE,
        .subcommand_id = SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE,
        .data = oversized,
        .data_len = sizeof(oversized),
    };

    int failed = 0;
    failed += swbt_switch_build_subcommand_reply(NULL, &report_options, &reply, report,
                                                 sizeof(report), &written) !=
              SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_INVALID_ARGUMENT;
    failed += expect_eq_size(written, 0u);
    failed += swbt_switch_build_subcommand_reply(&state, NULL, &reply, report, sizeof(report),
                                                 &written) !=
              SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_INVALID_ARGUMENT;
    failed += swbt_switch_build_subcommand_reply(&state, &report_options, NULL, report,
                                                 sizeof(report), &written) !=
              SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_INVALID_ARGUMENT;
    failed += swbt_switch_build_subcommand_reply(&state, &report_options, &reply, NULL,
                                                 sizeof(report), &written) !=
              SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_INVALID_ARGUMENT;
    failed += swbt_switch_build_subcommand_reply(&state, &report_options, &reply, report,
                                                 sizeof(report), &written) !=
              SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_DATA_TOO_LARGE;

    reply.data = NULL;
    reply.data_len = 1u;
    failed += swbt_switch_build_subcommand_reply(&state, &report_options, &reply, report,
                                                 sizeof(report), &written) !=
              SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_INVALID_ARGUMENT;
    return failed;
}

int main(void) {
    int failed = 0;
    failed += test_builds_subcommand_reply_report();
    failed += test_rejects_too_small_buffer();
    failed += test_rejects_invalid_arguments_and_oversized_data();
    return failed == 0 ? 0 : 1;
}
