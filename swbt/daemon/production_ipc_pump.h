#ifndef SWBT_DAEMON_PRODUCTION_IPC_PUMP_H
#define SWBT_DAEMON_PRODUCTION_IPC_PUMP_H

#include "btstack_bridge/production_ports.h"
#include "daemon/ipc_runner.h"

typedef struct {
    swbt_daemon_ipc_runner_t *runner;
    const swbt_btstack_production_ipc_pump_port_t *port;
    void *port_context;
} swbt_daemon_production_ipc_pump_t;

int swbt_daemon_production_ipc_pump_start(const swbt_daemon_production_ipc_pump_t *ipc_pump,
                                          swbt_control_t *control);

void swbt_daemon_production_ipc_pump_stop(const swbt_daemon_production_ipc_pump_t *ipc_pump);

#endif
