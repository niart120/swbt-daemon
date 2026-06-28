#ifndef SWBT_DAEMON_BTSTACK_IPC_PUMP_ADAPTER_H
#define SWBT_DAEMON_BTSTACK_IPC_PUMP_ADAPTER_H

#include "btstack_bridge/production_ports.h"
#include "daemon/ipc_runner.h"

typedef struct {
    swbt_daemon_ipc_runner_t *runner;
    const swbt_btstack_production_ipc_pump_port_t *port;
    void *port_context;
} swbt_daemon_btstack_ipc_pump_adapter_t;

int swbt_daemon_btstack_ipc_pump_adapter_start(
    const swbt_daemon_btstack_ipc_pump_adapter_t *ipc_pump, swbt_control_t *control);

void swbt_daemon_btstack_ipc_pump_adapter_stop(
    const swbt_daemon_btstack_ipc_pump_adapter_t *ipc_pump);

#endif
