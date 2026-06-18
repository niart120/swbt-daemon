#include "btstack_bridge/input_report_scheduler.h"

static bool swbt_btstack_input_report_scheduler_is_valid(
    const swbt_btstack_input_report_scheduler_t *scheduler) {
    return scheduler != NULL && scheduler->send_callback != NULL &&
           scheduler->report_period_us > 0u;
}

static void
swbt_btstack_input_report_scheduler_advance(swbt_btstack_input_report_scheduler_t *scheduler,
                                            uint64_t now_us) {
    const uint64_t next_deadline = scheduler->next_deadline_us + scheduler->report_period_us;
    scheduler->next_deadline_us =
        next_deadline <= now_us ? now_us + scheduler->report_period_us : next_deadline;
}

swbt_btstack_input_report_result_t swbt_btstack_input_report_scheduler_init(
    swbt_btstack_input_report_scheduler_t *scheduler,
    swbt_btstack_input_report_send_callback_t send_callback, void *send_context,
    const swbt_btstack_input_report_scheduler_config_t *config) {
    if (scheduler == NULL || send_callback == NULL || config == NULL ||
        config->report_period_us == 0u) {
        return SWBT_BTSTACK_INPUT_REPORT_ERROR_INVALID_ARGUMENT;
    }

    scheduler->send_callback = send_callback;
    scheduler->send_context = send_context;
    scheduler->report_period_us = config->report_period_us;
    scheduler->report_options = config->report_options;
    scheduler->hid_cid = 0u;
    scheduler->running = false;
    scheduler->next_deadline_us = 0u;
    scheduler->timer = config->report_options.timer;
    for (size_t index = 0; index < sizeof(scheduler->scratch); ++index) {
        scheduler->scratch[index] = 0u;
    }

    return SWBT_BTSTACK_INPUT_REPORT_OK;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_btstack_input_report_result_t
swbt_btstack_input_report_scheduler_start(swbt_btstack_input_report_scheduler_t *scheduler,
                                          uint16_t hid_cid, uint64_t now_us) {
    if (!swbt_btstack_input_report_scheduler_is_valid(scheduler)) {
        return SWBT_BTSTACK_INPUT_REPORT_ERROR_INVALID_ARGUMENT;
    }

    scheduler->hid_cid = hid_cid;
    scheduler->running = true;
    scheduler->next_deadline_us = now_us + scheduler->report_period_us;
    scheduler->timer = scheduler->report_options.timer;
    return SWBT_BTSTACK_INPUT_REPORT_OK;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

void swbt_btstack_input_report_scheduler_stop(swbt_btstack_input_report_scheduler_t *scheduler) {
    if (scheduler == NULL) {
        return;
    }

    scheduler->running = false;
    scheduler->hid_cid = 0u;
}

bool swbt_btstack_input_report_scheduler_is_running(
    const swbt_btstack_input_report_scheduler_t *scheduler) {
    return scheduler != NULL && scheduler->running;
}

uint64_t swbt_btstack_input_report_scheduler_next_deadline_us(
    const swbt_btstack_input_report_scheduler_t *scheduler) {
    return scheduler == NULL ? 0u : scheduler->next_deadline_us;
}

swbt_btstack_input_report_result_t
swbt_btstack_input_report_scheduler_tick(swbt_btstack_input_report_scheduler_t *scheduler,
                                         uint64_t now_us, const swbt_state_t *state) {
    if (!swbt_btstack_input_report_scheduler_is_valid(scheduler) || state == NULL) {
        return SWBT_BTSTACK_INPUT_REPORT_ERROR_INVALID_ARGUMENT;
    }
    if (!scheduler->running) {
        return SWBT_BTSTACK_INPUT_REPORT_STOPPED;
    }
    if (now_us < scheduler->next_deadline_us) {
        return SWBT_BTSTACK_INPUT_REPORT_NOT_DUE;
    }

    swbt_switch_report_options_t report_options = scheduler->report_options;
    report_options.timer = scheduler->timer;

    size_t written = 0u;
    if (swbt_switch_build_standard_full_report(state, &report_options, scheduler->scratch,
                                               sizeof(scheduler->scratch),
                                               &written) != SWBT_SWITCH_REPORT_OK) {
        return SWBT_BTSTACK_INPUT_REPORT_ERROR_BUILD_FAILED;
    }

    const int send_result = scheduler->send_callback(scheduler->send_context, scheduler->hid_cid,
                                                     scheduler->scratch, written);
    swbt_btstack_input_report_scheduler_advance(scheduler, now_us);
    if (send_result != 0) {
        return SWBT_BTSTACK_INPUT_REPORT_ERROR_SEND_FAILED;
    }

    scheduler->timer = (uint8_t)(scheduler->timer + 1u);
    return SWBT_BTSTACK_INPUT_REPORT_OK;
}
