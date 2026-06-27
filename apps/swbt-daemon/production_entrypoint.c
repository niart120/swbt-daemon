#include "production_entrypoint.h"

#include "platform_process.h"

#include "btstack_bridge/production_btstack_impl.h"
#include "daemon/production_runner_internal.h"
#include "support/diagnostics.h"

int swbt_daemon_production_entrypoint_list_adapter_locations(void *context, FILE *out, FILE *err) {
    return swbt_btstack_production_list_adapter_locations(context, out, err);
}

int swbt_daemon_production_entrypoint_run(const swbt_daemon_launch_config_t *launch_config) {
    swbt_daemon_production_runner_t backend;

    if (swbt_btstack_production_link_key_db_configure(
            launch_config->link_key_db_configured ? launch_config->link_key_db_path : NULL) != 0) {
        swbt_diagnostic_trace("production: link key db path invalid");
        return 1;
    }
    if (swbt_btstack_production_hci_dump_configure(launch_config->hci_dump_path) != 0) {
        swbt_diagnostic_trace("production: hci dump path invalid");
        return 1;
    }
    if (launch_config->adapter_location_configured &&
        swbt_btstack_production_impl_configure_adapter_location(launch_config->adapter_location) !=
            0) {
        swbt_diagnostic_trace("production: adapter location invalid");
        return 1;
    }
    swbt_diagnostic_trace("production: backend init");
    if (swbt_daemon_production_runner_init(&backend, &launch_config->config,
                                           swbt_btstack_production_ports_btstack(),
                                           NULL) != SWBT_DAEMON_PRODUCTION_OK) {
        swbt_diagnostic_trace("production: backend init failed");
        return 1;
    }
    if (launch_config->adapter_location_configured &&
        !swbt_daemon_production_runner_set_adapter_location_configured(&backend)) {
        swbt_diagnostic_trace("production: adapter location state invalid");
        return 1;
    }
    if (launch_config->learned_switch_address_target_configured &&
        !swbt_daemon_production_runner_set_learned_switch_address_target(
            &backend, &launch_config->learned_switch_address_target)) {
        swbt_diagnostic_trace("production: learned switch address target invalid");
        return 1;
    }
    swbt_diagnostic_trace("production: enter main");
    return swbt_daemon_production_main_with_runner_and_shutdown(
               &backend, swbt_daemon_platform_shutdown_listener(), NULL) ==
                   SWBT_DAEMON_PRODUCTION_OK
               ? 0
               : 1;
}
