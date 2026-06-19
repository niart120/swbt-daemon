#ifndef SWBT_DAEMON_CONFIG_H
#define SWBT_DAEMON_CONFIG_H

#include <stdint.h>

#define SWBT_DAEMON_DEFAULT_REPORT_PERIOD_US 8000u

typedef struct {
    uint32_t report_period_us;
} swbt_daemon_config_t;

swbt_daemon_config_t swbt_daemon_config_default(void);

#endif
