#include <stdio.h>
#include <stdlib.h>

#include "platform_process.h"
#include "production_entrypoint.h"

#include "support/diagnostics.h"
#include "daemon/cli.h"
#include "daemon/config.h"
#include "daemon/process.h"
#include "daemon/launch_options.h"

static swbt_daemon_config_env_t swbt_daemon_config_env_from_process_env(void) {
    return (swbt_daemon_config_env_t){
        .report_period_us = getenv("SWBT_REPORT_PERIOD_US"),
        .ipc_host = getenv("SWBT_IPC_HOST"),
        .ipc_port = getenv("SWBT_IPC_PORT"),
        .ipc_backlog = getenv("SWBT_IPC_BACKLOG"),
        .ipc_heartbeat_timeout_ms = getenv("SWBT_IPC_HEARTBEAT_TIMEOUT_MS"),
        .device_info_profile = getenv("SWBT_DEVICE_INFO_PROFILE"),
    };
}

int main(int argc, char **argv) {
    swbt_daemon_cli_dispatch_result_t cli_result;
    swbt_daemon_launch_config_t launch_config;
    swbt_daemon_launch_options_t launch_options;
    const swbt_daemon_config_env_t config_env = swbt_daemon_config_env_from_process_env();
    const swbt_daemon_cli_ports_t cli_ports = {
        .list_adapters = swbt_daemon_production_entrypoint_list_adapter_locations,
        .config_env = &config_env,
    };

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    cli_result = swbt_daemon_cli_dispatch_with_ports(argc, (const char *const *)argv, stdout,
                                                     stderr, &cli_ports);
    if (cli_result.handled) {
        return cli_result.exit_code;
    }
    swbt_diagnostic_trace("main: entered");
    if (swbt_daemon_launch_options_parse(&launch_options, argc, (const char *const *)argv) !=
        SWBT_DAEMON_LAUNCH_OPTIONS_OK) {
        swbt_diagnostic_trace("main: invalid CLI options");
        return 1;
    }
    swbt_daemon_platform_install_crash_dump_handler(launch_options.crash_dump_path);
    swbt_diagnostic_trace_set_path(launch_options.trace_path);
    if (!swbt_daemon_launch_config_prepare(&launch_config, &launch_options, &config_env)) {
        swbt_diagnostic_trace("main: invalid launch config");
        return 1;
    }
    switch (launch_options.backend) {
    case SWBT_DAEMON_LAUNCH_BACKEND_PRODUCTION:
        swbt_diagnostic_trace("main: selected production backend");
        return swbt_daemon_production_entrypoint_run(&launch_config);
    case SWBT_DAEMON_LAUNCH_BACKEND_NOOP:
        swbt_diagnostic_trace("main: selected noop backend");
        return swbt_daemon_main_with_process_backend(&launch_config.config,
                                                     swbt_daemon_process_noop_backend(), NULL);
    }
    swbt_diagnostic_trace("main: unknown backend");
    return 1;
}
