#include "control/control.h"

static const uint32_t swbt_control_direct_client_id = UINT32_MAX;

static swbt_control_result_t swbt_control_map_app_result(swbt_app_result_t result) {
    switch (result) {
    case SWBT_APP_OK:
    case SWBT_APP_ERROR_STALE_SEQUENCE:
        return SWBT_CONTROL_OK;
    case SWBT_APP_ERROR_OWNER_BUSY:
        return SWBT_CONTROL_ERROR_OWNER_BUSY;
    case SWBT_APP_ERROR_NOT_OWNER:
        return SWBT_CONTROL_ERROR_NOT_OWNER;
    case SWBT_APP_ERROR_INVALID_ARGUMENT:
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }
    return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
}

swbt_control_result_t swbt_control_init(swbt_control_t *control,
                                        const swbt_control_config_t *config) {
    if (control == NULL || config == NULL || config->app == NULL) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }

    *control = (swbt_control_t){
        .app = config->app,
        .runtime = config->runtime,
        .next_direct_sequence = 0u,
    };
    return SWBT_CONTROL_OK;
}

swbt_control_result_t swbt_control_acquire_client(swbt_control_t *control, uint32_t client_id) {
    if (control == NULL || control->app == NULL) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }

    return swbt_control_map_app_result(swbt_app_acquire(control->app, client_id));
}

swbt_control_result_t swbt_control_release_client(swbt_control_t *control, uint32_t client_id) {
    if (control == NULL || control->app == NULL) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }

    return swbt_control_map_app_result(
        swbt_app_revoke(control->app, (swbt_app_revoke_options_t){
                                          .reason = SWBT_APP_REVOKE_RELEASE,
                                          .client_id = client_id,
                                      }));
}

swbt_control_result_t swbt_control_disconnect_client(swbt_control_t *control, uint32_t client_id) {
    if (control == NULL || control->app == NULL) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }

    return swbt_control_map_app_result(
        swbt_app_revoke(control->app, (swbt_app_revoke_options_t){
                                          .reason = SWBT_APP_REVOKE_DISCONNECT,
                                          .client_id = client_id,
                                      }));
}

swbt_control_result_t swbt_control_heartbeat_timeout_client(swbt_control_t *control,
                                                            uint32_t client_id) {
    if (control == NULL || control->app == NULL) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }

    return swbt_control_map_app_result(
        swbt_app_revoke(control->app, (swbt_app_revoke_options_t){
                                          .reason = SWBT_APP_REVOKE_HEARTBEAT_TIMEOUT,
                                          .client_id = client_id,
                                      }));
}

swbt_control_result_t swbt_control_shutdown(swbt_control_t *control) {
    if (control == NULL || control->app == NULL) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }

    return swbt_control_map_app_result(
        swbt_app_revoke(control->app, (swbt_app_revoke_options_t){
                                          .reason = SWBT_APP_REVOKE_SHUTDOWN,
                                          .client_id = 0u,
                                      }));
}

swbt_control_result_t swbt_control_record_state_update_rejected(swbt_control_t *control) {
    if (control == NULL || control->app == NULL) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }

    return swbt_control_map_app_result(swbt_app_record_state_update_rejected(control->app));
}

swbt_control_result_t swbt_control_submit_client_state(swbt_control_t *control, uint32_t client_id,
                                                       const swbt_state_t *state,
                                                       uint64_t sequence) {
    if (control == NULL || control->app == NULL || state == NULL) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }

    return swbt_control_map_app_result(
        swbt_app_set_state(control->app, (swbt_app_set_state_options_t){
                                             .client_id = client_id,
                                             .state = state,
                                             .sequence = sequence,
                                         }));
}

swbt_control_result_t swbt_control_submit_state(swbt_control_t *control,
                                                const swbt_state_t *state) {
    if (control == NULL || control->app == NULL || state == NULL) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }

    const swbt_control_result_t acquire_result =
        swbt_control_map_app_result(swbt_app_acquire(control->app, swbt_control_direct_client_id));
    if (acquire_result != SWBT_CONTROL_OK) {
        return acquire_result;
    }

    control->next_direct_sequence += 1u;
    return swbt_control_map_app_result(
        swbt_app_set_state(control->app, (swbt_app_set_state_options_t){
                                             .client_id = swbt_control_direct_client_id,
                                             .state = state,
                                             .sequence = control->next_direct_sequence,
                                         }));
}

swbt_control_result_t swbt_control_submit_neutral(swbt_control_t *control) {
    const swbt_state_t neutral = swbt_state_neutral();

    return swbt_control_submit_state(control, &neutral);
}

swbt_control_result_t swbt_control_get_status(const swbt_control_t *control,
                                              swbt_control_status_t *out_status) {
    if (control == NULL || control->app == NULL || out_status == NULL) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }

    *out_status = (swbt_control_status_t){0};
    if (swbt_app_read_status(control->app, &out_status->app) != SWBT_APP_OK) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }
    if (control->runtime == NULL) {
        return SWBT_CONTROL_OK;
    }
    if (swbt_runtime_host_status(control->runtime, &out_status->runtime) != SWBT_RUNTIME_HOST_OK) {
        return SWBT_CONTROL_ERROR_INVALID_ARGUMENT;
    }
    out_status->has_runtime_status = true;
    return SWBT_CONTROL_OK;
}
