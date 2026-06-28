#include "daemon/btstack_process_backend.h"

#include "daemon/process_internal.h"
#include "daemon/btstack_hid_session.h"
#include "daemon/btstack_ipc_pump_adapter.h"
#include "daemon/btstack_report_timer_bridge.h"
#include "daemon/production_runner_internal.h"
#include "daemon/shutdown_sequence.h"

static int swbt_daemon_production_ipc_start(void *context, swbt_control_t *control) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_btstack_ipc_pump_adapter_t ipc_pump;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }
    ipc_pump = (swbt_daemon_btstack_ipc_pump_adapter_t){
        .runner = &backend->ipc_runner,
        .port = &backend->ports->ipc_pump,
        .port_context = backend->ports_context,
    };
    return swbt_daemon_btstack_ipc_pump_adapter_start(&ipc_pump, control);
}

static void swbt_daemon_production_ipc_stop(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_btstack_ipc_pump_adapter_t ipc_pump;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    ipc_pump = (swbt_daemon_btstack_ipc_pump_adapter_t){
        .runner = &backend->ipc_runner,
        .port = &backend->ports->ipc_pump,
        .port_context = backend->ports_context,
    };
    swbt_daemon_btstack_ipc_pump_adapter_stop(&ipc_pump);
}

static swbt_daemon_btstack_report_timer_bridge_t *
swbt_daemon_btstack_report_timer_bridge_from_backend(swbt_daemon_production_runner_t *backend) {
    backend->report_timer_bridge = (swbt_daemon_btstack_report_timer_bridge_t){
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

static void swbt_daemon_btstack_hid_session_finish_shutdown(void *context) {
    swbt_daemon_shutdown_sequence_finish(context);
}

static swbt_daemon_btstack_hid_session_t *
swbt_daemon_btstack_hid_session_from_backend(swbt_daemon_production_runner_t *backend) {
    backend->hid_session_bridge = (swbt_daemon_btstack_hid_session_t){
        .config = &backend->config,
        .device_port = &backend->ports->device,
        .report_timer_port = &backend->ports->report_timer,
        .controller_port = &backend->ports->controller,
        .clock_port = &backend->ports->clock,
        .port_context = backend->ports_context,
        .device = &backend->device,
        .report_timer = &backend->report_timer,
        .report_timer_initialized = &backend->report_timer_initialized,
        .shutdown_neutral_pending = &backend->shutdown.neutral_pending,
        .learned_switch_address_target = &backend->learned_switch_address_target,
        .learned_switch_address_target_configured =
            &backend->learned_switch_address_target_configured,
        .service_buffer = backend->hid_service_buffer,
        .service_buffer_size = sizeof(backend->hid_service_buffer),
        .finish_shutdown = swbt_daemon_btstack_hid_session_finish_shutdown,
        .finish_shutdown_context = &backend->shutdown,
    };
    return &backend->hid_session_bridge;
}

static int swbt_daemon_production_hid_register(void *context) {
    swbt_daemon_production_runner_t *backend = context;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }
    return swbt_daemon_btstack_hid_session_register(
        swbt_daemon_btstack_hid_session_from_backend(backend));
}

static void swbt_daemon_production_hid_stop(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    if (backend == NULL || !backend->initialized) {
        return;
    }
    swbt_daemon_btstack_hid_session_stop(swbt_daemon_btstack_hid_session_from_backend(backend));
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
    void *context, swbt_runtime_state_provider_t state_provider, void *state_context) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_btstack_report_timer_bridge_t *timer;

    if (backend == NULL || !backend->initialized) {
        return -1;
    }

    timer = swbt_daemon_btstack_report_timer_bridge_from_backend(backend);
    return swbt_daemon_btstack_report_timer_bridge_start(timer, state_provider, state_context);
}

static void swbt_daemon_production_process_report_timer_stop(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_btstack_report_timer_bridge_t *timer;
    if (backend == NULL || !backend->initialized || !backend->report_timer_initialized) {
        return;
    }
    timer = swbt_daemon_btstack_report_timer_bridge_from_backend(backend);
    swbt_daemon_btstack_report_timer_bridge_stop(timer);
}

static int swbt_daemon_production_process_report_timer_send_neutral_now(void *context) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_btstack_report_timer_bridge_t *timer;
    if (backend == NULL || !backend->initialized) {
        return -1;
    }
    timer = swbt_daemon_btstack_report_timer_bridge_from_backend(backend);
    return swbt_daemon_btstack_report_timer_bridge_send_neutral_now(timer);
}

static int swbt_daemon_production_process_subcommand_reply_enqueue(void *context, uint16_t hid_cid,
                                                                   const uint8_t *report,
                                                                   size_t report_size) {
    swbt_daemon_production_runner_t *backend = context;
    swbt_daemon_btstack_report_timer_bridge_t *timer;
    if (backend == NULL || !backend->report_timer_initialized) {
        return -1;
    }
    timer = swbt_daemon_btstack_report_timer_bridge_from_backend(backend);
    return swbt_daemon_btstack_report_timer_bridge_enqueue_subcommand_reply(timer, hid_cid, report,
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

static const swbt_runtime_host_backend_t swbt_daemon_btstack_runtime_backend_table = {
    .hid_register = swbt_daemon_production_hid_register,
    .hid_stop = swbt_daemon_production_hid_stop,
    .output_handler_start = swbt_daemon_production_output_handler_start,
    .output_handler_stop = swbt_daemon_production_output_handler_stop,
    .report_timer_start = swbt_daemon_production_process_report_timer_start,
    .report_timer_stop = swbt_daemon_production_process_report_timer_stop,
    .report_timer_send_neutral_now = swbt_daemon_production_process_report_timer_send_neutral_now,
    .subcommand_reply_enqueue = swbt_daemon_production_process_subcommand_reply_enqueue,
    .read_device_info = swbt_daemon_production_read_device_info,
    .time_ms = swbt_daemon_production_host_time_ms,
};

const swbt_runtime_host_backend_t *swbt_daemon_btstack_runtime_backend(void) {
    return &swbt_daemon_btstack_runtime_backend_table;
}

const swbt_daemon_process_backend_t *swbt_daemon_btstack_process_backend(void) {
    static const swbt_daemon_process_backend_t backend = {
        .daemon_backend = SWBT_DOMAIN_DAEMON_BACKEND_PRODUCTION,
        .ipc_start = swbt_daemon_production_ipc_start,
        .ipc_stop = swbt_daemon_production_ipc_stop,
        .runtime_backend = &swbt_daemon_btstack_runtime_backend_table,
    };
    return &backend;
}
