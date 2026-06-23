#include "daemon/host.h"

#include <stddef.h>

#include "core/diagnostics.h"

static bool swbt_daemon_backend_is_valid(const swbt_daemon_host_backend_t *backend) {
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

static swbt_state_t swbt_daemon_host_read_state(void *context) {
    swbt_daemon_host_t *host = context;
    swbt_app_snapshot_t snapshot;

    if (host == NULL || swbt_app_snapshot(host->app, &snapshot) != SWBT_APP_OK) {
        return swbt_state_neutral();
    }
    return snapshot.state;
}

static void swbt_daemon_host_on_output_report(void *context, uint16_t hid_cid,
                                              const swbt_switch_output_report_t *report) {
    swbt_daemon_host_t *host = context;
    swbt_switch_subcommand_dispatcher_response_t response;
    swbt_switch_device_info_t device_info;

    if (host == NULL || report == NULL) {
        return;
    }

    device_info = host->config.device_info;
    if (host->backend->read_device_info != NULL) {
        swbt_switch_device_info_t backend_device_info;
        if (host->backend->read_device_info(host->backend_context, &backend_device_info) == 0) {
            device_info = backend_device_info;
        }
    }

    if (swbt_app_handle_output_report(host->app, report, &host->config.report_options, &device_info,
                                      host->backend->time_ms(host->backend_context),
                                      &response) != SWBT_APP_OK ||
        response.action != SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY) {
        return;
    }

    (void)host->backend->subcommand_reply_enqueue(host->backend_context, hid_cid, response.report,
                                                  response.report_size);
}

static void swbt_daemon_host_store_neutral(swbt_daemon_host_t *host) {
    (void)swbt_app_revoke(host->app, SWBT_APP_REVOKE_SHUTDOWN, 0u);
}

swbt_daemon_host_result_t swbt_daemon_host_init(swbt_daemon_host_t *host,
                                                const swbt_daemon_config_t *config,
                                                const swbt_daemon_host_backend_t *backend,
                                                void *backend_context) {
    if (host == NULL || !swbt_daemon_config_is_valid(config) ||
        !swbt_daemon_backend_is_valid(backend)) {
        return SWBT_DAEMON_HOST_ERROR_INVALID_ARGUMENT;
    }

    *host = (swbt_daemon_host_t){0};
    host->config = *config;
    host->backend = backend;
    host->backend_context = backend_context;
    host->app = swbt_app_create();
    if (host->app == NULL) {
        return SWBT_DAEMON_HOST_ERROR_INVALID_ARGUMENT;
    }

    const swbt_app_daemon_status_t daemon_status = {
        .backend = backend->daemon_backend,
        .lifecycle_state = SWBT_APP_DAEMON_LIFECYCLE_STOPPED,
        .hardware_approval = SWBT_APP_HARDWARE_APPROVAL_UNAVAILABLE,
    };
    if (swbt_app_set_daemon_status(host->app, &daemon_status) != SWBT_APP_OK) {
        swbt_app_destroy(host->app);
        host->app = NULL;
        return SWBT_DAEMON_HOST_ERROR_INVALID_ARGUMENT;
    }

    swbt_btstack_output_report_handler_init(&host->output_handler,
                                            swbt_daemon_host_on_output_report, host);

    host->initialized = true;
    return SWBT_DAEMON_HOST_OK;
}

swbt_daemon_host_result_t swbt_daemon_host_start(swbt_daemon_host_t *host) {
    if (host == NULL || !host->initialized) {
        return SWBT_DAEMON_HOST_ERROR_INVALID_ARGUMENT;
    }
    if (host->running) {
        return SWBT_DAEMON_HOST_OK;
    }

    swbt_diagnostic_trace("host: ipc start");
    if (host->backend->ipc_start(host->backend_context, host->app) != 0) {
        swbt_diagnostic_trace("host: ipc start failed");
        return SWBT_DAEMON_HOST_ERROR_BACKEND;
    }
    swbt_diagnostic_trace("host: ipc start ok");
    host->ipc_started = true;

    swbt_diagnostic_trace("host: hid register");
    if (host->backend->hid_register(host->backend_context) != 0) {
        swbt_diagnostic_trace("host: hid register failed");
        swbt_daemon_host_stop(host);
        return SWBT_DAEMON_HOST_ERROR_BACKEND;
    }
    swbt_diagnostic_trace("host: hid register ok");
    host->hid_registered = true;

    swbt_diagnostic_trace("host: output handler start");
    host->backend->output_handler_start(host->backend_context, &host->output_handler);
    host->output_handler_started = true;

    swbt_diagnostic_trace("host: report timer start");
    if (host->backend->report_timer_start(host->backend_context, swbt_daemon_host_read_state,
                                          host) != 0) {
        swbt_diagnostic_trace("host: report timer start failed");
        swbt_daemon_host_stop(host);
        return SWBT_DAEMON_HOST_ERROR_BACKEND;
    }
    swbt_diagnostic_trace("host: report timer start ok");
    host->report_timer_started = true;
    (void)swbt_app_set_daemon_lifecycle(host->app, SWBT_APP_DAEMON_LIFECYCLE_RUNNING);
    host->running = true;
    return SWBT_DAEMON_HOST_OK;
}

swbt_daemon_host_result_t swbt_daemon_host_send_neutral_now(swbt_daemon_host_t *host) {
    if (host == NULL || !host->initialized) {
        return SWBT_DAEMON_HOST_ERROR_INVALID_ARGUMENT;
    }

    swbt_daemon_host_store_neutral(host);
    if (!host->report_timer_started) {
        return SWBT_DAEMON_HOST_OK;
    }
    const int send_result = host->backend->report_timer_send_neutral_now(host->backend_context);
    if (send_result > 0) {
        return SWBT_DAEMON_HOST_PENDING;
    }
    if (send_result != 0) {
        return SWBT_DAEMON_HOST_ERROR_BACKEND;
    }
    return SWBT_DAEMON_HOST_OK;
}

void swbt_daemon_host_stop(swbt_daemon_host_t *host) {
    if (host == NULL || !host->initialized) {
        return;
    }

    swbt_diagnostic_trace("host: stop enter");
    swbt_daemon_host_store_neutral(host);

    if (host->report_timer_started) {
        swbt_diagnostic_trace("host: report timer stop");
        host->backend->report_timer_stop(host->backend_context);
        host->report_timer_started = false;
    }
    if (host->output_handler_started) {
        swbt_diagnostic_trace("host: output handler stop");
        host->backend->output_handler_stop(host->backend_context);
        host->output_handler_started = false;
    }
    if (host->hid_registered) {
        swbt_diagnostic_trace("host: hid stop");
        host->backend->hid_stop(host->backend_context);
        host->hid_registered = false;
    }
    if (host->ipc_started) {
        swbt_diagnostic_trace("host: ipc stop");
        host->backend->ipc_stop(host->backend_context);
        host->ipc_started = false;
    }

    (void)swbt_app_set_daemon_lifecycle(host->app, SWBT_APP_DAEMON_LIFECYCLE_STOPPED);
    host->running = false;
    swbt_diagnostic_trace("host: stop done");
}

void swbt_daemon_host_destroy(swbt_daemon_host_t *host) {
    if (host == NULL) {
        return;
    }
    swbt_daemon_host_stop(host);
    swbt_app_destroy(host->app);
    host->app = NULL;
    host->initialized = false;
}

bool swbt_daemon_host_is_running(const swbt_daemon_host_t *host) {
    return host != NULL && host->running;
}

swbt_app_t *swbt_daemon_host_app(swbt_daemon_host_t *host) {
    return host == NULL ? NULL : host->app;
}

swbt_btstack_output_report_handler_t *swbt_daemon_host_output_handler(swbt_daemon_host_t *host) {
    return host == NULL ? NULL : &host->output_handler;
}

static int swbt_daemon_noop_ipc_start(void *context, swbt_app_t *app) {
    (void)context;
    (void)app;
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
                                               swbt_daemon_host_state_provider_t state_provider,
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

const swbt_daemon_host_backend_t *swbt_daemon_host_noop_backend(void) {
    static const swbt_daemon_host_backend_t backend = {
        .daemon_backend = SWBT_APP_DAEMON_BACKEND_NOOP,
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

int swbt_daemon_main_with_host_backend(const swbt_daemon_config_t *config,
                                       const swbt_daemon_host_backend_t *backend,
                                       void *backend_context) {
    swbt_daemon_host_t host;

    if (swbt_daemon_host_init(&host, config, backend, backend_context) != SWBT_DAEMON_HOST_OK) {
        return 1;
    }
    if (swbt_daemon_host_start(&host) != SWBT_DAEMON_HOST_OK) {
        swbt_daemon_host_destroy(&host);
        return 1;
    }

    swbt_daemon_host_destroy(&host);
    return 0;
}
