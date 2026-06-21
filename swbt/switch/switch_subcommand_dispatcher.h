#ifndef SWBT_SWITCH_SUBCOMMAND_DISPATCHER_H
#define SWBT_SWITCH_SUBCOMMAND_DISPATCHER_H

#include <stddef.h>
#include <stdint.h>

#include "switch/switch_device_info.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_player_lights.h"
#include "switch/switch_report.h"
#include "switch/switch_spi.h"
#include "switch/switch_subcommand.h"
#include "switch/switch_subcommand_reply.h"

typedef enum {
    SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK = 0,
    SWBT_SWITCH_SUBCOMMAND_DISPATCH_NO_REPLY = 1,
    SWBT_SWITCH_SUBCOMMAND_DISPATCH_UNSUPPORTED = 2,
    SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_INVALID_ARGUMENT = -1,
    SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_MALFORMED_SUBCOMMAND = -2,
    SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_SPI_READ_FAILED = -3,
    SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_REPLY_BUILD_FAILED = -4,
    SWBT_SWITCH_SUBCOMMAND_DISPATCH_ERROR_PLAYER_LIGHTS_FAILED = -5,
} swbt_switch_subcommand_dispatch_result_t;

typedef enum {
    SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_NONE = 0,
    SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY = 1,
} swbt_switch_subcommand_dispatch_action_t;

typedef struct {
    const swbt_state_t *state;
    const swbt_switch_report_options_t *report_options;
    const swbt_switch_spi_t *spi;
    swbt_switch_player_lights_state_t *player_lights;
    const swbt_switch_device_info_t *device_info;
} swbt_switch_subcommand_dispatcher_config_t;

typedef struct {
    swbt_switch_subcommand_dispatch_action_t action;
    uint8_t report[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE];
    size_t report_size;
} swbt_switch_subcommand_dispatcher_response_t;

swbt_switch_subcommand_dispatch_result_t
swbt_switch_subcommand_dispatch(const swbt_switch_subcommand_dispatcher_config_t *config,
                                const swbt_switch_output_report_t *output_report,
                                swbt_switch_subcommand_dispatcher_response_t *response);

#endif
