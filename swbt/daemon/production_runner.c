#include "daemon/production_runner.h"

#include <stddef.h>
#include <string.h>

#include "support/diagnostics.h"
#include "daemon/production_hid_session.h"
#include "daemon/production_ipc_pump.h"
#include "daemon/production_report_timer.h"
#include "daemon/production_reconnect.h"

static void swbt_daemon_production_finish_shutdown(swbt_daemon_production_runner_t *backend);
static void swbt_daemon_production_hid_session_finish_shutdown(void *context);
static void swbt_daemon_production_shutdown_on_main_thread(void *context);

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

static int swbt_daemon_production_ipc_start(void *context, swbt_control_t *control) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_production_ipc_pump_t ipc_pump;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }
    ipc_pump = (swbt_daemon_production_ipc_pump_t){
        .runner = &backend->ipc_runner,
        .port = &backend->ports->ipc_pump,
        .port_context = backend->ports_context,
    };
    return swbt_daemon_production_ipc_pump_start(&ipc_pump, control);
}

static void swbt_daemon_production_ipc_stop(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_production_ipc_pump_t ipc_pump;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    ipc_pump = (swbt_daemon_production_ipc_pump_t){
        .runner = &backend->ipc_runner,
        .port = &backend->ports->ipc_pump,
        .port_context = backend->ports_context,
    };
    swbt_daemon_production_ipc_pump_stop(&ipc_pump);
}

static swbt_daemon_production_report_timer_t *
swbt_daemon_production_report_timer_from_backend(swbt_daemon_production_runner_t *backend) {
    backend->report_timer_bridge = (swbt_daemon_production_report_timer_t){
        .config = &backend->config,
        .port = &backend->ports->report_timer,
        .port_context = backend->ports_context,
        .adapter = &backend->report_timer,
        .device = &backend->device,
        .host = &backend->host,
        .initialized = &backend->report_timer_initialized,
    };
    return &backend->report_timer_bridge;
}

static swbt_daemon_production_hid_session_t *
swbt_daemon_production_hid_session_from_backend(swbt_daemon_production_runner_t *backend) {
    backend->hid_session_bridge = (swbt_daemon_production_hid_session_t){
        .config = &backend->config,
        .device_port = &backend->ports->device,
        .report_timer_port = &backend->ports->report_timer,
        .controller_port = &backend->ports->controller,
        .clock_port = &backend->ports->clock,
        .port_context = backend->ports_context,
        .device = &backend->device,
        .report_timer = &backend->report_timer,
        .report_timer_initialized = &backend->report_timer_initialized,
        .shutdown_neutral_pending = &backend->shutdown_neutral_pending,
        .learned_switch_address_target = &backend->learned_switch_address_target,
        .learned_switch_address_target_configured =
            &backend->learned_switch_address_target_configured,
        .service_buffer = backend->hid_service_buffer,
        .service_buffer_size = sizeof(backend->hid_service_buffer),
        .finish_shutdown = swbt_daemon_production_hid_session_finish_shutdown,
        .finish_shutdown_context = backend,
    };
    return &backend->hid_session_bridge;
}

static int swbt_daemon_production_hid_register(void *context) {
    swbt_daemon_production_runner_t *backend = context;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }
    return swbt_daemon_production_hid_session_register(
        swbt_daemon_production_hid_session_from_backend(backend));
}

static void swbt_daemon_production_hid_stop(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    swbt_daemon_production_hid_session_stop(
        swbt_daemon_production_hid_session_from_backend(backend));
}

static void
swbt_daemon_production_output_handler_start(void *context,
                                            swbt_btstack_output_report_handler_t *handler) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    backend->ports->output_handler.start(backend->ports_context, handler);
}

static void swbt_daemon_production_output_handler_stop(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    backend->ports->output_handler.stop(backend->ports_context);
}

static int swbt_daemon_production_process_report_timer_start(
    void *context, swbt_daemon_process_state_provider_t state_provider, void *state_context) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_production_report_timer_t *timer;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }

    timer = swbt_daemon_production_report_timer_from_backend(backend);
    return swbt_daemon_production_report_timer_start(timer, state_provider, state_context);
}

static void swbt_daemon_production_process_report_timer_stop(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_production_report_timer_t *timer;
    if (backend == NULL || !backend->initialized || !backend->report_timer_initialized) {
        return;
    }
    timer = swbt_daemon_production_report_timer_from_backend(backend);
    swbt_daemon_production_report_timer_stop(timer);
}

static int swbt_daemon_production_process_report_timer_send_neutral_now(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_production_report_timer_t *timer;
    if (backend == NULL || !backend->initialized) {
        return -1;
    }
    timer = swbt_daemon_production_report_timer_from_backend(backend);
    return swbt_daemon_production_report_timer_send_neutral_now(timer);
}

static int swbt_daemon_production_process_subcommand_reply_enqueue(void *context, uint16_t hid_cid,
                                                                   const uint8_t *report,
                                                                   size_t report_size) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_production_report_timer_t *timer;
    if (backend == NULL || !backend->report_timer_initialized) {
        return -1;
    }
    timer = swbt_daemon_production_report_timer_from_backend(backend);
    return swbt_daemon_production_report_timer_enqueue_subcommand_reply(timer, hid_cid, report,
                                                                        report_size);
}

static int swbt_daemon_production_read_device_info(void *context,
                                                   swbt_switch_device_info_t *out_device_info) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || out_device_info == NULL) {
        return -1;
    }

    *out_device_info = backend->config.device_info;
    return backend->ports->controller.read_controller_address(backend->ports_context,
                                                              out_device_info->bluetooth_address);
}

static uint32_t swbt_daemon_production_host_time_ms(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL) {
        return 0u;
    }

    return backend->ports->clock.time_ms(backend->ports_context);
}

const swbt_daemon_process_backend_t *swbt_daemon_production_process_backend(void) {
    static const swbt_daemon_process_backend_t backend = {
        .daemon_backend = SWBT_DOMAIN_DAEMON_BACKEND_PRODUCTION,
        .ipc_start = swbt_daemon_production_ipc_start,
        .ipc_stop = swbt_daemon_production_ipc_stop,
        .hid_register = swbt_daemon_production_hid_register,
        .hid_stop = swbt_daemon_production_hid_stop,
        .output_handler_start = swbt_daemon_production_output_handler_start,
        .output_handler_stop = swbt_daemon_production_output_handler_stop,
        .report_timer_start = swbt_daemon_production_process_report_timer_start,
        .report_timer_stop = swbt_daemon_production_process_report_timer_stop,
        .report_timer_send_neutral_now =
            swbt_daemon_production_process_report_timer_send_neutral_now,
        .subcommand_reply_enqueue = swbt_daemon_production_process_subcommand_reply_enqueue,
        .read_device_info = swbt_daemon_production_read_device_info,
        .time_ms = swbt_daemon_production_host_time_ms,
    };
    return &backend;
}

bool swbt_daemon_hardware_approval_is_granted(const swbt_daemon_hardware_approval_t *approval) {
    return approval != NULL && approval->run_hardware && approval->hardware_approved;
}

static bool swbt_daemon_hardware_approval_env_is_enabled(const char *value) {
    return value != NULL && strcmp(value, "1") == 0;
}

swbt_daemon_hardware_approval_t
swbt_daemon_hardware_approval_from_env(const swbt_daemon_hardware_approval_env_t *env) {
    if (env == NULL) {
        return (swbt_daemon_hardware_approval_t){0};
    }

    return (swbt_daemon_hardware_approval_t){
        .run_hardware = swbt_daemon_hardware_approval_env_is_enabled(env->run_hardware),
        .hardware_approved = swbt_daemon_hardware_approval_env_is_enabled(env->hardware_approved),
    };
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

static void swbt_daemon_production_finish_shutdown(swbt_daemon_production_runner_t *backend) {
    if (backend == NULL || !backend->initialized) {
        return;
    }
    swbt_daemon_production_power_off(backend);
    backend->ports->run_loop.trigger_exit(backend->ports_context);
}

static void swbt_daemon_production_hid_session_finish_shutdown(void *context) {
    swbt_daemon_production_finish_shutdown(context);
}

static bool
swbt_daemon_shutdown_listener_is_valid(const swbt_daemon_shutdown_listener_t *shutdown_listener) {
    return shutdown_listener == NULL ||
           (shutdown_listener->install != NULL && shutdown_listener->uninstall != NULL);
}

static void swbt_daemon_production_shutdown_on_main_thread(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }

    if (backend->host != NULL) {
        swbt_diagnostic_trace("production: shutdown neutral send");
        const swbt_daemon_process_result_t neutral_result =
            swbt_daemon_process_send_neutral_now(backend->host);
        if (neutral_result == SWBT_DAEMON_PROCESS_OK) {
            swbt_diagnostic_trace("production: shutdown neutral send ok");
            swbt_daemon_production_finish_shutdown(backend);
        } else if (neutral_result == SWBT_DAEMON_PROCESS_PENDING) {
            swbt_diagnostic_trace("production: shutdown neutral send pending");
            backend->shutdown_neutral_pending = true;
        } else {
            swbt_diagnostic_trace("production: shutdown neutral send failed");
            swbt_daemon_production_finish_shutdown(backend);
        }
    } else {
        swbt_daemon_production_finish_shutdown(backend);
    }
}

static void swbt_daemon_production_request_shutdown(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->initialized ||
        atomic_exchange(&backend->shutdown_requested, true)) {
        return;
    }

    swbt_diagnostic_trace("production: shutdown requested");
    backend->shutdown_callback = (btstack_context_callback_registration_t){
        .callback = swbt_daemon_production_shutdown_on_main_thread,
        .context = backend,
    };
    backend->ports->run_loop.execute_on_main_thread(backend->ports_context,
                                                    &backend->shutdown_callback);
}

swbt_daemon_production_result_t swbt_daemon_production_main_with_runner_and_shutdown(
    swbt_daemon_production_runner_t *backend, const swbt_daemon_hardware_approval_t *approval,
    const swbt_daemon_shutdown_listener_t *shutdown_listener, void *shutdown_context) {
    swbt_daemon_process_t host;
    swbt_daemon_process_result_t host_result;
    swbt_daemon_production_result_t result = SWBT_DAEMON_PRODUCTION_OK;
    bool shutdown_listener_installed = false;

    if (backend == NULL || !backend->initialized ||
        !swbt_daemon_shutdown_listener_is_valid(shutdown_listener)) {
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }
    (void)approval;
    if (!swbt_btstack_production_ports_is_valid(backend->ports)) {
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }
    if (!backend->adapter_location_configured) {
        swbt_diagnostic_trace("production: adapter location required");
        return SWBT_DAEMON_PRODUCTION_ERROR_ADAPTER_LOCATION_REQUIRED;
    }

    swbt_diagnostic_trace("production: host init");
    host_result = swbt_daemon_process_init(&host, &backend->config,
                                           swbt_daemon_production_process_backend(), backend);
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
        swbt_daemon_production_reconnect_request_active(&(swbt_daemon_production_reconnect_t){
            .config = &backend->config,
            .device = &backend->device,
            .app = swbt_daemon_process_app(&host),
        });
        atomic_store(&backend->shutdown_requested, false);
        backend->shutdown_neutral_pending = false;
        if (shutdown_listener != NULL) {
            if (shutdown_listener->install(shutdown_context,
                                           swbt_daemon_production_request_shutdown, backend) != 0) {
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
            shutdown_listener->uninstall(shutdown_context);
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
swbt_daemon_production_main_with_runner(swbt_daemon_production_runner_t *backend,
                                        const swbt_daemon_hardware_approval_t *approval) {
    return swbt_daemon_production_main_with_runner_and_shutdown(backend, approval, NULL, NULL);
}

uint32_t
swbt_daemon_production_runner_report_period_us(const swbt_daemon_production_runner_t *backend) {
    return backend == NULL ? 0u : backend->config.report_period_us;
}

swbt_daemon_ipc_runner_config_t
swbt_daemon_production_runner_ipc_config(const swbt_daemon_production_runner_t *backend) {
    if (backend == NULL) {
        return (swbt_daemon_ipc_runner_config_t){0};
    }
    return backend->ipc_runner.config;
}
