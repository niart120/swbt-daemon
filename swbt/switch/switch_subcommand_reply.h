#ifndef SWBT_SWITCH_SUBCOMMAND_REPLY_H
#define SWBT_SWITCH_SUBCOMMAND_REPLY_H

#include <stddef.h>
#include <stdint.h>

#include "switch/switch_controller_state.h"
#include "switch/switch_report.h"

#define SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY 0x21u
#define SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE 50u
#define SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_OFFSET 13u
#define SWBT_SWITCH_SUBCOMMAND_REPLY_SUBCOMMAND_ID_OFFSET 14u
#define SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET 15u
#define SWBT_SWITCH_SUBCOMMAND_REPLY_MAX_DATA_SIZE 35u

#define SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SIMPLE 0x80u
#define SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_DEVICE_INFO 0x82u
#define SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_TRIGGER_BUTTONS_ELAPSED 0x83u
#define SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SPI_READ 0x90u
#define SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_MCU_CONFIG 0xA0u
#define SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_PLAYER_LIGHTS 0xB0u

typedef enum {
    SWBT_SWITCH_SUBCOMMAND_REPLY_OK = 0,
    SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_INVALID_ARGUMENT = -1,
    SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_BUFFER_TOO_SMALL = -2,
    SWBT_SWITCH_SUBCOMMAND_REPLY_ERROR_DATA_TOO_LARGE = -3,
} swbt_switch_subcommand_reply_result_t;

typedef struct {
    uint8_t ack;
    uint8_t subcommand_id;
    const uint8_t *data;
    size_t data_len;
} swbt_switch_subcommand_reply_t;

swbt_switch_subcommand_reply_result_t
swbt_switch_build_subcommand_reply(const swbt_state_t *state,
                                   const swbt_switch_report_options_t *report_options,
                                   const swbt_switch_subcommand_reply_t *reply, uint8_t *out_report,
                                   size_t out_report_size, size_t *out_written);

#endif
