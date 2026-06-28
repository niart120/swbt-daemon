#include "daemon/btstack_ipc_pump_adapter.h"

static bool swbt_daemon_production_ipc_runner_is_running(void *context) {
    return swbt_daemon_ipc_runner_is_running((const swbt_daemon_ipc_runner_t *)context);
}

static void swbt_daemon_production_ipc_runner_poll_once_at(void *context, uint32_t now_ms) {
    (void)swbt_daemon_ipc_runner_poll_once_at((swbt_daemon_ipc_runner_t *)context, now_ms);
}

static bool swbt_daemon_btstack_ipc_pump_adapter_is_valid(
    const swbt_daemon_btstack_ipc_pump_adapter_t *ipc_pump) {
    return ipc_pump != NULL && ipc_pump->runner != NULL && ipc_pump->port != NULL &&
           ipc_pump->port->start != NULL && ipc_pump->port->stop != NULL;
}

int swbt_daemon_btstack_ipc_pump_adapter_start(
    const swbt_daemon_btstack_ipc_pump_adapter_t *ipc_pump, swbt_control_t *control) {
    swbt_btstack_production_ipc_pump_t btstack_pump;

    if (!swbt_daemon_btstack_ipc_pump_adapter_is_valid(ipc_pump)) {
        return -1;
    }
    if (swbt_daemon_ipc_runner_start(ipc_pump->runner, control, &ipc_pump->runner->config) !=
        SWBT_DAEMON_IPC_RUNNER_OK) {
        return -1;
    }

    btstack_pump = (swbt_btstack_production_ipc_pump_t){
        .is_running = swbt_daemon_production_ipc_runner_is_running,
        .poll_once_at = swbt_daemon_production_ipc_runner_poll_once_at,
        .context = ipc_pump->runner,
    };
    if (ipc_pump->port->start(ipc_pump->port_context, &btstack_pump) != 0) {
        swbt_daemon_ipc_runner_stop(ipc_pump->runner);
        return -1;
    }
    return 0;
}

void swbt_daemon_btstack_ipc_pump_adapter_stop(
    const swbt_daemon_btstack_ipc_pump_adapter_t *ipc_pump) {
    if (!swbt_daemon_btstack_ipc_pump_adapter_is_valid(ipc_pump)) {
        return;
    }

    ipc_pump->port->stop(ipc_pump->port_context);
    swbt_daemon_ipc_runner_stop(ipc_pump->runner);
}
