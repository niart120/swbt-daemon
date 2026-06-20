#ifndef SWBT_DAEMON_CONFIG_H
#define SWBT_DAEMON_CONFIG_H

#include <stdint.h>

#include "switch/switch_report.h"

#define SWBT_DAEMON_DEFAULT_REPORT_PERIOD_US 8000u

typedef struct {
    uint32_t report_period_us;
    swbt_switch_report_options_t report_options;
} swbt_daemon_config_t;

swbt_daemon_config_t swbt_daemon_config_default(void);

#endif
