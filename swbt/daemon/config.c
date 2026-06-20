#include "daemon/config.h"

swbt_daemon_config_t swbt_daemon_config_default(void) {
    swbt_daemon_config_t config = {
        .report_period_us = SWBT_DAEMON_DEFAULT_REPORT_PERIOD_US,
        .ipc_host = SWBT_DAEMON_DEFAULT_IPC_HOST,
        .ipc_port = SWBT_DAEMON_DEFAULT_IPC_PORT,
        .ipc_backlog = SWBT_DAEMON_DEFAULT_IPC_BACKLOG,
        .report_options = {0},
    };
    return config;
}
