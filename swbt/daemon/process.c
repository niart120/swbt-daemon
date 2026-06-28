#include "daemon/process.h"

#include <stddef.h>

#include "support/diagnostics.h"

static bool swbt_daemon_backend_is_valid(const swbt_daemon_process_backend_t *backend) {
    return backend != NULL && backend->ipc_start != NULL && backend->ipc_stop != NULL &&
           backend->runtime_backend != NULL;
}

static bool swbt_daemon_config_is_valid(const swbt_daemon_config_t *config) {
    return config != NULL && config->report_period_us != 0u;
}

static swbt_daemon_process_result_t
swbt_daemon_process_result_from_runtime(swbt_runtime_host_result_t result) {
    switch (result) {
    case SWBT_RUNTIME_HOST_OK:
        return SWBT_DAEMON_PROCESS_OK;
    case SWBT_RUNTIME_HOST_PENDING:
        return SWBT_DAEMON_PROCESS_PENDING;
    case SWBT_RUNTIME_HOST_ERROR_INVALID_ARGUMENT:
        return SWBT_DAEMON_PROCESS_ERROR_INVALID_ARGUMENT;
    case SWBT_RUNTIME_HOST_ERROR_BACKEND:
        return SWBT_DAEMON_PROCESS_ERROR_BACKEND;
    }
    return SWBT_DAEMON_PROCESS_ERROR_BACKEND;
}

swbt_daemon_process_result_t swbt_daemon_process_init(swbt_daemon_process_t *host,
                                                      const swbt_daemon_config_t *config,
                                                      const swbt_daemon_process_backend_t *backend,
                                                      void *backend_context) {
    if (host == NULL || !swbt_daemon_config_is_valid(config) ||
        !swbt_daemon_backend_is_valid(backend)) {
        return SWBT_DAEMON_PROCESS_ERROR_INVALID_ARGUMENT;
    }

    *host = (swbt_daemon_process_t){0};
    host->config = *config;
    host->backend = backend;
    host->backend_context = backend_context;

    const swbt_domain_daemon_status_t daemon_status = {
        .backend = backend->daemon_backend,
        .lifecycle_state = SWBT_DOMAIN_DAEMON_LIFECYCLE_STOPPED,
        .hardware_approval = SWBT_DOMAIN_HARDWARE_APPROVAL_UNAVAILABLE,
    };
    if (swbt_runtime_host_init(&host->runtime,
                               &(swbt_runtime_host_config_t){
                                   .report_options = config->report_options,
                                   .device_info = config->device_info,
                                   .daemon_status = daemon_status,
                               },
                               backend->runtime_backend, backend_context) != SWBT_RUNTIME_HOST_OK) {
        return SWBT_DAEMON_PROCESS_ERROR_INVALID_ARGUMENT;
    }

    host->initialized = true;
    return SWBT_DAEMON_PROCESS_OK;
}

swbt_daemon_process_result_t swbt_daemon_process_start(swbt_daemon_process_t *host) {
    if (host == NULL || !host->initialized) {
        return SWBT_DAEMON_PROCESS_ERROR_INVALID_ARGUMENT;
    }
    if (host->running) {
        return SWBT_DAEMON_PROCESS_OK;
    }

    swbt_diagnostic_trace("host: ipc start");
    if (host->backend->ipc_start(host->backend_context,
                                 swbt_runtime_host_control(&host->runtime)) != 0) {
        swbt_diagnostic_trace("host: ipc start failed");
        return SWBT_DAEMON_PROCESS_ERROR_BACKEND;
    }
    swbt_diagnostic_trace("host: ipc start ok");
    host->ipc_started = true;

    swbt_diagnostic_trace("host: runtime start");
    if (swbt_runtime_host_start(&host->runtime) != SWBT_RUNTIME_HOST_OK) {
        swbt_diagnostic_trace("host: runtime start failed");
        swbt_daemon_process_stop(host);
        return SWBT_DAEMON_PROCESS_ERROR_BACKEND;
    }
    swbt_diagnostic_trace("host: runtime start ok");
    (void)swbt_domain_set_daemon_lifecycle(swbt_runtime_host_app(&host->runtime),
                                           SWBT_DOMAIN_DAEMON_LIFECYCLE_RUNNING);
    host->running = true;
    return SWBT_DAEMON_PROCESS_OK;
}

swbt_daemon_process_result_t swbt_daemon_process_send_neutral_now(swbt_daemon_process_t *host) {
    if (host == NULL || !host->initialized) {
        return SWBT_DAEMON_PROCESS_ERROR_INVALID_ARGUMENT;
    }

    return swbt_daemon_process_result_from_runtime(
        swbt_runtime_host_send_neutral_now(&host->runtime));
}

void swbt_daemon_process_stop(swbt_daemon_process_t *host) {
    if (host == NULL || !host->initialized) {
        return;
    }

    swbt_diagnostic_trace("host: stop enter");
    swbt_diagnostic_trace("host: runtime stop");
    swbt_runtime_host_stop(&host->runtime);
    if (host->ipc_started) {
        swbt_diagnostic_trace("host: ipc stop");
        host->backend->ipc_stop(host->backend_context);
        host->ipc_started = false;
    }

    (void)swbt_domain_set_daemon_lifecycle(swbt_runtime_host_app(&host->runtime),
                                           SWBT_DOMAIN_DAEMON_LIFECYCLE_STOPPED);
    host->running = false;
    swbt_diagnostic_trace("host: stop done");
}

void swbt_daemon_process_destroy(swbt_daemon_process_t *host) {
    if (host == NULL) {
        return;
    }
    swbt_daemon_process_stop(host);
    swbt_runtime_host_destroy(&host->runtime);
    host->initialized = false;
}

bool swbt_daemon_process_is_running(const swbt_daemon_process_t *host) {
    return host != NULL && host->running;
}

swbt_domain_t *swbt_daemon_process_app(swbt_daemon_process_t *host) {
    return host == NULL ? NULL : swbt_runtime_host_app(&host->runtime);
}

swbt_control_t *swbt_daemon_process_control(swbt_daemon_process_t *host) {
    return host == NULL ? NULL : swbt_runtime_host_control(&host->runtime);
}

swbt_btstack_output_report_handler_t *
swbt_daemon_process_output_handler(swbt_daemon_process_t *host) {
    return host == NULL ? NULL : swbt_runtime_host_output_handler(&host->runtime);
}

static int swbt_daemon_noop_ipc_start(void *context, swbt_control_t *control) {
    (void)context;
    (void)control;
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
                                               swbt_runtime_state_provider_t state_provider,
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

const swbt_daemon_process_backend_t *swbt_daemon_process_noop_backend(void) {
    static const swbt_runtime_host_backend_t runtime_backend = {
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
    static const swbt_daemon_process_backend_t backend = {
        .daemon_backend = SWBT_DOMAIN_DAEMON_BACKEND_NOOP,
        .ipc_start = swbt_daemon_noop_ipc_start,
        .ipc_stop = swbt_daemon_noop_stop,
        .runtime_backend = &runtime_backend,
    };
    return &backend;
}

int swbt_daemon_main_with_process_backend(const swbt_daemon_config_t *config,
                                          const swbt_daemon_process_backend_t *backend,
                                          void *backend_context) {
    swbt_daemon_process_t host;

    if (swbt_daemon_process_init(&host, config, backend, backend_context) !=
        SWBT_DAEMON_PROCESS_OK) {
        return 1;
    }
    if (swbt_daemon_process_start(&host) != SWBT_DAEMON_PROCESS_OK) {
        swbt_daemon_process_destroy(&host);
        return 1;
    }

    swbt_daemon_process_destroy(&host);
    return 0;
}
