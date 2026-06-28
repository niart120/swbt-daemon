#ifndef SWBT_DAEMON_PRODUCTION_RUNNER_INTERNAL_H
#define SWBT_DAEMON_PRODUCTION_RUNNER_INTERNAL_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "btstack_bridge/production_ports.h"
#include "daemon/ipc_runner.h"
#include "daemon/process.h"
#include "daemon/btstack_hid_session.h"
#include "daemon/btstack_report_timer_bridge.h"
#include "daemon/production_runner.h"
#include "daemon/shutdown_sequence.h"

#define SWBT_DAEMON_PRODUCTION_HID_SERVICE_BUFFER_SIZE 512u

struct swbt_daemon_production_runner {
    swbt_daemon_config_t config;
    swbt_daemon_config_file_target_t learned_switch_address_target;
    swbt_daemon_ipc_runner_t ipc_runner;
    swbt_btstack_device_t device;
    swbt_btstack_input_report_timer_adapter_t report_timer;
    swbt_daemon_btstack_hid_session_t hid_session_bridge;
    swbt_daemon_btstack_report_timer_bridge_t report_timer_bridge;
    swbt_daemon_shutdown_sequence_t shutdown;
    const swbt_btstack_production_ports_t *ports;
    void *ports_context;
    swbt_daemon_process_t *host;
    uint8_t hid_service_buffer[SWBT_DAEMON_PRODUCTION_HID_SERVICE_BUFFER_SIZE];
    bool initialized;
    bool report_timer_initialized;
    bool learned_switch_address_target_configured;
    bool adapter_location_configured;
    atomic_bool hardware_powered;
};

#endif
