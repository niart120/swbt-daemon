#include "domain/lease.h"

#include <stddef.h>

void swbt_domain_lease_init(swbt_domain_lease_t *lease) {
    if (lease == NULL) {
        return;
    }

    lease->has_owner = false;
    lease->owner_client_id = 0;
    lease->last_sequence = 0;
}

swbt_domain_lease_snapshot_t swbt_domain_lease_snapshot(const swbt_domain_lease_t *lease) {
    if (lease == NULL) {
        return (swbt_domain_lease_snapshot_t){0};
    }

    return (swbt_domain_lease_snapshot_t){
        .has_owner = lease->has_owner,
        .owner_client_id = lease->owner_client_id,
        .last_sequence = lease->last_sequence,
    };
}

swbt_domain_lease_result_t swbt_domain_lease_acquire(swbt_domain_lease_t *lease,
                                                     uint32_t client_id) {
    if (lease == NULL) {
        return SWBT_DOMAIN_LEASE_ERROR_NOT_OWNER;
    }
    if (lease->has_owner && lease->owner_client_id != client_id) {
        return SWBT_DOMAIN_LEASE_ERROR_OWNER_BUSY;
    }

    lease->has_owner = true;
    lease->owner_client_id = client_id;
    return SWBT_DOMAIN_LEASE_OK;
}

swbt_domain_lease_result_t
swbt_domain_lease_accept_sequence(swbt_domain_lease_t *lease,
                                  swbt_domain_lease_accept_sequence_options_t options) {
    const uint32_t client_id = options.client_id;
    if (lease == NULL || !lease->has_owner || lease->owner_client_id != client_id) {
        return SWBT_DOMAIN_LEASE_ERROR_NOT_OWNER;
    }

    lease->last_sequence = options.sequence;
    return SWBT_DOMAIN_LEASE_OK;
}

swbt_domain_lease_result_t swbt_domain_lease_release(swbt_domain_lease_t *lease,
                                                     uint32_t client_id) {
    if (lease == NULL || !lease->has_owner || lease->owner_client_id != client_id) {
        return SWBT_DOMAIN_LEASE_ERROR_NOT_OWNER;
    }

    swbt_domain_lease_revoke(lease);
    return SWBT_DOMAIN_LEASE_OK;
}

bool swbt_domain_lease_revoke_if_owner(swbt_domain_lease_t *lease, uint32_t client_id) {
    if (lease == NULL || !lease->has_owner || lease->owner_client_id != client_id) {
        return false;
    }

    swbt_domain_lease_revoke(lease);
    return true;
}

void swbt_domain_lease_revoke(swbt_domain_lease_t *lease) {
    if (lease == NULL) {
        return;
    }

    lease->has_owner = false;
    lease->owner_client_id = 0;
    lease->last_sequence = 0;
}
