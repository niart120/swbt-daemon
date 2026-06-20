#ifndef SWBT_CORE_STATE_MAILBOX_H
#define SWBT_CORE_STATE_MAILBOX_H

#include <stdbool.h>
#include <stdint.h>

#include "core/spin_lock.h"
#include "switch/switch_controller_state.h"

typedef enum {
    SWBT_STATE_MAILBOX_OK = 0,
    SWBT_STATE_MAILBOX_ERROR_INVALID_ARGUMENT = -1,
} swbt_state_mailbox_result_t;

typedef struct {
    swbt_spin_lock_t lock;
    swbt_state_t state;
    uint64_t generation;
    uint64_t loaded_generation;
} swbt_state_mailbox_t;

typedef struct {
    swbt_state_t state;
    uint64_t generation;
    uint64_t coalesced_updates;
    bool has_update;
} swbt_state_mailbox_snapshot_t;

swbt_state_mailbox_result_t swbt_state_mailbox_init(swbt_state_mailbox_t *mailbox);

swbt_state_mailbox_result_t swbt_state_mailbox_store(swbt_state_mailbox_t *mailbox,
                                                     const swbt_state_t *state);

swbt_state_mailbox_result_t swbt_state_mailbox_load(swbt_state_mailbox_t *mailbox,
                                                    swbt_state_mailbox_snapshot_t *out_snapshot);

#endif
