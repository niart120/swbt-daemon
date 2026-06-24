#include "application/control_lease.h"

#include <stddef.h>

void swbt_control_lease_init(swbt_control_lease_t *lease) {
    if (lease == NULL) {
        return;
    }

    lease->has_owner = false;
    lease->owner_client_id = 0;
    lease->last_sequence = 0;
}

swbt_control_lease_snapshot_t swbt_control_lease_snapshot(const swbt_control_lease_t *lease) {
    if (lease == NULL) {
        return (swbt_control_lease_snapshot_t){0};
    }

    return (swbt_control_lease_snapshot_t){
        .has_owner = lease->has_owner,
        .owner_client_id = lease->owner_client_id,
        .last_sequence = lease->last_sequence,
    };
}

swbt_control_lease_result_t swbt_control_lease_acquire(swbt_control_lease_t *lease,
                                                       uint32_t client_id) {
    if (lease == NULL) {
        return SWBT_CONTROL_LEASE_ERROR_NOT_OWNER;
    }
    if (lease->has_owner && lease->owner_client_id != client_id) {
        return SWBT_CONTROL_LEASE_ERROR_OWNER_BUSY;
    }

    lease->has_owner = true;
    lease->owner_client_id = client_id;
    return SWBT_CONTROL_LEASE_OK;
}

swbt_control_lease_result_t
swbt_control_lease_accept_sequence(swbt_control_lease_t *lease,
                                   swbt_control_lease_accept_sequence_options_t options) {
    const uint32_t client_id = options.client_id;
    if (lease == NULL || !lease->has_owner || lease->owner_client_id != client_id) {
        return SWBT_CONTROL_LEASE_ERROR_NOT_OWNER;
    }

    lease->last_sequence = options.sequence;
    return SWBT_CONTROL_LEASE_OK;
}

swbt_control_lease_result_t swbt_control_lease_release(swbt_control_lease_t *lease,
                                                       uint32_t client_id) {
    if (lease == NULL || !lease->has_owner || lease->owner_client_id != client_id) {
        return SWBT_CONTROL_LEASE_ERROR_NOT_OWNER;
    }

    swbt_control_lease_revoke(lease);
    return SWBT_CONTROL_LEASE_OK;
}

bool swbt_control_lease_revoke_if_owner(swbt_control_lease_t *lease, uint32_t client_id) {
    if (lease == NULL || !lease->has_owner || lease->owner_client_id != client_id) {
        return false;
    }

    swbt_control_lease_revoke(lease);
    return true;
}

void swbt_control_lease_revoke(swbt_control_lease_t *lease) {
    if (lease == NULL) {
        return;
    }

    lease->has_owner = false;
    lease->owner_client_id = 0;
    lease->last_sequence = 0;
}
