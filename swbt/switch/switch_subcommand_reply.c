#include "switch/switch_subcommand_reply.h"

static uint16_t swbt_clamp_stick_12bit(uint16_t value) {
    return value > 0x0FFFu ? 0x0FFFu : value;
}

static void swbt_zero_reply_report(uint8_t *out) {
    for (size_t index = 0; index < SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE; ++index) {
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

static void swbt_write_standard_prefix(uint8_t *out, const swbt_state_t *state,
                                       const swbt_switch_report_options_t *report_options) {
    out[0] = SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY;
    out[1] = report_options->timer;
    out[2] = report_options->battery_connection;
    out[3] = (uint8_t)(state->buttons & 0xFFu);
    out[4] = (uint8_t)((state->buttons >> 8) & 0xFFu);
    out[5] = (uint8_t)((state->buttons >> 16) & 0xFFu);
    swbt_write_stick(&out[6], state->lx, state->ly);
    swbt_write_stick(&out[9], state->rx, state->ry);
    out[12] = report_options->vibrator_report;
}

swbt_switch_subcommand_reply_result_t
swbt_switch_build_subcommand_reply(const swbt_state_t *state,
                                   const swbt_switch_report_options_t *report_options,
                                   const swbt_switch_subcommand_reply_t *reply, uint8_t *out_report,
                                   size_t out_report_size, size_t *out_written) {
    if (out_written != NULL) {
        *out_written = 0;
    }

    if (state == NULL || report_options == NULL || reply == NULL || out_report == NULL ||
        (reply->data_len > 0u && reply->data == NULL)) {
        return SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_INVALID_ARGUMENT;
    }
    if (reply->data_len > SWBT_SWITCH_SUBCOMMAND_REPLY_MAX_DATA_SIZE) {
        return SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_DATA_TOO_LARGE;
    }
    if (out_report_size < SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE) {
        if (out_written != NULL) {
            *out_written = SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE;
        }
        return SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_BUFFER_TOO_SMALL;
    }

    swbt_zero_reply_report(out_report);
    swbt_write_standard_prefix(out_report, state, report_options);
    out_report[SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_OFFSET] = reply->ack;
    out_report[SWBT_SWITCH_SUBCOMMAND_REPLY_SUBCOMMAND_ID_OFFSET] = reply->subcommand_id;
    for (size_t index = 0; index < reply->data_len; ++index) {
        out_report[SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET + index] = reply->data[index];
    }

    if (out_written != NULL) {
        *out_written = SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE;
    }
    return SWBT_SWITCH_SUBCOMMAND_REPLY_OK;
}
