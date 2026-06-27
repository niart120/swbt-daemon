#ifndef SWBT_DAEMON_PRODUCTION_REPORT_TIMER_H
#define SWBT_DAEMON_PRODUCTION_REPORT_TIMER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/device.h"
#include "btstack_bridge/production_ports.h"
#include "daemon/config.h"
#include "daemon/process.h"

typedef struct {
    const swbt_daemon_config_t *config;
    const swbt_btstack_production_report_timer_port_t *port;
    void *port_context;
    swbt_btstack_input_report_timer_adapter_t *adapter;
    swbt_btstack_device_t *device;
    swbt_daemon_process_t **host;
    bool *initialized;
} swbt_daemon_production_report_timer_t;

int swbt_daemon_production_report_timer_start(swbt_daemon_production_report_timer_t *timer,
                                              swbt_daemon_process_state_provider_t state_provider,
                                              void *state_context);

void swbt_daemon_production_report_timer_stop(swbt_daemon_production_report_timer_t *timer);

int swbt_daemon_production_report_timer_send_neutral_now(
    swbt_daemon_production_report_timer_t *timer);

int swbt_daemon_production_report_timer_enqueue_subcommand_reply(
    swbt_daemon_production_report_timer_t *timer, uint16_t hid_cid, const uint8_t *report,
    size_t report_size);

#endif
