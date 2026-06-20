#include "daemon/config.h"

swbt_daemon_config_t swbt_daemon_config_default(void) {
    swbt_daemon_config_t config = {
        .report_period_us = SWBT_DAEMON_DEFAULT_REPORT_PERIOD_US,
        .report_options = {0},
    };
    return config;
}
