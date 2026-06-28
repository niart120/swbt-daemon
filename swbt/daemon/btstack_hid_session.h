#ifndef SWBT_DAEMON_BTSTACK_HID_SESSION_H
#define SWBT_DAEMON_BTSTACK_HID_SESSION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/device.h"
#include "btstack_bridge/production_ports.h"
#include "daemon/config.h"

typedef void (*swbt_daemon_btstack_hid_session_finish_shutdown_t)(void *context);
typedef void (*swbt_daemon_btstack_hid_session_hid_open_completed_t)(void *context);

typedef struct {
    swbt_daemon_config_t *config;
    const swbt_btstack_device_port_t *device_port;
    const swbt_btstack_production_report_timer_port_t *report_timer_port;
    const swbt_btstack_production_controller_port_t *controller_port;
    const swbt_btstack_production_clock_port_t *clock_port;
    void *port_context;
    swbt_btstack_device_t *device;
    swbt_btstack_input_report_timer_adapter_t *report_timer;
    bool *report_timer_initialized;
    bool *shutdown_neutral_pending;
    bool *shutdown_disconnect_pending;
    swbt_daemon_config_file_target_t *learned_switch_address_target;
    bool *learned_switch_address_target_configured;
    uint8_t *service_buffer;
    size_t service_buffer_size;
    swbt_daemon_btstack_hid_session_hid_open_completed_t hid_open_completed;
    void *hid_open_completed_context;
    swbt_daemon_btstack_hid_session_finish_shutdown_t finish_shutdown;
    void *finish_shutdown_context;
} swbt_daemon_btstack_hid_session_t;

int swbt_daemon_btstack_hid_session_register(swbt_daemon_btstack_hid_session_t *session);

void swbt_daemon_btstack_hid_session_stop(swbt_daemon_btstack_hid_session_t *session);

#endif
