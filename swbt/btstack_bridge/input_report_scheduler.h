#ifndef SWBT_BTSTACK_BRIDGE_INPUT_REPORT_SCHEDULER_H
#define SWBT_BTSTACK_BRIDGE_INPUT_REPORT_SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "switch/switch_controller_state.h"
#include "switch/switch_report.h"

#define SWBT_BTSTACK_INPUT_REPORT_DEFAULT_PERIOD_US 8000u

typedef enum {
    SWBT_BTSTACK_INPUT_REPORT_OK = 0,
    SWBT_BTSTACK_INPUT_REPORT_NOT_DUE = 1,
    SWBT_BTSTACK_INPUT_REPORT_STOPPED = 2,
    SWBT_BTSTACK_INPUT_REPORT_ERROR_INVALID_ARGUMENT = -1,
    SWBT_BTSTACK_INPUT_REPORT_ERROR_BUILD_FAILED = -2,
    SWBT_BTSTACK_INPUT_REPORT_ERROR_SEND_FAILED = -3,
} swbt_btstack_input_report_result_t;

typedef int (*swbt_btstack_input_report_send_callback_t)(void *context, uint16_t hid_cid,
                                                         const uint8_t *report, size_t report_size);

typedef struct {
    uint32_t report_period_us;
    swbt_switch_report_options_t report_options;
} swbt_btstack_input_report_scheduler_config_t;

typedef struct {
    swbt_btstack_input_report_send_callback_t send_callback;
    void *send_context;
    uint32_t report_period_us;
    swbt_switch_report_options_t report_options;
    uint16_t hid_cid;
    bool running;
    uint64_t next_deadline_us;
    uint8_t timer;
    uint8_t scratch[SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE];
} swbt_btstack_input_report_scheduler_t;

swbt_btstack_input_report_result_t swbt_btstack_input_report_scheduler_init(
    swbt_btstack_input_report_scheduler_t *scheduler,
    swbt_btstack_input_report_send_callback_t send_callback, void *send_context,
    const swbt_btstack_input_report_scheduler_config_t *config);

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_btstack_input_report_result_t
swbt_btstack_input_report_scheduler_start(swbt_btstack_input_report_scheduler_t *scheduler,
                                          uint16_t hid_cid, uint64_t now_us);
// NOLINTEND(bugprone-easily-swappable-parameters)

void swbt_btstack_input_report_scheduler_stop(swbt_btstack_input_report_scheduler_t *scheduler);

bool swbt_btstack_input_report_scheduler_is_running(
    const swbt_btstack_input_report_scheduler_t *scheduler);

uint64_t swbt_btstack_input_report_scheduler_next_deadline_us(
    const swbt_btstack_input_report_scheduler_t *scheduler);

swbt_btstack_input_report_result_t
swbt_btstack_input_report_scheduler_tick(swbt_btstack_input_report_scheduler_t *scheduler,
                                         uint64_t now_us, const swbt_state_t *state);

#endif
