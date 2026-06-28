#include "daemon/btstack_report_timer_bridge.h"

#include "support/diagnostics.h"

static bool swbt_daemon_btstack_report_timer_bridge_is_valid(
    const swbt_daemon_btstack_report_timer_bridge_t *timer) {
    return timer != NULL && timer->config != NULL && timer->port != NULL &&
           timer->port->init != NULL && timer->port->stop != NULL &&
           timer->port->send_neutral_now != NULL && timer->port->enqueue_subcommand_reply != NULL &&
           timer->adapter != NULL && timer->device != NULL && timer->host != NULL &&
           timer->initialized != NULL;
}

static int swbt_daemon_btstack_report_timer_bridge_send(void *context, uint16_t hid_cid,
                                                        const uint8_t *message,
                                                        size_t message_size) {
    swbt_daemon_btstack_report_timer_bridge_t *timer = context;
    if (!swbt_daemon_btstack_report_timer_bridge_is_valid(timer)) {
        return -1;
    }

    return swbt_btstack_device_send(timer->device, hid_cid, message, message_size) ==
                   SWBT_BTSTACK_DEVICE_OK
               ? 0
               : -1;
}

static swbt_metrics_report_send_result_t swbt_daemon_production_report_send_result(
    swbt_btstack_input_report_timer_report_send_result_t send_result) {
    switch (send_result) {
    case SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_OK:
        return SWBT_METRICS_REPORT_SEND_OK;
    case SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_FAILED:
        return SWBT_METRICS_REPORT_SEND_FAILED;
    }
    return SWBT_METRICS_REPORT_SEND_FAILED;
}

static void swbt_daemon_production_record_report_tick(
    void *context, uint64_t now_us,
    swbt_btstack_input_report_timer_report_send_result_t send_result) {
    swbt_daemon_btstack_report_timer_bridge_t *timer = context;
    swbt_domain_t *app;

    if (!swbt_daemon_btstack_report_timer_bridge_is_valid(timer) || *timer->host == NULL) {
        return;
    }
    app = swbt_daemon_process_app(*timer->host);
    if (app == NULL) {
        return;
    }

    (void)swbt_domain_record_report_tick(app, now_us,
                                         swbt_daemon_production_report_send_result(send_result));
}

static swbt_btstack_input_report_timer_adapter_config_t
swbt_daemon_btstack_report_timer_bridge_config(swbt_daemon_btstack_report_timer_bridge_t *timer,
                                               swbt_runtime_state_provider_t state_provider,
                                               void *state_context) {
    return (swbt_btstack_input_report_timer_adapter_config_t){
        .backend = NULL,
        .hid_sender = swbt_daemon_btstack_report_timer_bridge_send,
        .hid_sender_context = timer,
        .state_provider = state_provider,
        .state_context = state_context,
        .report_tick_observer = swbt_daemon_production_record_report_tick,
        .report_tick_context = timer,
        .scheduler_config =
            {
                .report_period_us = timer->config->report_period_us,
                .report_options = timer->config->report_options,
            },
    };
}

int swbt_daemon_btstack_report_timer_bridge_start(swbt_daemon_btstack_report_timer_bridge_t *timer,
                                                  swbt_runtime_state_provider_t state_provider,
                                                  void *state_context) {
    if (!swbt_daemon_btstack_report_timer_bridge_is_valid(timer)) {
        return -1;
    }

    const swbt_btstack_input_report_timer_adapter_config_t config =
        swbt_daemon_btstack_report_timer_bridge_config(timer, state_provider, state_context);
    if (timer->port->init(timer->port_context, timer->adapter, &config) != 0) {
        return -1;
    }
    *timer->initialized = true;
    return 0;
}

void swbt_daemon_btstack_report_timer_bridge_stop(
    swbt_daemon_btstack_report_timer_bridge_t *timer) {
    if (!swbt_daemon_btstack_report_timer_bridge_is_valid(timer) || !*timer->initialized) {
        return;
    }
    timer->port->stop(timer->port_context, timer->adapter);
    *timer->initialized = false;
}

int swbt_daemon_btstack_report_timer_bridge_send_neutral_now(
    swbt_daemon_btstack_report_timer_bridge_t *timer) {
    if (!swbt_daemon_btstack_report_timer_bridge_is_valid(timer)) {
        swbt_diagnostic_trace("production: neutral send adapter unavailable");
        return -1;
    }
    if (!*timer->initialized) {
        swbt_diagnostic_trace("production: neutral send timer uninitialized");
        return -1;
    }
    if (!timer->adapter->running) {
        swbt_diagnostic_trace("production: neutral send timer stopped");
        return -1;
    }
    const int result = timer->port->send_neutral_now(timer->port_context, timer->adapter);
    if (result == 0) {
        swbt_diagnostic_trace("production: neutral send adapter ok");
    } else if (result > 0) {
        swbt_diagnostic_trace("production: neutral send adapter pending");
    } else {
        swbt_diagnostic_trace("production: neutral send adapter error");
    }
    return result;
}

int swbt_daemon_btstack_report_timer_bridge_enqueue_subcommand_reply(
    swbt_daemon_btstack_report_timer_bridge_t *timer, uint16_t hid_cid, const uint8_t *report,
    size_t report_size) {
    if (!swbt_daemon_btstack_report_timer_bridge_is_valid(timer) || !*timer->initialized) {
        return -1;
    }
    return timer->port->enqueue_subcommand_reply(timer->port_context, timer->adapter, hid_cid,
                                                 report, report_size);
}
