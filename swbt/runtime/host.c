#include "runtime/host.h"

#include <stddef.h>

static bool swbt_runtime_backend_is_valid(const swbt_runtime_host_backend_t *backend) {
    return backend != NULL && backend->hid_register != NULL && backend->hid_stop != NULL &&
           backend->output_handler_start != NULL && backend->output_handler_stop != NULL &&
           backend->report_timer_start != NULL && backend->report_timer_stop != NULL &&
           backend->report_timer_send_neutral_now != NULL &&
           backend->subcommand_reply_enqueue != NULL && backend->time_ms != NULL;
}

static swbt_state_t swbt_runtime_host_read_state(void *context) {
    swbt_runtime_host_t *runtime = context;
    swbt_state_t state;

    if (runtime == NULL || swbt_app_read_controller_state(runtime->app, &state) != SWBT_APP_OK) {
        return swbt_state_neutral();
    }
    return state;
}

static void swbt_runtime_host_on_output_report(void *context, uint16_t hid_cid,
                                               const swbt_switch_output_report_t *report) {
    (void)context;
    (void)hid_cid;
    (void)report;
}

swbt_runtime_host_result_t swbt_runtime_host_init(swbt_runtime_host_t *runtime,
                                                  const swbt_runtime_host_config_t *config,
                                                  const swbt_runtime_host_backend_t *backend,
                                                  void *backend_context) {
    if (runtime == NULL || config == NULL || config->app == NULL ||
        !swbt_runtime_backend_is_valid(backend)) {
        return SWBT_RUNTIME_HOST_ERROR_INVALID_ARGUMENT;
    }

    *runtime = (swbt_runtime_host_t){0};
    runtime->backend = backend;
    runtime->backend_context = backend_context;
    runtime->app = config->app;
    swbt_btstack_output_report_handler_init(&runtime->output_handler,
                                            swbt_runtime_host_on_output_report, runtime);
    runtime->initialized = true;
    return SWBT_RUNTIME_HOST_OK;
}

swbt_runtime_host_result_t swbt_runtime_host_start(swbt_runtime_host_t *runtime) {
    if (runtime == NULL || !runtime->initialized) {
        return SWBT_RUNTIME_HOST_ERROR_INVALID_ARGUMENT;
    }
    if (runtime->running) {
        return SWBT_RUNTIME_HOST_OK;
    }

    if (runtime->backend->hid_register(runtime->backend_context) != 0) {
        return SWBT_RUNTIME_HOST_ERROR_BACKEND;
    }
    runtime->hid_registered = true;

    runtime->backend->output_handler_start(runtime->backend_context, &runtime->output_handler);
    runtime->output_handler_started = true;

    if (runtime->backend->report_timer_start(runtime->backend_context, swbt_runtime_host_read_state,
                                             runtime) != 0) {
        swbt_runtime_host_stop(runtime);
        return SWBT_RUNTIME_HOST_ERROR_BACKEND;
    }
    runtime->report_timer_started = true;
    runtime->running = true;
    return SWBT_RUNTIME_HOST_OK;
}

swbt_runtime_host_result_t swbt_runtime_host_send_neutral_now(swbt_runtime_host_t *runtime) {
    if (runtime == NULL || !runtime->initialized) {
        return SWBT_RUNTIME_HOST_ERROR_INVALID_ARGUMENT;
    }
    if (!runtime->report_timer_started) {
        return SWBT_RUNTIME_HOST_OK;
    }

    const int send_result =
        runtime->backend->report_timer_send_neutral_now(runtime->backend_context);
    if (send_result > 0) {
        return SWBT_RUNTIME_HOST_PENDING;
    }
    if (send_result != 0) {
        return SWBT_RUNTIME_HOST_ERROR_BACKEND;
    }
    return SWBT_RUNTIME_HOST_OK;
}

void swbt_runtime_host_stop(swbt_runtime_host_t *runtime) {
    if (runtime == NULL || !runtime->initialized) {
        return;
    }

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
    runtime->running = false;
}

bool swbt_runtime_host_is_running(const swbt_runtime_host_t *runtime) {
    return runtime != NULL && runtime->running;
}

swbt_btstack_output_report_handler_t *
swbt_runtime_host_output_handler(swbt_runtime_host_t *runtime) {
    return runtime == NULL ? NULL : &runtime->output_handler;
}
