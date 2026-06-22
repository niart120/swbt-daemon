#ifndef SWBT_APPLICATION_CONTROL_LEASE_H
#define SWBT_APPLICATION_CONTROL_LEASE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SWBT_CONTROL_LEASE_OK = 0,
    SWBT_CONTROL_LEASE_ERROR_OWNER_BUSY = -1,
    SWBT_CONTROL_LEASE_ERROR_NOT_OWNER = -2,
} swbt_control_lease_result_t;

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    uint64_t last_sequence;
} swbt_control_lease_snapshot_t;

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    uint64_t last_sequence;
} swbt_control_lease_t;

void swbt_control_lease_init(swbt_control_lease_t *lease);
swbt_control_lease_snapshot_t swbt_control_lease_snapshot(const swbt_control_lease_t *lease);
swbt_control_lease_result_t swbt_control_lease_acquire(swbt_control_lease_t *lease,
                                                       uint32_t client_id);
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_control_lease_result_t swbt_control_lease_accept_sequence(swbt_control_lease_t *lease,
                                                               uint32_t client_id,
                                                               uint64_t sequence);
// NOLINTEND(bugprone-easily-swappable-parameters)
swbt_control_lease_result_t swbt_control_lease_release(swbt_control_lease_t *lease,
                                                       uint32_t client_id);
bool swbt_control_lease_revoke_if_owner(swbt_control_lease_t *lease, uint32_t client_id);
void swbt_control_lease_revoke(swbt_control_lease_t *lease);

#endif
