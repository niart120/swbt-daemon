#include "daemon/runtime.h"

#include <stddef.h>

#include "core/diagnostics.h"
#include "switch/switch_spi_seed.h"
#include "switch/switch_subcommand_dispatcher.h"

static bool swbt_daemon_backend_is_valid(const swbt_daemon_runtime_backend_t *backend) {
    return backend != NULL && backend->ipc_start != NULL && backend->ipc_stop != NULL &&
           backend->hid_register != NULL && backend->hid_stop != NULL &&
           backend->output_handler_start != NULL && backend->output_handler_stop != NULL &&
           backend->report_timer_start != NULL && backend->report_timer_stop != NULL &&
           backend->report_timer_send_neutral_now != NULL &&
           backend->subcommand_reply_enqueue != NULL && backend->time_ms != NULL;
}

static bool swbt_daemon_config_is_valid(const swbt_daemon_config_t *config) {
    return config != NULL && config->report_period_us != 0u;
}

static swbt_state_t swbt_daemon_runtime_read_state(void *context) {
    swbt_daemon_runtime_t *runtime = context;
    swbt_state_mailbox_snapshot_t snapshot;

    if (runtime == NULL ||
        swbt_state_mailbox_load(&runtime->mailbox, &snapshot) != SWBT_STATE_MAILBOX_OK) {
        return swbt_state_neutral();
    }

    return snapshot.state;
}

static void swbt_daemon_runtime_on_output_report(void *context, uint16_t hid_cid,
                                                 const swbt_switch_output_report_t *report) {
    swbt_daemon_runtime_t *runtime = context;
    swbt_switch_subcommand_dispatcher_response_t response;
    swbt_switch_device_info_t device_info;

    if (runtime == NULL || report == NULL) {
        return;
    }

    (void)swbt_ipc_record_output_report_rumble(&runtime->ipc_session, report,
                                               runtime->backend->time_ms(runtime->backend_context));

    const swbt_state_t state = swbt_daemon_runtime_read_state(runtime);
    device_info = runtime->config.device_info;
    if (runtime->backend->read_device_info != NULL) {
        swbt_switch_device_info_t backend_device_info;
        if (runtime->backend->read_device_info(runtime->backend_context, &backend_device_info) ==
            0) {
            device_info = backend_device_info;
        }
    }

    const swbt_switch_subcommand_dispatcher_config_t dispatch_config = {
        .state = &state,
        .report_options = &runtime->config.report_options,
        .spi = &runtime->spi,
        .player_lights = &runtime->player_lights,
        .device_info = &device_info,
    };

    const swbt_switch_subcommand_dispatch_result_t dispatch_result =
        swbt_switch_subcommand_dispatch(&dispatch_config, report, &response);
    if (dispatch_result != SWBT_SWITCH_SUBCOMMAND_DISPATCH_OK ||
        response.action != SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY) {
        return;
    }

    (void)runtime->backend->subcommand_reply_enqueue(runtime->backend_context, hid_cid,
                                                     response.report, response.report_size);
}

static void swbt_daemon_runtime_store_neutral(swbt_daemon_runtime_t *runtime) {
    (void)swbt_ipc_clear_owner(&runtime->ipc_session);
}

swbt_daemon_runtime_result_t swbt_daemon_runtime_init(swbt_daemon_runtime_t *runtime,
                                                      const swbt_daemon_config_t *config,
                                                      const swbt_daemon_runtime_backend_t *backend,
                                                      void *backend_context) {
    if (runtime == NULL || !swbt_daemon_config_is_valid(config) ||
        !swbt_daemon_backend_is_valid(backend)) {
        return SWBT_DAEMON_RUNTIME_ERROR_INVALID_ARGUMENT;
    }

    *runtime = (swbt_daemon_runtime_t){0};
    runtime->config = *config;
    runtime->backend = backend;
    runtime->backend_context = backend_context;
    const swbt_ipc_daemon_status_t daemon_status = {
        .backend = backend->daemon_backend,
        .lifecycle_state = SWBT_IPC_DAEMON_LIFECYCLE_STOPPED,
        .hardware_approval = SWBT_IPC_HARDWARE_APPROVAL_UNAVAILABLE,
    };

    const swbt_switch_spi_seed_profile_t spi_profile = swbt_switch_spi_seed_dev_profile();
    if (swbt_state_mailbox_init(&runtime->mailbox) != SWBT_STATE_MAILBOX_OK ||
        swbt_ipc_session_init(&runtime->ipc_session) != SWBT_IPC_OK ||
        swbt_ipc_session_set_daemon_status(&runtime->ipc_session, &daemon_status) != SWBT_IPC_OK ||
        swbt_ipc_session_bind_mailbox(&runtime->ipc_session, &runtime->mailbox) != SWBT_IPC_OK ||
        swbt_switch_spi_init(&runtime->spi) != SWBT_SWITCH_SPI_OK ||
        swbt_switch_spi_seed_apply(&runtime->spi, &spi_profile) != SWBT_SWITCH_SPI_OK ||
        swbt_switch_player_lights_init(&runtime->player_lights) != SWBT_SWITCH_PLAYER_LIGHTS_OK) {
        return SWBT_DAEMON_RUNTIME_ERROR_INVALID_ARGUMENT;
    }
    swbt_btstack_output_report_handler_init(&runtime->output_handler,
                                            swbt_daemon_runtime_on_output_report, runtime);

    runtime->initialized = true;
    return SWBT_DAEMON_RUNTIME_OK;
}

swbt_daemon_runtime_result_t swbt_daemon_runtime_start(swbt_daemon_runtime_t *runtime) {
    if (runtime == NULL || !runtime->initialized) {
        return SWBT_DAEMON_RUNTIME_ERROR_INVALID_ARGUMENT;
    }
    if (runtime->running) {
        return SWBT_DAEMON_RUNTIME_OK;
    }

    swbt_diagnostic_trace("runtime: ipc start");
    if (runtime->backend->ipc_start(runtime->backend_context, &runtime->ipc_session) != 0) {
        swbt_diagnostic_trace("runtime: ipc start failed");
        return SWBT_DAEMON_RUNTIME_ERROR_BACKEND;
    }
    swbt_diagnostic_trace("runtime: ipc start ok");
    runtime->ipc_started = true;

    swbt_diagnostic_trace("runtime: hid register");
    if (runtime->backend->hid_register(runtime->backend_context) != 0) {
        swbt_diagnostic_trace("runtime: hid register failed");
        swbt_daemon_runtime_stop(runtime);
        return SWBT_DAEMON_RUNTIME_ERROR_BACKEND;
    }
    swbt_diagnostic_trace("runtime: hid register ok");
    runtime->hid_registered = true;

    swbt_diagnostic_trace("runtime: output handler start");
    runtime->backend->output_handler_start(runtime->backend_context, &runtime->output_handler);
    runtime->output_handler_started = true;

    swbt_diagnostic_trace("runtime: report timer start");
    if (runtime->backend->report_timer_start(runtime->backend_context,
                                             swbt_daemon_runtime_read_state, runtime) != 0) {
        swbt_diagnostic_trace("runtime: report timer start failed");
        swbt_daemon_runtime_stop(runtime);
        return SWBT_DAEMON_RUNTIME_ERROR_BACKEND;
    }
    swbt_diagnostic_trace("runtime: report timer start ok");
    runtime->report_timer_started = true;
    (void)swbt_ipc_session_set_daemon_lifecycle(&runtime->ipc_session,
                                                SWBT_IPC_DAEMON_LIFECYCLE_RUNNING);
    runtime->running = true;
    return SWBT_DAEMON_RUNTIME_OK;
}

swbt_daemon_runtime_result_t swbt_daemon_runtime_send_neutral_now(swbt_daemon_runtime_t *runtime) {
    if (runtime == NULL || !runtime->initialized) {
        return SWBT_DAEMON_RUNTIME_ERROR_INVALID_ARGUMENT;
    }

    swbt_daemon_runtime_store_neutral(runtime);
    if (!runtime->report_timer_started) {
        return SWBT_DAEMON_RUNTIME_OK;
    }
    if (runtime->backend->report_timer_send_neutral_now(runtime->backend_context) != 0) {
        return SWBT_DAEMON_RUNTIME_ERROR_BACKEND;
    }
    return SWBT_DAEMON_RUNTIME_OK;
}

void swbt_daemon_runtime_stop(swbt_daemon_runtime_t *runtime) {
    if (runtime == NULL || !runtime->initialized) {
        return;
    }

    swbt_diagnostic_trace("runtime: stop enter");
    swbt_daemon_runtime_store_neutral(runtime);

    if (runtime->report_timer_started) {
        swbt_diagnostic_trace("runtime: report timer stop");
        runtime->backend->report_timer_stop(runtime->backend_context);
        runtime->report_timer_started = false;
    }
    if (runtime->output_handler_started) {
        swbt_diagnostic_trace("runtime: output handler stop");
        runtime->backend->output_handler_stop(runtime->backend_context);
        runtime->output_handler_started = false;
    }
    if (runtime->hid_registered) {
        swbt_diagnostic_trace("runtime: hid stop");
        runtime->backend->hid_stop(runtime->backend_context);
        runtime->hid_registered = false;
    }
    if (runtime->ipc_started) {
        swbt_diagnostic_trace("runtime: ipc stop");
        runtime->backend->ipc_stop(runtime->backend_context);
        runtime->ipc_started = false;
    }

    (void)swbt_ipc_session_set_daemon_lifecycle(&runtime->ipc_session,
                                                SWBT_IPC_DAEMON_LIFECYCLE_STOPPED);
    runtime->running = false;
    swbt_diagnostic_trace("runtime: stop done");
}

bool swbt_daemon_runtime_is_running(const swbt_daemon_runtime_t *runtime) {
    return runtime != NULL && runtime->running;
}

swbt_ipc_session_t *swbt_daemon_runtime_ipc_session(swbt_daemon_runtime_t *runtime) {
    return runtime == NULL ? NULL : &runtime->ipc_session;
}

swbt_state_mailbox_t *swbt_daemon_runtime_mailbox(swbt_daemon_runtime_t *runtime) {
    return runtime == NULL ? NULL : &runtime->mailbox;
}

swbt_btstack_output_report_handler_t *
swbt_daemon_runtime_output_handler(swbt_daemon_runtime_t *runtime) {
    return runtime == NULL ? NULL : &runtime->output_handler;
}

static int swbt_daemon_noop_ipc_start(void *context, swbt_ipc_session_t *session) {
    (void)context;
    (void)session;
    return 0;
}

static void swbt_daemon_noop_stop(void *context) {
    (void)context;
}

static int swbt_daemon_noop_start(void *context) {
    (void)context;
    return 0;
}

static void swbt_daemon_noop_output_handler_start(void *context,
                                                  swbt_btstack_output_report_handler_t *handler) {
    (void)context;
    (void)handler;
}

static int swbt_daemon_noop_report_timer_start(void *context,
                                               swbt_daemon_state_provider_t state_provider,
                                               void *state_context) {
    (void)context;
    (void)state_provider;
    (void)state_context;
    return 0;
}

static int swbt_daemon_noop_report_timer_send_neutral_now(void *context) {
    (void)context;
    return 0;
}

static int swbt_daemon_noop_subcommand_reply_enqueue(void *context, uint16_t hid_cid,
                                                     const uint8_t *report, size_t report_size) {
    (void)context;
    (void)hid_cid;
    (void)report;
    (void)report_size;
    return 0;
}

static uint32_t swbt_daemon_noop_time_ms(void *context) {
    (void)context;
    return 0u;
}

const swbt_daemon_runtime_backend_t *swbt_daemon_runtime_noop_backend(void) {
    static const swbt_daemon_runtime_backend_t backend = {
        .daemon_backend = SWBT_IPC_DAEMON_BACKEND_NOOP,
        .ipc_start = swbt_daemon_noop_ipc_start,
        .ipc_stop = swbt_daemon_noop_stop,
        .hid_register = swbt_daemon_noop_start,
        .hid_stop = swbt_daemon_noop_stop,
        .output_handler_start = swbt_daemon_noop_output_handler_start,
        .output_handler_stop = swbt_daemon_noop_stop,
        .report_timer_start = swbt_daemon_noop_report_timer_start,
        .report_timer_stop = swbt_daemon_noop_stop,
        .report_timer_send_neutral_now = swbt_daemon_noop_report_timer_send_neutral_now,
        .subcommand_reply_enqueue = swbt_daemon_noop_subcommand_reply_enqueue,
        .time_ms = swbt_daemon_noop_time_ms,
    };
    return &backend;
}

int swbt_daemon_main_with_backend(const swbt_daemon_config_t *config,
                                  const swbt_daemon_runtime_backend_t *backend,
                                  void *backend_context) {
    swbt_daemon_runtime_t runtime;

    if (swbt_daemon_runtime_init(&runtime, config, backend, backend_context) !=
        SWBT_DAEMON_RUNTIME_OK) {
        return 1;
    }
    if (swbt_daemon_runtime_start(&runtime) != SWBT_DAEMON_RUNTIME_OK) {
        return 1;
    }

    swbt_daemon_runtime_stop(&runtime);
    return 0;
}
