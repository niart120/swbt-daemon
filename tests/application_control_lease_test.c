#include <stdbool.h>
#include <stdint.h>

#include "application/control_lease.h"

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_false(bool value) {
    return value ? 1 : 0;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u64(uint64_t actual, uint64_t expected) {
    return actual == expected ? 0 : 1;
}

int main(void) {
    swbt_control_lease_t lease;
    swbt_control_lease_snapshot_t snapshot;
    int failed = 0;

    swbt_control_lease_init(&lease);
    snapshot = swbt_control_lease_snapshot(&lease);
    failed += expect_false(snapshot.has_owner);
    failed += expect_eq_u32(snapshot.owner_client_id, 0u);
    failed += expect_eq_u64(snapshot.last_sequence, 0u);

    failed += expect_eq_int(swbt_control_lease_acquire(&lease, 1001u), SWBT_CONTROL_LEASE_OK);
    failed += expect_eq_int(swbt_control_lease_acquire(&lease, 1001u), SWBT_CONTROL_LEASE_OK);
    failed += expect_eq_int(swbt_control_lease_acquire(&lease, 2002u),
                            SWBT_CONTROL_LEASE_ERROR_OWNER_BUSY);
    failed += expect_eq_int(swbt_control_lease_accept_sequence(&lease, 2002u, 77u),
                            SWBT_CONTROL_LEASE_ERROR_NOT_OWNER);
    failed += expect_eq_int(swbt_control_lease_accept_sequence(&lease, 1001u, 77u),
                            SWBT_CONTROL_LEASE_OK);
    snapshot = swbt_control_lease_snapshot(&lease);
    failed += expect_true(snapshot.has_owner);
    failed += expect_eq_u32(snapshot.owner_client_id, 1001u);
    failed += expect_eq_u64(snapshot.last_sequence, 77u);

    failed += expect_eq_int(swbt_control_lease_release(&lease, 2002u),
                            SWBT_CONTROL_LEASE_ERROR_NOT_OWNER);
    failed += expect_eq_int(swbt_control_lease_release(&lease, 1001u), SWBT_CONTROL_LEASE_OK);
    snapshot = swbt_control_lease_snapshot(&lease);
    failed += expect_false(snapshot.has_owner);
    failed += expect_eq_u64(snapshot.last_sequence, 0u);

    failed += expect_eq_int(swbt_control_lease_acquire(&lease, 2002u), SWBT_CONTROL_LEASE_OK);
    failed += expect_eq_int(swbt_control_lease_accept_sequence(&lease, 2002u, 88u),
                            SWBT_CONTROL_LEASE_OK);
    failed += expect_false(swbt_control_lease_revoke_if_owner(&lease, 1001u));
    failed += expect_true(swbt_control_lease_revoke_if_owner(&lease, 2002u));
    snapshot = swbt_control_lease_snapshot(&lease);
    failed += expect_false(snapshot.has_owner);
    failed += expect_eq_u32(snapshot.owner_client_id, 0u);
    failed += expect_eq_u64(snapshot.last_sequence, 0u);

    return failed == 0 ? 0 : 1;
}
