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

    if (runtime == NULL ||
        swbt_domain_read_controller_state(runtime->app, &state) != SWBT_DOMAIN_OK) {
        return swbt_state_neutral();
    }
    return state;
}

static void swbt_runtime_host_on_output_report(void *context, uint16_t hid_cid,
                                               const swbt_switch_output_report_t *report) {
    swbt_runtime_host_t *runtime = context;
    swbt_switch_subcommand_dispatcher_response_t response;
    swbt_switch_device_info_t device_info;

    if (runtime == NULL || report == NULL) {
        return;
    }

    device_info = runtime->device_info;
    if (runtime->backend->read_device_info != NULL) {
        swbt_switch_device_info_t backend_device_info;
        if (runtime->backend->read_device_info(runtime->backend_context, &backend_device_info) ==
            0) {
            device_info = backend_device_info;
        }
    }

    if (swbt_domain_handle_output_report(
            runtime->app, report, &runtime->report_options, &device_info,
            runtime->backend->time_ms(runtime->backend_context), &response) != SWBT_DOMAIN_OK ||
        response.action != SWBT_SWITCH_SUBCOMMAND_DISPATCH_ACTION_REPLY) {
        return;
    }

    (void)runtime->backend->subcommand_reply_enqueue(runtime->backend_context, hid_cid,
                                                     response.report, response.report_size);
}

static void swbt_runtime_host_store_neutral(swbt_runtime_host_t *runtime) {
    (void)swbt_domain_revoke(runtime->app, (swbt_domain_revoke_options_t){
                                               .reason = SWBT_DOMAIN_REVOKE_SHUTDOWN,
                                               .client_id = 0u,
                                           });
}

static int swbt_runtime_host_read_status_for_control(void *context,
                                                     swbt_control_runtime_status_t *out_status) {
    swbt_runtime_host_t *runtime = context;

    if (runtime == NULL || out_status == NULL) {
        return -1;
    }

    *out_status = (swbt_control_runtime_status_t){
        .initialized = runtime->initialized,
        .running = runtime->running,
        .hid_registered = runtime->hid_registered,
        .output_handler_started = runtime->output_handler_started,
        .report_timer_started = runtime->report_timer_started,
    };
    return 0;
}

swbt_runtime_host_result_t swbt_runtime_host_init(swbt_runtime_host_t *runtime,
                                                  const swbt_runtime_host_config_t *config,
                                                  const swbt_runtime_host_backend_t *backend,
                                                  void *backend_context) {
    if (runtime == NULL || config == NULL || !swbt_runtime_backend_is_valid(backend)) {
        return SWBT_RUNTIME_HOST_ERROR_INVALID_ARGUMENT;
    }

    *runtime = (swbt_runtime_host_t){0};
    runtime->backend = backend;
    runtime->backend_context = backend_context;
    runtime->app = swbt_domain_create();
    if (runtime->app == NULL) {
        return SWBT_RUNTIME_HOST_ERROR_INVALID_ARGUMENT;
    }
    if (swbt_domain_set_daemon_status(runtime->app, &config->daemon_status) != SWBT_DOMAIN_OK) {
        swbt_domain_destroy(runtime->app);
        runtime->app = NULL;
        return SWBT_RUNTIME_HOST_ERROR_INVALID_ARGUMENT;
    }
    runtime->report_options = config->report_options;
    runtime->device_info = config->device_info;
    swbt_btstack_output_report_handler_init(&runtime->output_handler,
                                            swbt_runtime_host_on_output_report, runtime);
    if (swbt_control_init(&runtime->control,
                          &(swbt_control_config_t){
                              .app = runtime->app,
                              .read_runtime_status = swbt_runtime_host_read_status_for_control,
                              .runtime_status_context = runtime,
                          }) != SWBT_CONTROL_OK) {
        swbt_domain_destroy(runtime->app);
        runtime->app = NULL;
        return SWBT_RUNTIME_HOST_ERROR_INVALID_ARGUMENT;
    }
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

    swbt_runtime_host_store_neutral(runtime);
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

    swbt_runtime_host_store_neutral(runtime);

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

void swbt_runtime_host_destroy(swbt_runtime_host_t *runtime) {
    if (runtime == NULL) {
        return;
    }

    swbt_runtime_host_stop(runtime);
    swbt_domain_destroy(runtime->app);
    *runtime = (swbt_runtime_host_t){0};
}

bool swbt_runtime_host_is_running(const swbt_runtime_host_t *runtime) {
    return runtime != NULL && runtime->running;
}

swbt_runtime_host_result_t swbt_runtime_host_status(const swbt_runtime_host_t *runtime,
                                                    swbt_runtime_host_status_t *out_status) {
    if (runtime == NULL || out_status == NULL) {
        return SWBT_RUNTIME_HOST_ERROR_INVALID_ARGUMENT;
    }

    *out_status = (swbt_runtime_host_status_t){
        .initialized = runtime->initialized,
        .running = runtime->running,
        .hid_registered = runtime->hid_registered,
        .output_handler_started = runtime->output_handler_started,
        .report_timer_started = runtime->report_timer_started,
    };
    return SWBT_RUNTIME_HOST_OK;
}

swbt_btstack_output_report_handler_t *
swbt_runtime_host_output_handler(swbt_runtime_host_t *runtime) {
    return runtime == NULL ? NULL : &runtime->output_handler;
}

swbt_domain_t *swbt_runtime_host_app(swbt_runtime_host_t *runtime) {
    return runtime == NULL ? NULL : runtime->app;
}

swbt_control_t *swbt_runtime_host_control(swbt_runtime_host_t *runtime) {
    return runtime == NULL ? NULL : &runtime->control;
}
