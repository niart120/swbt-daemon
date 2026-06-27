#ifndef SWBT_DAEMON_CLI_H
#define SWBT_DAEMON_CLI_H

#include <stdbool.h>
#include <stdio.h>

#include "daemon/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*swbt_daemon_cli_list_adapters_t)(void *context, FILE *out, FILE *err);

typedef struct {
    swbt_daemon_cli_list_adapters_t list_adapters;
    void *adapter_inventory_context;
    const swbt_daemon_config_env_t *config_env;
} swbt_daemon_cli_ports_t;

typedef struct {
    bool handled;
    int exit_code;
} swbt_daemon_cli_dispatch_result_t;

swbt_daemon_cli_dispatch_result_t swbt_daemon_cli_dispatch(int argc, const char *const argv[],
                                                           FILE *out, FILE *err);

swbt_daemon_cli_dispatch_result_t
swbt_daemon_cli_dispatch_with_ports(int argc, const char *const argv[], FILE *out, FILE *err,
                                    const swbt_daemon_cli_ports_t *ports);

#ifdef __cplusplus
}
#endif

#endif
