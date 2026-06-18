#include "switch/switch_subcommand.h"

static void swbt_switch_reset_output_report(swbt_switch_output_report_t *out_report) {
    out_report->output_report_id = 0;
    out_report->packet_counter = 0;
    for (size_t index = 0; index < SWBT_SWITCH_RUMBLE_DATA_SIZE; ++index) {
        out_report->rumble[index] = 0;
    }
    out_report->has_subcommand = false;
    out_report->subcommand_id = 0;
    out_report->subcommand_data = NULL;
    out_report->subcommand_data_len = 0;
}

static void swbt_switch_copy_rumble(const uint8_t *data, swbt_switch_output_report_t *out_report) {
    for (size_t index = 0; index < SWBT_SWITCH_RUMBLE_DATA_SIZE; ++index) {
        out_report->rumble[index] = data[2u + index];
    }
}

static swbt_switch_subcommand_result_t
swbt_switch_parse_rumble_only(const uint8_t *data, size_t data_len,
                              swbt_switch_output_report_t *out_report) {
    if (data_len < SWBT_SWITCH_OUTPUT_RUMBLE_ONLY_SIZE) {
        return SWBT_SWITCH_SUBCOMMAND_ERROR_PACKET_TOO_SHORT;
    }

    out_report->packet_counter = data[1];
    swbt_switch_copy_rumble(data, out_report);
    return SWBT_SWITCH_SUBCOMMAND_OK;
}

static swbt_switch_subcommand_result_t
swbt_switch_parse_rumble_and_subcommand(const uint8_t *data, size_t data_len,
                                        swbt_switch_output_report_t *out_report) {
    if (data_len < SWBT_SWITCH_OUTPUT_SUBCOMMAND_HEADER_SIZE) {
        return SWBT_SWITCH_SUBCOMMAND_ERROR_PACKET_TOO_SHORT;
    }

    out_report->packet_counter = data[1];
    swbt_switch_copy_rumble(data, out_report);
    out_report->has_subcommand = true;
    out_report->subcommand_id = data[10];
    out_report->subcommand_data_len = data_len - SWBT_SWITCH_OUTPUT_SUBCOMMAND_HEADER_SIZE;
    out_report->subcommand_data = out_report->subcommand_data_len > 0u
                                      ? &data[SWBT_SWITCH_OUTPUT_SUBCOMMAND_HEADER_SIZE]
                                      : NULL;
    return SWBT_SWITCH_SUBCOMMAND_OK;
}

swbt_switch_subcommand_result_t
swbt_switch_parse_output_report(const uint8_t *data, size_t data_len,
                                swbt_switch_output_report_t *out_report) {
    if (out_report != NULL) {
        swbt_switch_reset_output_report(out_report);
    }
    if (data == NULL || out_report == NULL) {
        return SWBT_SWITCH_SUBCOMMAND_ERROR_INVALID_ARGUMENT;
    }
    if (data_len < 1u) {
        return SWBT_SWITCH_SUBCOMMAND_ERROR_PACKET_TOO_SHORT;
    }

    out_report->output_report_id = data[0];
    switch (data[0]) {
    case SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND:
        return swbt_switch_parse_rumble_and_subcommand(data, data_len, out_report);
    case SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY:
        return swbt_switch_parse_rumble_only(data, data_len, out_report);
    default:
        return SWBT_SWITCH_SUBCOMMAND_ERROR_UNSUPPORTED_REPORT_ID;
    }
}

bool swbt_switch_subcommand_is_known(uint8_t subcommand_id) {
    switch (subcommand_id) {
    case SWBT_SWITCH_SUBCOMMAND_STATE:
    case SWBT_SWITCH_SUBCOMMAND_MANUAL_BT_PAIRING:
    case SWBT_SWITCH_SUBCOMMAND_REQUEST_DEVICE_INFO:
    case SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE:
    case SWBT_SWITCH_SUBCOMMAND_TRIGGER_BUTTONS_ELAPSED:
    case SWBT_SWITCH_SUBCOMMAND_GET_PAGE_LIST_STATE:
    case SWBT_SWITCH_SUBCOMMAND_SET_HCI_STATE:
    case SWBT_SWITCH_SUBCOMMAND_RESET_PAIRING_INFO:
    case SWBT_SWITCH_SUBCOMMAND_LOW_POWER_MODE:
    case SWBT_SWITCH_SUBCOMMAND_SPI_FLASH_READ:
    case SWBT_SWITCH_SUBCOMMAND_SPI_FLASH_WRITE:
    case SWBT_SWITCH_SUBCOMMAND_RESET_MCU:
    case SWBT_SWITCH_SUBCOMMAND_SET_MCU_CONFIG:
    case SWBT_SWITCH_SUBCOMMAND_SET_MCU_STATE:
    case SWBT_SWITCH_SUBCOMMAND_SET_PLAYER_LIGHTS:
    case SWBT_SWITCH_SUBCOMMAND_GET_PLAYER_LIGHTS:
    case SWBT_SWITCH_SUBCOMMAND_SET_HOME_LIGHT:
    case SWBT_SWITCH_SUBCOMMAND_ENABLE_IMU:
    case SWBT_SWITCH_SUBCOMMAND_SET_IMU_SENSITIVITY:
    case SWBT_SWITCH_SUBCOMMAND_WRITE_IMU_REGISTER:
    case SWBT_SWITCH_SUBCOMMAND_READ_IMU_REGISTER:
    case SWBT_SWITCH_SUBCOMMAND_ENABLE_VIBRATION:
    case SWBT_SWITCH_SUBCOMMAND_GET_REGULATED_VOLTAGE:
        return true;
    default:
        return false;
    }
}
