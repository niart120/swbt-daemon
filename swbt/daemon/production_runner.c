#include "daemon/production_runner.h"

#include <stddef.h>
#include <string.h>

#include "support/diagnostics.h"
#include "btstack_bridge/hid_event.h"
#include "daemon/production_reconnect.h"

static swbt_daemon_production_runner_t *g_active_backend;

static void swbt_daemon_production_finish_shutdown(swbt_daemon_production_runner_t *backend);
static void swbt_daemon_production_record_report_tick(
    void *context, uint64_t now_us,
    swbt_btstack_input_report_timer_report_send_result_t send_result);
static void swbt_daemon_production_shutdown_on_main_thread(void *context);

static char swbt_daemon_production_hex_digit_upper(uint8_t value) {
    return (char)(value < 10u ? (uint8_t)'0' + value : (uint8_t)'A' + (uint8_t)(value - 10u));
}

static void
swbt_daemon_production_format_switch_address(const uint8_t address[6],
                                             char text[SWBT_DAEMON_CONFIG_SWITCH_ADDRESS_SIZE]) {
    for (size_t index = 0u; index < 6u; ++index) {
        const size_t offset = index * 3u;
        text[offset] = swbt_daemon_production_hex_digit_upper((uint8_t)(address[index] >> 4u));
        text[offset + 1u] =
            swbt_daemon_production_hex_digit_upper((uint8_t)(address[index] & 0x0Fu));
        if (index < 5u) {
            text[offset + 2u] = ':';
        }
    }
    text[SWBT_DAEMON_CONFIG_SWITCH_ADDRESS_SIZE - 1u] = '\0';
}

static void
swbt_daemon_production_persist_learned_switch_address(swbt_daemon_production_runner_t *backend,
                                                      const uint8_t address[6]) {
    char address_text[SWBT_DAEMON_CONFIG_SWITCH_ADDRESS_SIZE];
    swbt_daemon_config_file_result_t result;

    if (backend == NULL || address == NULL || !backend->learned_switch_address_target_configured) {
        return;
    }

    swbt_daemon_production_format_switch_address(address, address_text);
    swbt_diagnostic_trace("production: learned switch address save");
    result = swbt_daemon_config_save_active_reconnect_learned_switch_address(
        &backend->config, &backend->learned_switch_address_target, address_text);
    swbt_diagnostic_trace(result == SWBT_DAEMON_CONFIG_FILE_OK
                              ? "production: learned switch address save ok"
                              : "production: learned switch address save failed");
}

static int swbt_daemon_production_report_timer_send(void *context, uint16_t hid_cid,
                                                    const uint8_t *message, size_t message_size) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return -1;
    }

    return swbt_btstack_device_send(&backend->device, hid_cid, message, message_size) ==
                   SWBT_BTSTACK_DEVICE_OK
               ? 0
               : -1;
}

static swbt_btstack_input_report_timer_adapter_config_t
swbt_daemon_production_timer_config(swbt_daemon_production_runner_t *backend,
                                    swbt_daemon_process_state_provider_t state_provider,
                                    void *state_context) {
    return (swbt_btstack_input_report_timer_adapter_config_t){
        .backend = NULL,
        .hid_sender = swbt_daemon_production_report_timer_send,
        .hid_sender_context = backend,
        .state_provider = state_provider,
        .state_context = state_context,
        .report_tick_observer = swbt_daemon_production_record_report_tick,
        .report_tick_context = backend,
        .scheduler_config =
            {
                .report_period_us = backend->config.report_period_us,
                .report_options = backend->config.report_options,
            },
    };
}

static swbt_metrics_report_send_result_t swbt_daemon_production_report_send_result(
    swbt_btstack_input_report_timer_report_send_result_t send_result) {
    switch (send_result) {
    case SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_OK:
        return SWBT_METRICS_REPORT_SEND_OK;
    case SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_FAILED:
        return SWBT_METRICS_REPORT_SEND_FAILED;
    }
    return SWBT_METRICS_REPORT_SEND_FAILED;
}

static void swbt_daemon_production_record_report_tick(
    void *context, uint64_t now_us,
    swbt_btstack_input_report_timer_report_send_result_t send_result) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_domain_t *app;

    if (backend == NULL || backend->host == NULL) {
        return;
    }
    app = swbt_daemon_process_app(backend->host);
    if (app == NULL) {
        return;
    }

    (void)swbt_domain_record_report_tick(app, now_us,
                                         swbt_daemon_production_report_send_result(send_result));
}

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

static bool swbt_daemon_production_ipc_runner_is_running(void *context) {
    return swbt_daemon_ipc_runner_is_running((const swbt_daemon_ipc_runner_t *)context);
}

static void swbt_daemon_production_ipc_runner_poll_once_at(void *context, uint32_t now_ms) {
    (void)swbt_daemon_ipc_runner_poll_once_at((swbt_daemon_ipc_runner_t *)context, now_ms);
}

static int swbt_daemon_production_ipc_start(void *context, swbt_control_t *control) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_btstack_production_ipc_pump_t pump;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }
    if (swbt_daemon_ipc_runner_start(&backend->ipc_runner, control, &backend->ipc_runner.config) !=
        SWBT_DAEMON_IPC_RUNNER_OK) {
        return -1;
    }

    pump = (swbt_btstack_production_ipc_pump_t){
        .is_running = swbt_daemon_production_ipc_runner_is_running,
        .poll_once_at = swbt_daemon_production_ipc_runner_poll_once_at,
        .context = &backend->ipc_runner,
    };
    if (backend->ports->ipc_pump.start(backend->ports_context, &pump) != 0) {
        swbt_daemon_ipc_runner_stop(&backend->ipc_runner);
        return -1;
    }
    return 0;
}

static void swbt_daemon_production_ipc_stop(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    backend->ports->ipc_pump.stop(backend->ports_context);
    swbt_daemon_ipc_runner_stop(&backend->ipc_runner);
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters): BTstack packet handler ABI.
static void swbt_daemon_production_hid_packet_handler(uint8_t packet_type, uint16_t channel,
                                                      uint8_t *packet, uint16_t size) {
    swbt_daemon_production_runner_t *backend = g_active_backend;
    swbt_btstack_hid_event_t event;
    (void)channel;

    if (backend == NULL || swbt_btstack_device_recv(&backend->device, packet_type, packet, size,
                                                    &event) != SWBT_BTSTACK_DEVICE_OK) {
        return;
    }

    switch (event.type) {
    case SWBT_BTSTACK_HID_EVENT_USER_CONFIRMATION_REQUEST:
        (void)backend->ports->controller.confirm_ssp_user_confirmation(backend->ports_context,
                                                                       event.address);
        break;
    case SWBT_BTSTACK_HID_EVENT_CONNECTION_OPENED:
        if (!backend->report_timer_initialized || event.status != 0u) {
            return;
        }
        swbt_diagnostic_trace("production: hid connection opened");
        swbt_daemon_production_persist_learned_switch_address(backend, event.address);
        (void)backend->ports->report_timer.start(
            backend->ports_context, &backend->report_timer,
            (swbt_btstack_input_report_timer_start_options_t){
                .hid_cid = event.hid_cid,
                .now_us = (uint64_t)backend->ports->clock.time_ms(backend->ports_context) * 1000u,
            });
        break;
    case SWBT_BTSTACK_HID_EVENT_CAN_SEND_NOW: {
        if (!backend->report_timer_initialized) {
            return;
        }
        const int can_send_result = backend->ports->report_timer.on_can_send_now(
            backend->ports_context, &backend->report_timer);
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
        backend->ports->report_timer.stop(backend->ports_context, &backend->report_timer);
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
    swbt_daemon_production_runner_t *backend = context;
    swbt_btstack_hid_registration_config_t config;
    swbt_btstack_device_result_t result;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }
    swbt_diagnostic_trace("production: hid register enter");

    config = swbt_btstack_production_hid_registration_config();
    config.packet_handler = swbt_daemon_production_hid_packet_handler;
    g_active_backend = backend;
    if (swbt_btstack_device_init(&backend->device, &backend->ports->device,
                                 backend->ports_context) != SWBT_BTSTACK_DEVICE_OK) {
        if (g_active_backend == backend) {
            g_active_backend = NULL;
        }
        return -1;
    }
    swbt_diagnostic_trace("production: device open");
    result = swbt_btstack_device_open(
        &backend->device, (swbt_btstack_device_open_options_t){
                              .service_buffer = backend->hid_service_buffer,
                              .service_buffer_size = sizeof(backend->hid_service_buffer),
                              .registration = &config,
                          });
    if (result != SWBT_BTSTACK_DEVICE_OK) {
        swbt_diagnostic_trace("production: device open failed");
        if (g_active_backend == backend) {
            g_active_backend = NULL;
        }
        return -1;
    }
    swbt_diagnostic_trace("production: device open ok");
    return 0;
}

static void swbt_daemon_production_hid_stop(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    if (g_active_backend == backend) {
        g_active_backend = NULL;
    }
    swbt_diagnostic_trace("production: device close");
    swbt_btstack_device_close(&backend->device);
    swbt_diagnostic_trace("production: device close done");
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

static int swbt_daemon_production_report_timer_start(
    void *context, swbt_daemon_process_state_provider_t state_provider, void *state_context) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_btstack_input_report_timer_adapter_config_t config;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }

    config = swbt_daemon_production_timer_config(backend, state_provider, state_context);
    if (backend->ports->report_timer.init(backend->ports_context, &backend->report_timer,
                                          &config) != 0) {
        return -1;
    }
    backend->report_timer_initialized = true;
    return 0;
}

static void swbt_daemon_production_report_timer_stop(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->initialized || !backend->report_timer_initialized) {
        return;
    }
    backend->ports->report_timer.stop(backend->ports_context, &backend->report_timer);
    backend->report_timer_initialized = false;
}

static int swbt_daemon_production_report_timer_send_neutral_now(void *context) {
    swbt_daemon_production_runner_t *backend = context;
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
    const int result = backend->ports->report_timer.send_neutral_now(backend->ports_context,
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
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->report_timer_initialized) {
        return -1;
    }
    return backend->ports->report_timer.enqueue_subcommand_reply(
        backend->ports_context, &backend->report_timer, hid_cid, report, report_size);
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
