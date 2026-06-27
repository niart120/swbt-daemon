#include "swbt.h"

#include <stddef.h>
#include <stdlib.h>

#include "domain/domain.h"
#include "control/control.h"
#include "support/swbt_version.h"
#include "switch/switch_controller_state.h"

struct swbt {
    swbt_domain_t *app;
    swbt_control_t control;
};

static swbt_result_t swbt_map_control_result(swbt_control_result_t result) {
    switch (result) {
    case SWBT_CONTROL_OK:
        return SWBT_OK;
    case SWBT_CONTROL_ERROR_OWNER_BUSY:
        return SWBT_ERROR_OWNER_BUSY;
    case SWBT_CONTROL_ERROR_NOT_OWNER:
        return SWBT_ERROR_NOT_OWNER;
    case SWBT_CONTROL_ERROR_INVALID_ARGUMENT:
        return SWBT_ERROR_INVALID_ARGUMENT;
    }
    return SWBT_ERROR_INVALID_ARGUMENT;
}

static swbt_state_t swbt_to_internal_state(const swbt_controller_state_t *state) {
    return (swbt_state_t){
        .buttons = state->buttons,
        .lx = state->lx,
        .ly = state->ly,
        .rx = state->rx,
        .ry = state->ry,
        .accel_x = state->accel_x,
        .accel_y = state->accel_y,
        .accel_z = state->accel_z,
        .gyro_x = state->gyro_x,
        .gyro_y = state->gyro_y,
        .gyro_z = state->gyro_z,
    };
}

static swbt_controller_state_t swbt_from_internal_state(swbt_state_t state) {
    return (swbt_controller_state_t){
        .buttons = state.buttons,
        .lx = state.lx,
        .ly = state.ly,
        .rx = state.rx,
        .ry = state.ry,
        .accel_x = state.accel_x,
        .accel_y = state.accel_y,
        .accel_z = state.accel_z,
        .gyro_x = state.gyro_x,
        .gyro_y = state.gyro_y,
        .gyro_z = state.gyro_z,
    };
}

const char *swbt_version_string(void) {
    return swbt_get_version_string();
}

swbt_result_t swbt_open(const swbt_open_options_t *options, swbt_t **out_swbt) {
    (void)options;

    if (out_swbt == NULL) {
        return SWBT_ERROR_INVALID_ARGUMENT;
    }

    *out_swbt = NULL;
    swbt_t *handle = malloc(sizeof(*handle));
    if (handle == NULL) {
        return SWBT_ERROR_NO_MEMORY;
    }

    handle->app = swbt_domain_create();
    if (handle->app == NULL) {
        free(handle);
        return SWBT_ERROR_NO_MEMORY;
    }

    const swbt_control_result_t control_result =
        swbt_control_init(&handle->control, &(swbt_control_config_t){
                                                .app = handle->app,
                                            });
    if (control_result != SWBT_CONTROL_OK) {
        swbt_domain_destroy(handle->app);
        free(handle);
        return swbt_map_control_result(control_result);
    }

    *out_swbt = handle;
    return SWBT_OK;
}

void swbt_close(swbt_t *swbt) {
    if (swbt == NULL) {
        return;
    }

    swbt_domain_destroy(swbt->app);
    free(swbt);
}

swbt_result_t swbt_submit_state(swbt_t *swbt, const swbt_controller_state_t *state) {
    if (swbt == NULL || state == NULL) {
        return SWBT_ERROR_INVALID_ARGUMENT;
    }

    const swbt_state_t internal_state = swbt_to_internal_state(state);
    return swbt_map_control_result(swbt_control_submit_state(&swbt->control, &internal_state));
}

swbt_result_t swbt_submit_neutral(swbt_t *swbt) {
    if (swbt == NULL) {
        return SWBT_ERROR_INVALID_ARGUMENT;
    }

    return swbt_map_control_result(swbt_control_submit_neutral(&swbt->control));
}

swbt_result_t swbt_get_status(swbt_t *swbt, swbt_status_t *out_status) {
    swbt_control_status_t control_status;

    if (swbt == NULL || out_status == NULL) {
        return SWBT_ERROR_INVALID_ARGUMENT;
    }

    const swbt_control_result_t control_result =
        swbt_control_get_status(&swbt->control, &control_status);
    if (control_result != SWBT_CONTROL_OK) {
        return swbt_map_control_result(control_result);
    }

    *out_status = (swbt_status_t){
        .state = swbt_from_internal_state(control_status.app.state),
        .runtime_available = control_status.has_runtime_status,
        .runtime_running = control_status.has_runtime_status && control_status.runtime.running,
    };
    return SWBT_OK;
}
