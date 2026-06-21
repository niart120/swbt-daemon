#include "daemon/config.h"

#include <string.h>

swbt_daemon_config_t swbt_daemon_config_default(void) {
    swbt_daemon_config_t config = {
        .report_period_us = SWBT_DAEMON_DEFAULT_REPORT_PERIOD_US,
        .ipc_host = SWBT_DAEMON_DEFAULT_IPC_HOST,
        .ipc_port = SWBT_DAEMON_DEFAULT_IPC_PORT,
        .ipc_backlog = SWBT_DAEMON_DEFAULT_IPC_BACKLOG,
        .ipc_heartbeat_timeout_ms = SWBT_DAEMON_DEFAULT_IPC_HEARTBEAT_TIMEOUT_MS,
        .report_options =
            {
                .battery_connection = 0x8Eu,
                .vibrator_report = 0x80u,
            },
    };
    config.device_info = swbt_switch_device_info_default();
    return config;
}

bool swbt_daemon_config_apply_device_info_profile(swbt_daemon_config_t *config,
                                                  const char *profile) {
    if (config == NULL) {
        return false;
    }
    if (profile == NULL || profile[0] == '\0') {
        return true;
    }
    if (strcmp(profile, "default") == 0) {
        config->device_info = swbt_switch_device_info_default();
        return true;
    }
    if (strcmp(profile, "mizuyoukanao-pro") == 0) {
        config->device_info = swbt_switch_device_info_mizuyoukanao_pro();
        return true;
    }
    return false;
}
