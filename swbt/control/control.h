#ifndef SWBT_CONTROL_CONTROL_H
#define SWBT_CONTROL_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "domain/domain.h"
#include "runtime/host.h"
#include "switch/switch_controller_state.h"

typedef enum {
    SWBT_CONTROL_OK = 0,
    SWBT_CONTROL_ERROR_INVALID_ARGUMENT = -1,
    SWBT_CONTROL_ERROR_OWNER_BUSY = -2,
    SWBT_CONTROL_ERROR_NOT_OWNER = -3,
} swbt_control_result_t;

typedef struct {
    swbt_domain_t *app;
    swbt_runtime_host_t *runtime;
} swbt_control_config_t;

typedef struct {
    swbt_domain_t *app;
    swbt_runtime_host_t *runtime;
    uint64_t next_direct_sequence;
} swbt_control_t;

typedef struct {
    swbt_domain_status_snapshot_t app;
    bool has_runtime_status;
    swbt_runtime_host_status_t runtime;
} swbt_control_status_t;

swbt_control_result_t swbt_control_init(swbt_control_t *control,
                                        const swbt_control_config_t *config);

swbt_control_result_t swbt_control_acquire_client(swbt_control_t *control, uint32_t client_id);

swbt_control_result_t swbt_control_release_client(swbt_control_t *control, uint32_t client_id);

swbt_control_result_t swbt_control_disconnect_client(swbt_control_t *control, uint32_t client_id);

swbt_control_result_t swbt_control_heartbeat_timeout_client(swbt_control_t *control,
                                                            uint32_t client_id);

swbt_control_result_t swbt_control_shutdown(swbt_control_t *control);

swbt_control_result_t swbt_control_record_state_update_rejected(swbt_control_t *control);

swbt_control_result_t swbt_control_submit_client_state(swbt_control_t *control, uint32_t client_id,
                                                       const swbt_state_t *state,
                                                       uint64_t sequence);

swbt_control_result_t swbt_control_submit_state(swbt_control_t *control, const swbt_state_t *state);

swbt_control_result_t swbt_control_submit_neutral(swbt_control_t *control);

swbt_control_result_t swbt_control_get_status(const swbt_control_t *control,
                                              swbt_control_status_t *out_status);

#endif
