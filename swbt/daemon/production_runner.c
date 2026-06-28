#include "daemon/production_runner_internal.h"

#include <stddef.h>

#include "support/diagnostics.h"
#include "daemon/btstack_process_backend.h"
#include "daemon/process_internal.h"
#include "daemon/active_reconnect.h"

swbt_daemon_production_result_t swbt_daemon_production_runner_init(
    swbt_daemon_production_runner_t *backend, const swbt_daemon_config_t *config,
    const swbt_btstack_production_ports_t *ports, void *ports_context) {
    if (backend == NULL || config == NULL || config->report_period_us == 0u ||
        !swbt_btstack_production_ports_has_ipc_pump(ports)) {
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }

    *backend = (swbt_daemon_production_runner_t){0};
    backend->config = *config;
    backend->ports = ports;
    backend->ports_context = ports_context;
    backend->ipc_runner.config = swbt_daemon_ipc_runner_config_from_daemon_config(config);
    if (swbt_daemon_ipc_runner_init(&backend->ipc_runner) != SWBT_DAEMON_IPC_RUNNER_OK) {
        return SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
    }
    backend->ipc_runner.config = swbt_daemon_ipc_runner_config_from_daemon_config(config);
    backend->initialized = true;
    return SWBT_DAEMON_PRODUCTION_OK;
}

bool swbt_daemon_production_runner_set_learned_switch_address_target(
    swbt_daemon_production_runner_t *backend, const swbt_daemon_config_file_target_t *target) {
    if (backend == NULL || target == NULL || target->path == NULL || target->path[0] == '\0') {
        return false;
    }
    backend->learned_switch_address_target = *target;
    backend->learned_switch_address_target_configured = true;
    return true;
}

bool swbt_daemon_production_runner_set_adapter_location_configured(
    swbt_daemon_production_runner_t *backend) {
    if (backend == NULL || !backend->initialized) {
        return false;
    }
    backend->adapter_location_configured = true;
    return true;
}

static swbt_daemon_production_result_t
swbt_daemon_production_power_on(swbt_daemon_production_runner_t *backend) {
    if (backend->ports->power.on(backend->ports_context) != 0) {
        return SWBT_DAEMON_PRODUCTION_ERROR_HARDWARE;
    }
    atomic_store(&backend->hardware_powered, true);
    return SWBT_DAEMON_PRODUCTION_OK;
}

static void swbt_daemon_production_power_off(swbt_daemon_production_runner_t *backend) {
    if (backend != NULL && atomic_exchange(&backend->hardware_powered, false)) {
        backend->ports->power.off(backend->ports_context);
    }
}

static void swbt_daemon_production_runner_finish_shutdown(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    swbt_daemon_production_power_off(backend);
    backend->ports->run_loop.trigger_exit(backend->ports_context);
}

swbt_daemon_production_result_t swbt_daemon_production_main_with_runner_and_shutdown(
    swbt_daemon_production_runner_t *backend,
    const swbt_daemon_shutdown_listener_t *shutdown_listener, void *shutdown_context) {
    swbt_daemon_process_t host;
    swbt_daemon_process_result_t host_result;
    swbt_daemon_production_result_t result = SWBT_DAEMON_PRODUCTION_OK;
    bool shutdown_listener_installed = false;

    if (backend == NULL || !backend->initialized ||
        !swbt_daemon_shutdown_sequence_listener_is_valid(shutdown_listener)) {
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }
    if (!swbt_btstack_production_ports_is_valid(backend->ports)) {
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }
    if (!backend->adapter_location_configured) {
        swbt_diagnostic_trace("production: adapter location required");
        return SWBT_DAEMON_PRODUCTION_ERROR_ADAPTER_LOCATION_REQUIRED;
    }
    if (!swbt_daemon_shutdown_sequence_init(
            &backend->shutdown, &(swbt_daemon_shutdown_sequence_config_t){
                                    .run_loop = &backend->ports->run_loop,
                                    .port_context = backend->ports_context,
                                    .host = &backend->host,
                                    .finish = swbt_daemon_production_runner_finish_shutdown,
                                    .finish_context = backend,
                                })) {
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }

    swbt_diagnostic_trace("production: host init");
    host_result = swbt_daemon_process_init(&host, &backend->config,
                                           swbt_daemon_btstack_process_backend(), backend);
    if (host_result != SWBT_DAEMON_PROCESS_OK) {
        swbt_diagnostic_trace("production: host init failed");
        return SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
    }
    if (swbt_domain_set_hardware_approval(swbt_daemon_process_app(&host),
                                          SWBT_DOMAIN_HARDWARE_APPROVAL_APPROVED) !=
        SWBT_DOMAIN_OK) {
        swbt_daemon_process_destroy(&host);
        return SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
    }
    swbt_diagnostic_trace("production: host start");
    host_result = swbt_daemon_process_start(&host);
    if (host_result != SWBT_DAEMON_PROCESS_OK) {
        swbt_diagnostic_trace("production: host start failed");
        swbt_daemon_process_destroy(&host);
        return SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
    }
    swbt_diagnostic_trace("production: host start ok");
    backend->host = &host;

    swbt_diagnostic_trace("production: power on");
    result = swbt_daemon_production_power_on(backend);
    if (result == SWBT_DAEMON_PRODUCTION_OK) {
        swbt_daemon_active_reconnect_request_active(&(swbt_daemon_active_reconnect_t){
            .config = &backend->config,
            .device = &backend->device,
            .app = swbt_daemon_process_app(&host),
        });
        swbt_daemon_shutdown_sequence_prepare(&backend->shutdown);
        if (shutdown_listener != NULL) {
            if (swbt_daemon_shutdown_sequence_install_listener(
                    &backend->shutdown, shutdown_listener, shutdown_context) != 0) {
                result = SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
            } else {
                shutdown_listener_installed = true;
            }
        }
        if (result == SWBT_DAEMON_PRODUCTION_OK) {
            swbt_diagnostic_trace("production: run loop execute");
            backend->ports->run_loop.execute(backend->ports_context);
            swbt_diagnostic_trace("production: run loop returned");
        }
        if (shutdown_listener_installed) {
            swbt_daemon_shutdown_sequence_uninstall_listener(shutdown_listener, shutdown_context);
        }
    }

    swbt_diagnostic_trace("production: power off cleanup");
    swbt_daemon_production_power_off(backend);
    swbt_diagnostic_trace("production: host stop");
    swbt_daemon_process_destroy(&host);
    backend->host = NULL;
    swbt_diagnostic_trace("production: host stop done");
    return result;
}

swbt_daemon_production_result_t
swbt_daemon_production_main_with_runner(swbt_daemon_production_runner_t *backend) {
    return swbt_daemon_production_main_with_runner_and_shutdown(backend, NULL, NULL);
}

swbt_daemon_production_result_t
swbt_daemon_production_run(const swbt_daemon_production_run_config_t *run_config) {
    swbt_daemon_production_runner_t backend;
    swbt_daemon_production_result_t result;

    if (run_config == NULL) {
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }

    swbt_diagnostic_trace("production: backend init");
    result = swbt_daemon_production_runner_init(&backend, run_config->config, run_config->ports,
                                                run_config->ports_context);
    if (result != SWBT_DAEMON_PRODUCTION_OK) {
        swbt_diagnostic_trace("production: backend init failed");
        return result;
    }
    if (run_config->adapter_location_configured &&
        !swbt_daemon_production_runner_set_adapter_location_configured(&backend)) {
        swbt_diagnostic_trace("production: adapter location state invalid");
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }
    if (run_config->learned_switch_address_target_configured &&
        !swbt_daemon_production_runner_set_learned_switch_address_target(
            &backend, &run_config->learned_switch_address_target)) {
        swbt_diagnostic_trace("production: learned switch address target invalid");
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }

    swbt_diagnostic_trace("production: enter main");
    return swbt_daemon_production_main_with_runner_and_shutdown(
        &backend, run_config->shutdown_listener, run_config->shutdown_context);
}
