#ifndef SWBT_DOMAIN_LEASE_H
#define SWBT_DOMAIN_LEASE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SWBT_DOMAIN_LEASE_OK = 0,
    SWBT_DOMAIN_LEASE_ERROR_OWNER_BUSY = -1,
    SWBT_DOMAIN_LEASE_ERROR_NOT_OWNER = -2,
} swbt_domain_lease_result_t;

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    uint64_t last_sequence;
} swbt_domain_lease_snapshot_t;

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    uint64_t last_sequence;
} swbt_domain_lease_t;

typedef struct {
    uint32_t client_id;
    uint64_t sequence;
} swbt_domain_lease_accept_sequence_options_t;

void swbt_domain_lease_init(swbt_domain_lease_t *lease);
swbt_domain_lease_snapshot_t swbt_domain_lease_snapshot(const swbt_domain_lease_t *lease);
swbt_domain_lease_result_t swbt_domain_lease_acquire(swbt_domain_lease_t *lease,
                                                     uint32_t client_id);
swbt_domain_lease_result_t
swbt_domain_lease_accept_sequence(swbt_domain_lease_t *lease,
                                  swbt_domain_lease_accept_sequence_options_t options);
swbt_domain_lease_result_t swbt_domain_lease_release(swbt_domain_lease_t *lease,
                                                     uint32_t client_id);
bool swbt_domain_lease_revoke_if_owner(swbt_domain_lease_t *lease, uint32_t client_id);
void swbt_domain_lease_revoke(swbt_domain_lease_t *lease);

#endif
