#ifndef SWBT_DAEMON_CONFIG_H
#define SWBT_DAEMON_CONFIG_H

#include <stdint.h>

#include "switch/switch_report.h"

#define SWBT_DAEMON_DEFAULT_IPC_HOST "127.0.0.1"
#define SWBT_DAEMON_DEFAULT_IPC_PORT 0u
#define SWBT_DAEMON_DEFAULT_IPC_BACKLOG 1
#define SWBT_DAEMON_DEFAULT_REPORT_PERIOD_US 8000u

typedef struct {
    uint32_t report_period_us;
    const char *ipc_host;
    uint16_t ipc_port;
    int ipc_backlog;
    swbt_switch_report_options_t report_options;
} swbt_daemon_config_t;

swbt_daemon_config_t swbt_daemon_config_default(void);

#endif
