#include "switch/switch_report.h"

static uint16_t swbt_clamp_stick_12bit(uint16_t value) {
    return value > 0x0FFFu ? 0x0FFFu : value;
}

static void swbt_zero_report(uint8_t *out) {
    for (size_t index = 0; index < SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE; ++index) {
        out[index] = 0;
    }
}

static void swbt_write_stick(uint8_t *out, uint16_t x, uint16_t y) {
    const uint16_t clamped_x = swbt_clamp_stick_12bit(x);
    const uint16_t clamped_y = swbt_clamp_stick_12bit(y);

    out[0] = (uint8_t)(clamped_x & 0xFFu);
    out[1] = (uint8_t)(((clamped_x >> 8) & 0x0Fu) | ((clamped_y & 0x0Fu) << 4));
    out[2] = (uint8_t)((clamped_y >> 4) & 0xFFu);
}

static void swbt_write_i16_le(uint8_t *out, int16_t value) {
    const uint16_t raw = (uint16_t)value;

    out[0] = (uint8_t)(raw & 0xFFu);
    out[1] = (uint8_t)((raw >> 8) & 0xFFu);
}

static void swbt_write_imu_frame(uint8_t *out, const swbt_state_t *state) {
    swbt_write_i16_le(&out[0], state->accel_x);
    swbt_write_i16_le(&out[2], state->accel_y);
    swbt_write_i16_le(&out[4], state->accel_z);
    swbt_write_i16_le(&out[6], state->gyro_x);
    swbt_write_i16_le(&out[8], state->gyro_y);
    swbt_write_i16_le(&out[10], state->gyro_z);
}

swbt_switch_report_result_t swbt_switch_build_standard_full_report(
    const swbt_state_t *state, const swbt_switch_report_options_t *options, uint8_t *out_report,
    size_t out_report_size, size_t *out_written) {
    if (out_written != NULL) {
        *out_written = 0;
    }

    if (state == NULL || options == NULL || out_report == NULL) {
        return SWBT_SWITCH_REPORT_ERROR_INVALID_ARGUMENT;
    }
    if (out_report_size < SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE) {
        if (out_written != NULL) {
            *out_written = SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE;
        }
        return SWBT_SWITCH_REPORT_ERROR_BUFFER_TOO_SMALL;
    }

    swbt_zero_report(out_report);

    out_report[0] = SWBT_SWITCH_INPUT_REPORT_STANDARD_FULL;
    out_report[1] = options->timer;
    out_report[2] = options->battery_connection;
    out_report[3] = (uint8_t)(state->buttons & 0xFFu);
    out_report[4] = (uint8_t)((state->buttons >> 8) & 0xFFu);
    out_report[5] = (uint8_t)((state->buttons >> 16) & 0xFFu);
    swbt_write_stick(&out_report[6], state->lx, state->ly);
    swbt_write_stick(&out_report[9], state->rx, state->ry);
    out_report[12] = options->vibrator_report;

    for (size_t frame = 0; frame < 3u; ++frame) {
        swbt_write_imu_frame(&out_report[13u + (frame * 12u)], state);
    }

    if (out_written != NULL) {
        *out_written = SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE;
    }
    return SWBT_SWITCH_REPORT_OK;
}
