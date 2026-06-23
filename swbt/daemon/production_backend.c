#include "daemon/production_backend.h"

#include <stddef.h>
#include <string.h>

#include "core/diagnostics.h"
#include "btstack_bridge/hid_event.h"

static swbt_daemon_production_backend_t *g_active_backend;

static void swbt_daemon_production_finish_shutdown(swbt_daemon_production_backend_t *backend);
static void swbt_daemon_production_shutdown_on_main_thread(void *context);

static bool
swbt_daemon_production_adapter_is_valid(const swbt_btstack_production_adapter_t *adapter) {
    return adapter != NULL && adapter->ipc_pump_start != NULL && adapter->ipc_pump_stop != NULL &&
           adapter->platform_start != NULL && adapter->platform_stop != NULL &&
           adapter->hid_register != NULL && adapter->hid_stop != NULL &&
           adapter->output_handler_start != NULL && adapter->output_handler_stop != NULL &&
           adapter->report_timer_init != NULL && adapter->report_timer_start != NULL &&
           adapter->report_timer_on_can_send_now != NULL &&
           adapter->report_timer_enqueue_subcommand_reply != NULL &&
           adapter->report_timer_stop != NULL && adapter->report_timer_send_neutral_now != NULL &&
           adapter->ssp_confirm_user_confirmation != NULL && adapter->time_ms != NULL &&
           adapter->read_controller_address != NULL && adapter->power_on != NULL &&
           adapter->power_off != NULL && adapter->run_loop_execute != NULL &&
           adapter->run_loop_execute_on_main_thread != NULL &&
           adapter->run_loop_trigger_exit != NULL;
}

static swbt_btstack_input_report_timer_adapter_config_t
swbt_daemon_production_timer_config(swbt_daemon_production_backend_t *backend,
                                    swbt_daemon_host_state_provider_t state_provider,
                                    void *state_context) {
    return (swbt_btstack_input_report_timer_adapter_config_t){
        .backend = NULL,
        .state_provider = state_provider,
        .state_context = state_context,
        .scheduler_config =
            {
                .report_period_us = backend->config.report_period_us,
                .report_options = backend->config.report_options,
            },
    };
}

swbt_daemon_production_result_t swbt_daemon_production_backend_init(
    swbt_daemon_production_backend_t *backend, const swbt_daemon_config_t *config,
    const swbt_btstack_production_adapter_t *adapter, void *adapter_context) {
    if (backend == NULL || config == NULL || config->report_period_us == 0u ||
        !swbt_daemon_production_adapter_is_valid(adapter)) {
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }

    *backend = (swbt_daemon_production_backend_t){0};
    backend->config = *config;
    backend->adapter = adapter;
    backend->adapter_context = adapter_context;
    backend->ipc_runner.config = swbt_daemon_ipc_runner_config_from_daemon_config(config);
    if (swbt_daemon_ipc_runner_init(&backend->ipc_runner) != SWBT_DAEMON_IPC_RUNNER_OK) {
        return SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
    }
    backend->ipc_runner.config = swbt_daemon_ipc_runner_config_from_daemon_config(config);
    backend->initialized = true;
    return SWBT_DAEMON_PRODUCTION_OK;
}

static bool swbt_daemon_production_ipc_runner_is_running(void *context) {
    return swbt_daemon_ipc_runner_is_running((const swbt_daemon_ipc_runner_t *)context);
}

static void swbt_daemon_production_ipc_runner_poll_once_at(void *context, uint32_t now_ms) {
    (void)swbt_daemon_ipc_runner_poll_once_at((swbt_daemon_ipc_runner_t *)context, now_ms);
}

static int swbt_daemon_production_ipc_start(void *context, swbt_app_t *app) {
    swbt_daemon_production_backend_t *backend = context;
    swbt_btstack_production_ipc_pump_t pump;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }
    if (swbt_daemon_ipc_runner_start(&backend->ipc_runner, app, &backend->ipc_runner.config) !=
        SWBT_DAEMON_IPC_RUNNER_OK) {
        return -1;
    }

    pump = (swbt_btstack_production_ipc_pump_t){
        .is_running = swbt_daemon_production_ipc_runner_is_running,
        .poll_once_at = swbt_daemon_production_ipc_runner_poll_once_at,
        .context = &backend->ipc_runner,
    };
    if (backend->adapter->ipc_pump_start(backend->adapter_context, &pump) != 0) {
        swbt_daemon_ipc_runner_stop(&backend->ipc_runner);
        return -1;
    }
    return 0;
}

static void swbt_daemon_production_ipc_stop(void *context) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    backend->adapter->ipc_pump_stop(backend->adapter_context);
    swbt_daemon_ipc_runner_stop(&backend->ipc_runner);
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static void swbt_daemon_production_hid_packet_handler(uint8_t packet_type, uint16_t channel,
                                                      uint8_t *packet, uint16_t size) {
    swbt_daemon_production_backend_t *backend = g_active_backend;
    swbt_btstack_hid_event_t event;
    (void)channel;

    if (backend == NULL || swbt_btstack_hid_event_decode(packet_type, packet, size, &event) !=
                               SWBT_BTSTACK_HID_EVENT_OK) {
        return;
    }

    switch (event.type) {
    case SWBT_BTSTACK_HID_EVENT_USER_CONFIRMATION_REQUEST:
        (void)backend->adapter->ssp_confirm_user_confirmation(backend->adapter_context,
                                                              event.address);
        break;
    case SWBT_BTSTACK_HID_EVENT_CONNECTION_OPENED:
        if (!backend->report_timer_initialized || event.status != 0u) {
            return;
        }
        swbt_diagnostic_trace("production: hid connection opened");
        (void)backend->adapter->report_timer_start(
            backend->adapter_context, &backend->report_timer,
            (swbt_btstack_input_report_timer_start_options_t){
                .hid_cid = event.hid_cid,
                .now_us = (uint64_t)backend->adapter->time_ms(backend->adapter_context) * 1000u,
            });
        break;
    case SWBT_BTSTACK_HID_EVENT_CAN_SEND_NOW: {
        if (!backend->report_timer_initialized) {
            return;
        }
        const int can_send_result = backend->adapter->report_timer_on_can_send_now(
            backend->adapter_context, &backend->report_timer);
        if (backend->shutdown_neutral_pending && can_send_result == 0) {
            swbt_diagnostic_trace("production: shutdown neutral pending sent");
            backend->shutdown_neutral_pending = false;
            swbt_daemon_production_finish_shutdown(backend);
        } else if (backend->shutdown_neutral_pending && can_send_result != 0) {
            swbt_diagnostic_trace("production: shutdown neutral pending failed");
            backend->shutdown_neutral_pending = false;
            swbt_daemon_production_finish_shutdown(backend);
        }
        break;
    }
    case SWBT_BTSTACK_HID_EVENT_CONNECTION_CLOSED:
        if (!backend->report_timer_initialized) {
            return;
        }
        swbt_diagnostic_trace("production: hid connection closed");
        backend->adapter->report_timer_stop(backend->adapter_context, &backend->report_timer);
        if (backend->shutdown_neutral_pending) {
            swbt_diagnostic_trace("production: shutdown neutral pending connection closed");
            backend->shutdown_neutral_pending = false;
            swbt_daemon_production_finish_shutdown(backend);
        }
        break;
    case SWBT_BTSTACK_HID_EVENT_NONE:
    default:
        break;
    }
}
// NOLINTEND(bugprone-easily-swappable-parameters)

static int swbt_daemon_production_hid_register(void *context) {
    swbt_daemon_production_backend_t *backend = context;
    swbt_btstack_hid_registration_config_t config;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }
    swbt_diagnostic_trace("production: hid register enter");
    if (!backend->platform_started) {
        swbt_diagnostic_trace("production: platform start");
        if (backend->adapter->platform_start(backend->adapter_context) != 0) {
            swbt_diagnostic_trace("production: platform start failed");
            return -1;
        }
        backend->platform_started = true;
        swbt_diagnostic_trace("production: platform start ok");
    }

    config = swbt_btstack_production_hid_registration_config();
    config.packet_handler = swbt_daemon_production_hid_packet_handler;
    g_active_backend = backend;
    swbt_diagnostic_trace("production: hid register btstack");
    if (backend->adapter->hid_register(backend->adapter_context, backend->hid_service_buffer,
                                       sizeof(backend->hid_service_buffer), &config) != 0) {
        swbt_diagnostic_trace("production: hid register failed");
        if (g_active_backend == backend) {
            g_active_backend = NULL;
        }
        if (backend->platform_started) {
            swbt_diagnostic_trace("production: platform stop after hid failure");
            backend->adapter->platform_stop(backend->adapter_context);
            backend->platform_started = false;
            swbt_diagnostic_trace("production: platform stop after hid failure done");
        }
        return -1;
    }
    backend->hid_registered = true;
    swbt_diagnostic_trace("production: hid register ok");
    return 0;
}

static void swbt_daemon_production_hid_stop(void *context) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    if (backend->hid_registered) {
        swbt_diagnostic_trace("production: hid stop");
        backend->adapter->hid_stop(backend->adapter_context);
        backend->hid_registered = false;
    }
    if (g_active_backend == backend) {
        g_active_backend = NULL;
    }
    if (backend->platform_started) {
        swbt_diagnostic_trace("production: platform stop");
        backend->adapter->platform_stop(backend->adapter_context);
        backend->platform_started = false;
        swbt_diagnostic_trace("production: platform stop done");
    }
}

static void
swbt_daemon_production_output_handler_start(void *context,
                                            swbt_btstack_output_report_handler_t *handler) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    backend->adapter->output_handler_start(backend->adapter_context, handler);
}

static void swbt_daemon_production_output_handler_stop(void *context) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    backend->adapter->output_handler_stop(backend->adapter_context);
}

static int swbt_daemon_production_report_timer_start(
    void *context, swbt_daemon_host_state_provider_t state_provider, void *state_context) {
    swbt_daemon_production_backend_t *backend = context;
    swbt_btstack_input_report_timer_adapter_config_t config;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }

    config = swbt_daemon_production_timer_config(backend, state_provider, state_context);
    if (backend->adapter->report_timer_init(backend->adapter_context, &backend->report_timer,
                                            &config) != 0) {
        return -1;
    }
    backend->report_timer_initialized = true;
    return 0;
}

static void swbt_daemon_production_report_timer_stop(void *context) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized || !backend->report_timer_initialized) {
        return;
    }
    backend->adapter->report_timer_stop(backend->adapter_context, &backend->report_timer);
    backend->report_timer_initialized = false;
}

static int swbt_daemon_production_report_timer_send_neutral_now(void *context) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        swbt_diagnostic_trace("production: neutral send adapter unavailable");
        return -1;
    }
    if (!backend->report_timer_initialized) {
        swbt_diagnostic_trace("production: neutral send timer uninitialized");
        return -1;
    }
    if (!backend->report_timer.running) {
        swbt_diagnostic_trace("production: neutral send timer stopped");
        return -1;
    }
    const int result = backend->adapter->report_timer_send_neutral_now(backend->adapter_context,
                                                                       &backend->report_timer);
    if (result == 0) {
        swbt_diagnostic_trace("production: neutral send adapter ok");
    } else if (result > 0) {
        swbt_diagnostic_trace("production: neutral send adapter pending");
    } else {
        swbt_diagnostic_trace("production: neutral send adapter error");
    }
    return result;
}

static int swbt_daemon_production_subcommand_reply_enqueue(void *context, uint16_t hid_cid,
                                                           const uint8_t *report,
                                                           size_t report_size) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->report_timer_initialized) {
        return -1;
    }
    return backend->adapter->report_timer_enqueue_subcommand_reply(
        backend->adapter_context, &backend->report_timer, hid_cid, report, report_size);
}

static int swbt_daemon_production_read_device_info(void *context,
                                                   swbt_switch_device_info_t *out_device_info) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || out_device_info == NULL) {
        return -1;
    }

    *out_device_info = backend->config.device_info;
    return backend->adapter->read_controller_address(backend->adapter_context,
                                                     out_device_info->bluetooth_address);
}

static uint32_t swbt_daemon_production_host_time_ms(void *context) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL) {
        return 0u;
    }

    return backend->adapter->time_ms(backend->adapter_context);
}

const swbt_daemon_host_backend_t *swbt_daemon_production_host_backend(void) {
    static const swbt_daemon_host_backend_t backend = {
        .daemon_backend = SWBT_APP_DAEMON_BACKEND_PRODUCTION,
        .ipc_start = swbt_daemon_production_ipc_start,
        .ipc_stop = swbt_daemon_production_ipc_stop,
        .hid_register = swbt_daemon_production_hid_register,
        .hid_stop = swbt_daemon_production_hid_stop,
        .output_handler_start = swbt_daemon_production_output_handler_start,
        .output_handler_stop = swbt_daemon_production_output_handler_stop,
        .report_timer_start = swbt_daemon_production_report_timer_start,
        .report_timer_stop = swbt_daemon_production_report_timer_stop,
        .report_timer_send_neutral_now = swbt_daemon_production_report_timer_send_neutral_now,
        .subcommand_reply_enqueue = swbt_daemon_production_subcommand_reply_enqueue,
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
swbt_daemon_production_power_on(swbt_daemon_production_backend_t *backend) {
    if (backend->adapter->power_on(backend->adapter_context) != 0) {
        return SWBT_DAEMON_PRODUCTION_ERROR_HARDWARE;
    }
    atomic_store(&backend->hardware_powered, true);
    return SWBT_DAEMON_PRODUCTION_OK;
}

static void swbt_daemon_production_power_off(swbt_daemon_production_backend_t *backend) {
    if (backend != NULL && atomic_exchange(&backend->hardware_powered, false)) {
        backend->adapter->power_off(backend->adapter_context);
    }
}

static void swbt_daemon_production_finish_shutdown(swbt_daemon_production_backend_t *backend) {
    if (backend == NULL || !backend->initialized) {
        return;
    }
    swbt_daemon_production_power_off(backend);
    backend->adapter->run_loop_trigger_exit(backend->adapter_context);
}

static bool
swbt_daemon_shutdown_listener_is_valid(const swbt_daemon_shutdown_listener_t *shutdown_listener) {
    return shutdown_listener == NULL ||
           (shutdown_listener->install != NULL && shutdown_listener->uninstall != NULL);
}

static void swbt_daemon_production_shutdown_on_main_thread(void *context) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }

    if (backend->host != NULL) {
        swbt_diagnostic_trace("production: shutdown neutral send");
        const swbt_daemon_host_result_t neutral_result =
            swbt_daemon_host_send_neutral_now(backend->host);
        if (neutral_result == SWBT_DAEMON_HOST_OK) {
            swbt_diagnostic_trace("production: shutdown neutral send ok");
            swbt_daemon_production_finish_shutdown(backend);
        } else if (neutral_result == SWBT_DAEMON_HOST_PENDING) {
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
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized ||
        atomic_exchange(&backend->shutdown_requested, true)) {
        return;
    }

    swbt_diagnostic_trace("production: shutdown requested");
    backend->shutdown_callback = (btstack_context_callback_registration_t){
        .callback = swbt_daemon_production_shutdown_on_main_thread,
        .context = backend,
    };
    backend->adapter->run_loop_execute_on_main_thread(backend->adapter_context,
                                                      &backend->shutdown_callback);
}

swbt_daemon_production_result_t swbt_daemon_production_main_with_backend_and_shutdown(
    swbt_daemon_production_backend_t *backend, const swbt_daemon_hardware_approval_t *approval,
    const swbt_daemon_shutdown_listener_t *shutdown_listener, void *shutdown_context) {
    swbt_daemon_host_t host;
    swbt_daemon_host_result_t host_result;
    swbt_daemon_production_result_t result = SWBT_DAEMON_PRODUCTION_OK;
    bool shutdown_listener_installed = false;

    if (backend == NULL || !backend->initialized ||
        !swbt_daemon_shutdown_listener_is_valid(shutdown_listener)) {
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }
    if (!swbt_daemon_hardware_approval_is_granted(approval)) {
        return SWBT_DAEMON_PRODUCTION_ERROR_HARDWARE_APPROVAL_REQUIRED;
    }

    swbt_diagnostic_trace("production: host init");
    host_result = swbt_daemon_host_init(&host, &backend->config,
                                        swbt_daemon_production_host_backend(), backend);
    if (host_result != SWBT_DAEMON_HOST_OK) {
        swbt_diagnostic_trace("production: host init failed");
        return SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
    }
    if (swbt_app_set_hardware_approval(swbt_daemon_host_app(&host),
                                       SWBT_APP_HARDWARE_APPROVAL_APPROVED) != SWBT_APP_OK) {
        swbt_daemon_host_destroy(&host);
        return SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
    }
    swbt_diagnostic_trace("production: host start");
    host_result = swbt_daemon_host_start(&host);
    if (host_result != SWBT_DAEMON_HOST_OK) {
        swbt_diagnostic_trace("production: host start failed");
        swbt_daemon_host_destroy(&host);
        return SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
    }
    swbt_diagnostic_trace("production: host start ok");
    backend->host = &host;

    swbt_diagnostic_trace("production: power on");
    result = swbt_daemon_production_power_on(backend);
    if (result == SWBT_DAEMON_PRODUCTION_OK) {
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
            backend->adapter->run_loop_execute(backend->adapter_context);
            swbt_diagnostic_trace("production: run loop returned");
        }
        if (shutdown_listener_installed) {
            shutdown_listener->uninstall(shutdown_context);
        }
    }

    swbt_diagnostic_trace("production: power off cleanup");
    swbt_daemon_production_power_off(backend);
    swbt_diagnostic_trace("production: host stop");
    swbt_daemon_host_destroy(&host);
    backend->host = NULL;
    swbt_diagnostic_trace("production: host stop done");
    return result;
}

swbt_daemon_production_result_t
swbt_daemon_production_main_with_backend(swbt_daemon_production_backend_t *backend,
                                         const swbt_daemon_hardware_approval_t *approval) {
    return swbt_daemon_production_main_with_backend_and_shutdown(backend, approval, NULL, NULL);
}

uint32_t
swbt_daemon_production_backend_report_period_us(const swbt_daemon_production_backend_t *backend) {
    return backend == NULL ? 0u : backend->config.report_period_us;
}

swbt_daemon_ipc_runner_config_t
swbt_daemon_production_backend_ipc_config(const swbt_daemon_production_backend_t *backend) {
    if (backend == NULL) {
        return (swbt_daemon_ipc_runner_config_t){0};
    }
    return backend->ipc_runner.config;
}
