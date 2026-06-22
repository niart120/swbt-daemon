#include "application/app.h"

#include <stddef.h>

static swbt_app_result_t swbt_app_map_lease_result(swbt_control_lease_result_t result) {
    switch (result) {
    case SWBT_CONTROL_LEASE_OK:
        return SWBT_APP_OK;
    case SWBT_CONTROL_LEASE_ERROR_OWNER_BUSY:
        return SWBT_APP_ERROR_OWNER_BUSY;
    case SWBT_CONTROL_LEASE_ERROR_NOT_OWNER:
        return SWBT_APP_ERROR_NOT_OWNER;
    }
    return SWBT_APP_ERROR_INVALID_ARGUMENT;
}

static void swbt_app_apply_neutral(swbt_app_t *app) {
    app->state = swbt_state_neutral();
}

static void swbt_app_revoke_all(swbt_app_t *app) {
    swbt_control_lease_revoke(&app->lease);
    swbt_app_apply_neutral(app);
}

swbt_app_result_t swbt_app_init(swbt_app_t *app) {
    if (app == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    swbt_control_lease_init(&app->lease);
    swbt_app_apply_neutral(app);
    return SWBT_APP_OK;
}

swbt_app_result_t swbt_app_acquire(swbt_app_t *app, uint32_t client_id) {
    if (app == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    return swbt_app_map_lease_result(swbt_control_lease_acquire(&app->lease, client_id));
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_app_result_t swbt_app_set_state(swbt_app_t *app, uint32_t client_id, const swbt_state_t *state,
                                     uint64_t sequence) {
    if (app == NULL || state == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    const swbt_control_lease_snapshot_t lease = swbt_control_lease_snapshot(&app->lease);
    if (!lease.has_owner || lease.owner_client_id != client_id) {
        return SWBT_APP_ERROR_NOT_OWNER;
    }
    if (sequence < lease.last_sequence) {
        return SWBT_APP_ERROR_STALE_SEQUENCE;
    }

    const swbt_app_result_t sequence_result = swbt_app_map_lease_result(
        swbt_control_lease_accept_sequence(&app->lease, client_id, sequence));
    if (sequence_result != SWBT_APP_OK) {
        return sequence_result;
    }

    app->state = *state;
    return SWBT_APP_OK;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_app_result_t swbt_app_revoke(swbt_app_t *app, swbt_app_revoke_reason_t reason,
                                  uint32_t client_id) {
    if (app == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    switch (reason) {
    case SWBT_APP_REVOKE_RELEASE:
        if (swbt_control_lease_release(&app->lease, client_id) != SWBT_CONTROL_LEASE_OK) {
            return SWBT_APP_ERROR_NOT_OWNER;
        }
        swbt_app_apply_neutral(app);
        return SWBT_APP_OK;
    case SWBT_APP_REVOKE_DISCONNECT:
    case SWBT_APP_REVOKE_HEARTBEAT_TIMEOUT:
        if (swbt_control_lease_revoke_if_owner(&app->lease, client_id)) {
            swbt_app_apply_neutral(app);
        }
        return SWBT_APP_OK;
    case SWBT_APP_REVOKE_SHUTDOWN:
        swbt_app_revoke_all(app);
        return SWBT_APP_OK;
    }

    return SWBT_APP_ERROR_INVALID_ARGUMENT;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

swbt_app_result_t swbt_app_get_status(const swbt_app_t *app, swbt_app_status_t *out_status) {
    if (app == NULL || out_status == NULL) {
        return SWBT_APP_ERROR_INVALID_ARGUMENT;
    }

    const swbt_control_lease_snapshot_t lease = swbt_control_lease_snapshot(&app->lease);
    out_status->has_owner = lease.has_owner;
    out_status->owner_client_id = lease.owner_client_id;
    out_status->last_sequence = lease.last_sequence;
    out_status->state = app->state;
    return SWBT_APP_OK;
}
