#include <stdint.h>
#include <string.h>

#include "switch/switch_controller_state.h"
#include "switch/switch_report.h"

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_size(size_t actual, size_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_report_prefix(const uint8_t *report) {
    if (expect_eq_u8(report[0], SWBT_SWITCH_INPUT_REPORT_STANDARD_FULL)) {
        return 1;
    }
    if (expect_eq_u8(report[1], 0x42U)) {
        return 2;
    }
    if (expect_eq_u8(report[2], 0x8EU)) {
        return 3;
    }
    if (expect_eq_u8(report[12], 0x80U)) {
        return 4;
    }
    return 0;
}

static int expect_stick_bytes(const uint8_t *bytes, uint8_t b0, uint8_t b1, uint8_t b2) {
    if (expect_eq_u8(bytes[0], b0)) {
        return 1;
    }
    if (expect_eq_u8(bytes[1], b1)) {
        return 2;
    }
    if (expect_eq_u8(bytes[2], b2)) {
        return 3;
    }
    return 0;
}

static int expect_imu_frame(const uint8_t *bytes) {
    const uint8_t expected[12] = {
        0x01, 0x00, 0xFE, 0xFF, 0x03, 0x00, 0xFC, 0xFF, 0x05, 0x00, 0xFA, 0xFF,
    };

    return memcmp(bytes, expected, sizeof(expected)) == 0 ? 0 : 1;
}

int main(void) {
    uint8_t report[SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE];
    size_t written = 0;
    swbt_state_t state = swbt_state_neutral();
    swbt_switch_report_options_t options = {
        .timer = 0x42,
        .battery_connection = 0x8E,
        .vibrator_report = 0x80,
    };

    state.buttons =
        SWBT_BUTTON_A | SWBT_BUTTON_R_STICK | SWBT_BUTTON_CAPTURE | SWBT_BUTTON_UP | SWBT_BUTTON_L;
    state.lx = 0x123U;
    state.ly = 0xABCU;
    state.rx = 0xFFFU;
    state.ry = 0x000U;
    state.accel_x = 1;
    state.accel_y = -2;
    state.accel_z = 3;
    state.gyro_x = -4;
    state.gyro_y = 5;
    state.gyro_z = -6;

    if (swbt_switch_build_standard_full_report(&state, &options, report, sizeof(report),
                                               &written) != SWBT_SWITCH_REPORT_OK) {
        return 1;
    }
    if (expect_eq_size(written, SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE)) {
        return 2;
    }
    if (expect_report_prefix(report)) {
        return 3;
    }
    if (expect_eq_u8(report[3], 0x08U)) {
        return 4;
    }
    if (expect_eq_u8(report[4], 0x24U)) {
        return 5;
    }
    if (expect_eq_u8(report[5], 0x42U)) {
        return 6;
    }
    if (expect_stick_bytes(&report[6], 0x23U, 0xC1U, 0xABU)) {
        return 7;
    }
    if (expect_stick_bytes(&report[9], 0xFFU, 0x0FU, 0x00U)) {
        return 8;
    }
    for (size_t frame = 0; frame < 3; ++frame) {
        if (expect_imu_frame(&report[13 + (frame * 12)])) {
            return 9;
        }
    }

    if (swbt_switch_build_standard_full_report(&state, &options, report, sizeof(report) - 1U,
                                               &written) !=
        SWBT_SWITCH_REPORT_ERROR_BUFFER_TOO_SMALL) {
        return 10;
    }
    if (swbt_switch_build_standard_full_report(NULL, &options, report, sizeof(report), &written) !=
        SWBT_SWITCH_REPORT_ERROR_INVALID_ARGUMENT) {
        return 11;
    }

    return 0;
}
