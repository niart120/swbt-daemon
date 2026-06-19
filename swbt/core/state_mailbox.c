#include "core/state_mailbox.h"

#include <stddef.h>

static uint64_t swbt_state_mailbox_count_coalesced(uint64_t generation,
                                                   uint64_t loaded_generation) {
    if (generation <= loaded_generation) {
        return 0u;
    }
    const uint64_t delta = generation - loaded_generation;
    return delta > 1u ? delta - 1u : 0u;
}

swbt_state_mailbox_result_t swbt_state_mailbox_init(swbt_state_mailbox_t *mailbox) {
    if (mailbox == NULL) {
        return SWBT_STATE_MAILBOX_ERROR_INVALID_ARGUMENT;
    }

    mailbox->state = swbt_state_neutral();
    mailbox->generation = 0u;
    mailbox->loaded_generation = 0u;
    return SWBT_STATE_MAILBOX_OK;
}

swbt_state_mailbox_result_t swbt_state_mailbox_store(swbt_state_mailbox_t *mailbox,
                                                     const swbt_state_t *state) {
    if (mailbox == NULL || state == NULL) {
        return SWBT_STATE_MAILBOX_ERROR_INVALID_ARGUMENT;
    }

    mailbox->state = *state;
    mailbox->generation += 1u;
    return SWBT_STATE_MAILBOX_OK;
}

swbt_state_mailbox_result_t swbt_state_mailbox_load(swbt_state_mailbox_t *mailbox,
                                                    swbt_state_mailbox_snapshot_t *out_snapshot) {
    if (mailbox == NULL || out_snapshot == NULL) {
        return SWBT_STATE_MAILBOX_ERROR_INVALID_ARGUMENT;
    }

    out_snapshot->state = mailbox->state;
    out_snapshot->generation = mailbox->generation;
    out_snapshot->coalesced_updates =
        swbt_state_mailbox_count_coalesced(mailbox->generation, mailbox->loaded_generation);
    out_snapshot->has_update = mailbox->generation != mailbox->loaded_generation;
    mailbox->loaded_generation = mailbox->generation;
    return SWBT_STATE_MAILBOX_OK;
}
