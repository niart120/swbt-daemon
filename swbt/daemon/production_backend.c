#include "daemon/production_backend.h"

#include <stddef.h>

#include "core/diagnostics.h"
#include "switch/switch_hid_descriptor.h"

#define SWBT_BTSTACK_HCI_EVENT_PACKET 0x04u
#define SWBT_BTSTACK_HCI_EVENT_USER_CONFIRMATION_REQUEST 0x33u
#define SWBT_BTSTACK_HCI_EVENT_HID_META 0xefu
#define SWBT_BTSTACK_HID_SUBEVENT_CONNECTION_OPENED 0x02u
#define SWBT_BTSTACK_HID_SUBEVENT_CONNECTION_CLOSED 0x03u
#define SWBT_BTSTACK_HID_SUBEVENT_CAN_SEND_NOW 0x04u

static swbt_daemon_production_backend_t *g_active_backend;

static uint16_t swbt_daemon_production_read_u16_le(const uint8_t *data) {
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static bool swbt_daemon_production_ops_are_valid(const swbt_daemon_production_backend_ops_t *ops) {
    return ops != NULL && ops->ipc_start != NULL && ops->ipc_stop != NULL &&
           ops->platform_start != NULL && ops->platform_stop != NULL && ops->hid_register != NULL &&
           ops->hid_stop != NULL && ops->output_handler_start != NULL &&
           ops->output_handler_stop != NULL && ops->report_timer_init != NULL &&
           ops->report_timer_start != NULL && ops->report_timer_on_can_send_now != NULL &&
           ops->report_timer_enqueue_subcommand_reply != NULL && ops->report_timer_stop != NULL &&
           ops->ssp_confirm_user_confirmation != NULL && ops->time_ms != NULL &&
           ops->read_controller_address != NULL && ops->power_on != NULL &&
           ops->power_off != NULL && ops->run_loop_execute != NULL &&
           ops->run_loop_trigger_exit != NULL;
}

swbt_btstack_hid_registration_config_t swbt_daemon_production_hid_registration_config(void) {
    return (swbt_btstack_hid_registration_config_t){
        .hid_device_subclass = 0x2508u,
        .hid_country_code = 0x21u,
        .hid_virtual_cable = 1u,
        .hid_remote_wake = 1u,
        .hid_reconnect_initiate = 1u,
        .hid_normally_connectable = true,
        .hid_boot_device = false,
        .hid_ssr_host_max_latency = 0xffffu,
        .hid_ssr_host_min_timeout = 0xffffu,
        .hid_supervision_timeout = 0x0c80u,
        .hid_descriptor = swbt_switch_hid_descriptor_data(),
        .hid_descriptor_size = (uint16_t)swbt_switch_hid_descriptor_size(),
        .device_name = "Pro Controller",
        .packet_handler = NULL,
    };
}

static swbt_btstack_input_report_timer_adapter_config_t
swbt_daemon_production_timer_config(swbt_daemon_production_backend_t *backend,
                                    swbt_daemon_state_provider_t state_provider,
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
    const swbt_daemon_production_backend_ops_t *ops, void *ops_context) {
    if (backend == NULL || config == NULL || config->report_period_us == 0u ||
        !swbt_daemon_production_ops_are_valid(ops)) {
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }

    *backend = (swbt_daemon_production_backend_t){0};
    backend->config = *config;
    backend->ops = ops;
    backend->ops_context = ops_context;
    backend->ipc_runner.config = swbt_daemon_ipc_runner_config_from_daemon_config(config);
    if (swbt_daemon_ipc_runner_init(&backend->ipc_runner) != SWBT_DAEMON_IPC_RUNNER_OK) {
        return SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
    }
    backend->ipc_runner.config = swbt_daemon_ipc_runner_config_from_daemon_config(config);
    backend->initialized = true;
    return SWBT_DAEMON_PRODUCTION_OK;
}

static int swbt_daemon_production_ipc_start(void *context, swbt_ipc_session_t *session) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return -1;
    }
    return backend->ops->ipc_start(backend->ops_context, &backend->ipc_runner, session,
                                   &backend->ipc_runner.config);
}

static void swbt_daemon_production_ipc_stop(void *context) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    backend->ops->ipc_stop(backend->ops_context, &backend->ipc_runner);
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
static void swbt_daemon_production_hid_packet_handler(uint8_t packet_type, uint16_t channel,
                                                      uint8_t *packet, uint16_t size) {
    swbt_daemon_production_backend_t *backend = g_active_backend;
    (void)channel;

    if (backend == NULL || packet_type != SWBT_BTSTACK_HCI_EVENT_PACKET || packet == NULL) {
        return;
    }

    if (size >= 12u && packet[0] == SWBT_BTSTACK_HCI_EVENT_USER_CONFIRMATION_REQUEST) {
        const uint8_t address[6] = {packet[7], packet[6], packet[5],
                                    packet[4], packet[3], packet[2]};
        (void)backend->ops->ssp_confirm_user_confirmation(backend->ops_context, address);
        return;
    }

    if (size < 5u || packet[0] != SWBT_BTSTACK_HCI_EVENT_HID_META ||
        !backend->report_timer_initialized) {
        return;
    }

    switch (packet[2]) {
    case SWBT_BTSTACK_HID_SUBEVENT_CONNECTION_OPENED:
        if (size < 15u || packet[5] != 0u) {
            return;
        }
        (void)backend->ops->report_timer_start(
            backend->ops_context, &backend->report_timer,
            (swbt_btstack_input_report_timer_start_options_t){
                .hid_cid = swbt_daemon_production_read_u16_le(&packet[3]),
                .now_us = (uint64_t)backend->ops->time_ms(backend->ops_context) * 1000u,
            });
        break;
    case SWBT_BTSTACK_HID_SUBEVENT_CAN_SEND_NOW:
        (void)backend->ops->report_timer_on_can_send_now(backend->ops_context,
                                                         &backend->report_timer);
        break;
    case SWBT_BTSTACK_HID_SUBEVENT_CONNECTION_CLOSED:
        backend->ops->report_timer_stop(backend->ops_context, &backend->report_timer);
        break;
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
        if (backend->ops->platform_start(backend->ops_context) != 0) {
            swbt_diagnostic_trace("production: platform start failed");
            return -1;
        }
        backend->platform_started = true;
        swbt_diagnostic_trace("production: platform start ok");
    }

    config = swbt_daemon_production_hid_registration_config();
    config.packet_handler = swbt_daemon_production_hid_packet_handler;
    g_active_backend = backend;
    swbt_diagnostic_trace("production: hid register btstack");
    if (backend->ops->hid_register(backend->ops_context, backend->hid_service_buffer,
                                   sizeof(backend->hid_service_buffer), &config) != 0) {
        swbt_diagnostic_trace("production: hid register failed");
        if (g_active_backend == backend) {
            g_active_backend = NULL;
        }
        if (backend->platform_started) {
            swbt_diagnostic_trace("production: platform stop after hid failure");
            backend->ops->platform_stop(backend->ops_context);
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
        backend->ops->hid_stop(backend->ops_context);
        backend->hid_registered = false;
    }
    if (g_active_backend == backend) {
        g_active_backend = NULL;
    }
    if (backend->platform_started) {
        swbt_diagnostic_trace("production: platform stop");
        backend->ops->platform_stop(backend->ops_context);
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
    backend->ops->output_handler_start(backend->ops_context, handler);
}

static void swbt_daemon_production_output_handler_stop(void *context) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    backend->ops->output_handler_stop(backend->ops_context);
}

static int swbt_daemon_production_report_timer_start(void *context,
                                                     swbt_daemon_state_provider_t state_provider,
                                                     void *state_context) {
    swbt_daemon_production_backend_t *backend = context;
    swbt_btstack_input_report_timer_adapter_config_t config;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }

    config = swbt_daemon_production_timer_config(backend, state_provider, state_context);
    if (backend->ops->report_timer_init(backend->ops_context, &backend->report_timer, &config) !=
        0) {
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
    backend->ops->report_timer_stop(backend->ops_context, &backend->report_timer);
    backend->report_timer_initialized = false;
}

static int swbt_daemon_production_subcommand_reply_enqueue(void *context, uint16_t hid_cid,
                                                           const uint8_t *report,
                                                           size_t report_size) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->report_timer_initialized) {
        return -1;
    }
    return backend->ops->report_timer_enqueue_subcommand_reply(
        backend->ops_context, &backend->report_timer, hid_cid, report, report_size);
}

static int swbt_daemon_production_read_device_info(void *context,
                                                   swbt_switch_device_info_t *out_device_info) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || out_device_info == NULL) {
        return -1;
    }

    *out_device_info = backend->config.device_info;
    return backend->ops->read_controller_address(backend->ops_context,
                                                 out_device_info->bluetooth_address);
}

const swbt_daemon_runtime_backend_t *swbt_daemon_production_runtime_backend(void) {
    static const swbt_daemon_runtime_backend_t backend = {
        .ipc_start = swbt_daemon_production_ipc_start,
        .ipc_stop = swbt_daemon_production_ipc_stop,
        .hid_register = swbt_daemon_production_hid_register,
        .hid_stop = swbt_daemon_production_hid_stop,
        .output_handler_start = swbt_daemon_production_output_handler_start,
        .output_handler_stop = swbt_daemon_production_output_handler_stop,
        .report_timer_start = swbt_daemon_production_report_timer_start,
        .report_timer_stop = swbt_daemon_production_report_timer_stop,
        .subcommand_reply_enqueue = swbt_daemon_production_subcommand_reply_enqueue,
        .read_device_info = swbt_daemon_production_read_device_info,
    };
    return &backend;
}

bool swbt_daemon_hardware_approval_is_granted(const swbt_daemon_hardware_approval_t *approval) {
    return approval != NULL && approval->run_hardware && approval->hardware_approved;
}

static swbt_daemon_production_result_t
swbt_daemon_production_power_on(swbt_daemon_production_backend_t *backend) {
    if (backend->ops->power_on(backend->ops_context) != 0) {
        return SWBT_DAEMON_PRODUCTION_ERROR_HARDWARE;
    }
    atomic_store(&backend->hardware_powered, true);
    return SWBT_DAEMON_PRODUCTION_OK;
}

static void swbt_daemon_production_power_off(swbt_daemon_production_backend_t *backend) {
    if (backend != NULL && atomic_exchange(&backend->hardware_powered, false)) {
        backend->ops->power_off(backend->ops_context);
    }
}

static bool
swbt_daemon_shutdown_listener_is_valid(const swbt_daemon_shutdown_listener_t *shutdown_listener) {
    return shutdown_listener == NULL ||
           (shutdown_listener->install != NULL && shutdown_listener->uninstall != NULL);
}

static void swbt_daemon_production_request_shutdown(void *context) {
    swbt_daemon_production_backend_t *backend = context;
    if (backend == NULL || !backend->initialized ||
        atomic_exchange(&backend->shutdown_requested, true)) {
        return;
    }

    swbt_daemon_production_power_off(backend);
    backend->ops->run_loop_trigger_exit(backend->ops_context);
}

swbt_daemon_production_result_t swbt_daemon_production_main_with_backend_and_shutdown(
    swbt_daemon_production_backend_t *backend, const swbt_daemon_hardware_approval_t *approval,
    const swbt_daemon_shutdown_listener_t *shutdown_listener, void *shutdown_context) {
    swbt_daemon_runtime_t runtime;
    swbt_daemon_runtime_result_t runtime_result;
    swbt_daemon_production_result_t result = SWBT_DAEMON_PRODUCTION_OK;
    bool shutdown_listener_installed = false;

    if (backend == NULL || !backend->initialized ||
        !swbt_daemon_shutdown_listener_is_valid(shutdown_listener)) {
        return SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT;
    }
    if (!swbt_daemon_hardware_approval_is_granted(approval)) {
        return SWBT_DAEMON_PRODUCTION_ERROR_HARDWARE_APPROVAL_REQUIRED;
    }

    swbt_diagnostic_trace("production: runtime init");
    runtime_result = swbt_daemon_runtime_init(&runtime, &backend->config,
                                              swbt_daemon_production_runtime_backend(), backend);
    if (runtime_result != SWBT_DAEMON_RUNTIME_OK) {
        swbt_diagnostic_trace("production: runtime init failed");
        return SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
    }
    swbt_diagnostic_trace("production: runtime start");
    runtime_result = swbt_daemon_runtime_start(&runtime);
    if (runtime_result != SWBT_DAEMON_RUNTIME_OK) {
        swbt_diagnostic_trace("production: runtime start failed");
        return SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME;
    }
    swbt_diagnostic_trace("production: runtime start ok");

    swbt_diagnostic_trace("production: power on");
    result = swbt_daemon_production_power_on(backend);
    if (result == SWBT_DAEMON_PRODUCTION_OK) {
        atomic_store(&backend->shutdown_requested, false);
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
            backend->ops->run_loop_execute(backend->ops_context);
            swbt_diagnostic_trace("production: run loop returned");
        }
        if (shutdown_listener_installed) {
            shutdown_listener->uninstall(shutdown_context);
        }
    }

    swbt_diagnostic_trace("production: power off cleanup");
    swbt_daemon_production_power_off(backend);
    swbt_diagnostic_trace("production: runtime stop");
    swbt_daemon_runtime_stop(&runtime);
    swbt_diagnostic_trace("production: runtime stop done");
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
