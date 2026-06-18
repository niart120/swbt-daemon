#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "switch/switch_subcommand.h"

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
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

static int expect_rumble(const uint8_t *actual, const uint8_t *expected) {
    for (size_t index = 0; index < SWBT_SWITCH_RUMBLE_DATA_SIZE; ++index) {
        if (actual[index] != expected[index]) {
            return 1;
        }
    }
    return 0;
}

int main(void) {
    swbt_switch_output_report_t parsed;
    const uint8_t rumble[SWBT_SWITCH_RUMBLE_DATA_SIZE] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    };
    const uint8_t subcommand_packet[] = {
        SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND,
        0x0A,
        0x10,
        0x11,
        0x12,
        0x13,
        0x14,
        0x15,
        0x16,
        0x17,
        SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE,
        0x30,
        0x99,
    };
    const uint8_t rumble_packet[] = {
        SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY, 0x0B, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    };
    const uint8_t short_subcommand_packet[SWBT_SWITCH_OUTPUT_SUBCOMMAND_HEADER_SIZE - 1U] = {
        SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND,
    };
    const uint8_t short_rumble_packet[SWBT_SWITCH_OUTPUT_RUMBLE_ONLY_SIZE - 1U] = {
        SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY,
    };
    const uint8_t unsupported_report_packet[] = {
        SWBT_SWITCH_OUTPUT_REPORT_NFC_IR_MCU,
        0x00,
    };

    if (swbt_switch_parse_output_report(NULL, sizeof(subcommand_packet), &parsed) !=
        SWBT_SWITCH_SUBCOMMAND_ERROR_INVALID_ARGUMENT) {
        return 1;
    }
    if (swbt_switch_parse_output_report(subcommand_packet, sizeof(subcommand_packet), NULL) !=
        SWBT_SWITCH_SUBCOMMAND_ERROR_INVALID_ARGUMENT) {
        return 2;
    }
    if (swbt_switch_parse_output_report(subcommand_packet, 0, &parsed) !=
        SWBT_SWITCH_SUBCOMMAND_ERROR_PACKET_TOO_SHORT) {
        return 3;
    }
    if (swbt_switch_parse_output_report(short_subcommand_packet, sizeof(short_subcommand_packet),
                                        &parsed) != SWBT_SWITCH_SUBCOMMAND_ERROR_PACKET_TOO_SHORT) {
        return 4;
    }
    if (swbt_switch_parse_output_report(short_rumble_packet, sizeof(short_rumble_packet),
                                        &parsed) != SWBT_SWITCH_SUBCOMMAND_ERROR_PACKET_TOO_SHORT) {
        return 5;
    }
    if (swbt_switch_parse_output_report(unsupported_report_packet,
                                        sizeof(unsupported_report_packet), &parsed) !=
        SWBT_SWITCH_SUBCOMMAND_ERROR_UNSUPPORTED_REPORT_ID) {
        return 6;
    }
    if (swbt_switch_parse_output_report(subcommand_packet, sizeof(subcommand_packet), &parsed) !=
        SWBT_SWITCH_SUBCOMMAND_OK) {
        return 7;
    }
    if (expect_eq_u8(parsed.output_report_id, SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND)) {
        return 8;
    }
    if (expect_eq_u8(parsed.packet_counter, 0x0A)) {
        return 9;
    }
    if (expect_rumble(parsed.rumble, rumble)) {
        return 10;
    }
    if (expect_true(parsed.has_subcommand)) {
        return 11;
    }
    if (expect_eq_u8(parsed.subcommand_id, SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE)) {
        return 12;
    }
    if (expect_eq_size(parsed.subcommand_data_len, 2)) {
        return 13;
    }
    if (parsed.subcommand_data == NULL || parsed.subcommand_data[0] != 0x30 ||
        parsed.subcommand_data[1] != 0x99) {
        return 14;
    }
    if (swbt_switch_parse_output_report(rumble_packet, sizeof(rumble_packet), &parsed) !=
        SWBT_SWITCH_SUBCOMMAND_OK) {
        return 15;
    }
    if (expect_eq_u8(parsed.output_report_id, SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_ONLY)) {
        return 16;
    }
    if (expect_eq_u8(parsed.packet_counter, 0x0B)) {
        return 17;
    }
    if (expect_rumble(parsed.rumble, rumble)) {
        return 18;
    }
    if (expect_false(parsed.has_subcommand)) {
        return 19;
    }
    if (parsed.subcommand_data != NULL || parsed.subcommand_data_len != 0) {
        return 20;
    }
    if (expect_false(swbt_switch_subcommand_is_known(0x7FU))) {
        return 21;
    }
    if (expect_true(swbt_switch_subcommand_is_known(SWBT_SWITCH_SUBCOMMAND_REQUEST_DEVICE_INFO))) {
        return 22;
    }
    return 0;
}
