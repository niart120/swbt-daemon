#ifndef SWBT_CONTROL_CONTROL_H
#define SWBT_CONTROL_CONTROL_H

#include <stdint.h>

#include "application/app.h"
#include "runtime/host.h"
#include "switch/switch_controller_state.h"

typedef enum {
    SWBT_CONTROL_OK = 0,
    SWBT_CONTROL_ERROR_INVALID_ARGUMENT = -1,
    SWBT_CONTROL_ERROR_OWNER_BUSY = -2,
    SWBT_CONTROL_ERROR_NOT_OWNER = -3,
} swbt_control_result_t;

typedef struct {
    swbt_app_t *app;
    swbt_runtime_host_t *runtime;
} swbt_control_config_t;

typedef struct {
    swbt_app_t *app;
    swbt_runtime_host_t *runtime;
} swbt_control_t;

swbt_control_result_t swbt_control_init(swbt_control_t *control,
                                        const swbt_control_config_t *config);

swbt_control_result_t swbt_control_acquire_client(swbt_control_t *control, uint32_t client_id);

swbt_control_result_t swbt_control_release_client(swbt_control_t *control, uint32_t client_id);

swbt_control_result_t swbt_control_submit_client_state(swbt_control_t *control, uint32_t client_id,
                                                       const swbt_state_t *state,
                                                       uint64_t sequence);

#endif
