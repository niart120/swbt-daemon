#ifndef SWBT_DAEMON_APP_PRODUCTION_ENTRYPOINT_H
#define SWBT_DAEMON_APP_PRODUCTION_ENTRYPOINT_H

#include <stdio.h>

#include "daemon/launch_options.h"

int swbt_daemon_production_entrypoint_list_adapter_locations(void *context, FILE *out, FILE *err);

int swbt_daemon_production_entrypoint_run(const swbt_daemon_launch_config_t *launch_config);

#endif
