#ifndef SWBT_SWITCH_REPORT_H
#define SWBT_SWITCH_REPORT_H

#include <stddef.h>
#include <stdint.h>

#include "switch/switch_controller_state.h"

#define SWBT_SWITCH_INPUT_REPORT_STANDARD_FULL 0x30u
#define SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE 49u

typedef enum {
    SWBT_SWITCH_REPORT_OK = 0,
    SWBT_SWITCH_REPORT_ERROR_INVALID_ARGUMENT = -1,
    SWBT_SWITCH_REPORT_ERROR_BUFFER_TOO_SMALL = -2,
} swbt_switch_report_result_t;

typedef struct {
    uint8_t timer;
    uint8_t battery_connection;
    uint8_t vibrator_report;
} swbt_switch_report_options_t;

swbt_switch_report_result_t swbt_switch_build_standard_full_report(
    const swbt_state_t *state, const swbt_switch_report_options_t *options, uint8_t *out_report,
    size_t out_report_size, size_t *out_written);

#endif
