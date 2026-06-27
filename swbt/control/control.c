#include "control/control.h"

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
