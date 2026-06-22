#ifndef SWBT_APPLICATION_APP_H
#define SWBT_APPLICATION_APP_H

#include <stdbool.h>
#include <stdint.h>

#include "application/control_lease.h"
#include "switch/switch_controller_state.h"

typedef enum {
    SWBT_APP_OK = 0,
    SWBT_APP_ERROR_INVALID_ARGUMENT = -1,
    SWBT_APP_ERROR_OWNER_BUSY = -2,
    SWBT_APP_ERROR_NOT_OWNER = -3,
    SWBT_APP_ERROR_STALE_SEQUENCE = -4,
} swbt_app_result_t;

typedef enum {
    SWBT_APP_REVOKE_RELEASE = 0,
    SWBT_APP_REVOKE_DISCONNECT,
    SWBT_APP_REVOKE_HEARTBEAT_TIMEOUT,
    SWBT_APP_REVOKE_SHUTDOWN,
} swbt_app_revoke_reason_t;

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    uint64_t last_sequence;
    swbt_state_t state;
} swbt_app_status_t;

typedef struct {
    swbt_control_lease_t lease;
    swbt_state_t state;
} swbt_app_t;

swbt_app_result_t swbt_app_init(swbt_app_t *app);
swbt_app_result_t swbt_app_acquire(swbt_app_t *app, uint32_t client_id);
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_app_result_t swbt_app_set_state(swbt_app_t *app, uint32_t client_id, const swbt_state_t *state,
                                     uint64_t sequence);
// NOLINTEND(bugprone-easily-swappable-parameters)
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_app_result_t swbt_app_revoke(swbt_app_t *app, swbt_app_revoke_reason_t reason,
                                  uint32_t client_id);
// NOLINTEND(bugprone-easily-swappable-parameters)
swbt_app_result_t swbt_app_get_status(const swbt_app_t *app, swbt_app_status_t *out_status);

#endif
