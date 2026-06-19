#include "daemon/runtime.h"

#include <stddef.h>

static bool swbt_daemon_backend_is_valid(const swbt_daemon_runtime_backend_t *backend) {
    return backend != NULL && backend->ipc_start != NULL && backend->ipc_stop != NULL &&
           backend->hid_register != NULL && backend->hid_stop != NULL &&
           backend->output_handler_start != NULL && backend->output_handler_stop != NULL &&
           backend->report_timer_start != NULL && backend->report_timer_stop != NULL;
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
    (void)context;
    (void)hid_cid;
    (void)report;
}

static void swbt_daemon_runtime_store_neutral(swbt_daemon_runtime_t *runtime) {
    const swbt_state_t neutral = swbt_state_neutral();

    runtime->ipc_session.has_owner = false;
    runtime->ipc_session.owner_client_id = 0u;
    runtime->ipc_session.state = neutral;
    (void)swbt_state_mailbox_store(&runtime->mailbox, &neutral);
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

    if (swbt_state_mailbox_init(&runtime->mailbox) != SWBT_STATE_MAILBOX_OK ||
        swbt_ipc_session_init(&runtime->ipc_session) != SWBT_IPC_OK ||
        swbt_ipc_session_bind_mailbox(&runtime->ipc_session, &runtime->mailbox) != SWBT_IPC_OK) {
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

    if (runtime->backend->ipc_start(runtime->backend_context, &runtime->ipc_session) != 0) {
        return SWBT_DAEMON_RUNTIME_ERROR_BACKEND;
    }
    runtime->ipc_started = true;

    if (runtime->backend->hid_register(runtime->backend_context) != 0) {
        swbt_daemon_runtime_stop(runtime);
        return SWBT_DAEMON_RUNTIME_ERROR_BACKEND;
    }
    runtime->hid_registered = true;

    runtime->backend->output_handler_start(runtime->backend_context, &runtime->output_handler);
    runtime->output_handler_started = true;

    if (runtime->backend->report_timer_start(runtime->backend_context,
                                             swbt_daemon_runtime_read_state, runtime) != 0) {
        swbt_daemon_runtime_stop(runtime);
        return SWBT_DAEMON_RUNTIME_ERROR_BACKEND;
    }
    runtime->report_timer_started = true;
    runtime->running = true;
    return SWBT_DAEMON_RUNTIME_OK;
}

void swbt_daemon_runtime_stop(swbt_daemon_runtime_t *runtime) {
    if (runtime == NULL || !runtime->initialized) {
        return;
    }

    swbt_daemon_runtime_store_neutral(runtime);

    if (runtime->report_timer_started) {
        runtime->backend->report_timer_stop(runtime->backend_context);
        runtime->report_timer_started = false;
    }
    if (runtime->output_handler_started) {
        runtime->backend->output_handler_stop(runtime->backend_context);
        runtime->output_handler_started = false;
    }
    if (runtime->hid_registered) {
        runtime->backend->hid_stop(runtime->backend_context);
        runtime->hid_registered = false;
    }
    if (runtime->ipc_started) {
        runtime->backend->ipc_stop(runtime->backend_context);
        runtime->ipc_started = false;
    }

    runtime->running = false;
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

const swbt_daemon_runtime_backend_t *swbt_daemon_runtime_noop_backend(void) {
    static const swbt_daemon_runtime_backend_t backend = {
        .ipc_start = swbt_daemon_noop_ipc_start,
        .ipc_stop = swbt_daemon_noop_stop,
        .hid_register = swbt_daemon_noop_start,
        .hid_stop = swbt_daemon_noop_stop,
        .output_handler_start = swbt_daemon_noop_output_handler_start,
        .output_handler_stop = swbt_daemon_noop_stop,
        .report_timer_start = swbt_daemon_noop_report_timer_start,
        .report_timer_stop = swbt_daemon_noop_stop,
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
