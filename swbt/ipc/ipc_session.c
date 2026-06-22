#include "ipc/ipc_session.h"

#include <stddef.h>

static void swbt_ipc_apply_neutral_unlocked(swbt_ipc_session_t *session) {
    session->state = swbt_state_neutral();
}

static swbt_ipc_result_t swbt_ipc_map_lease_result(swbt_control_lease_result_t result) {
    switch (result) {
    case SWBT_CONTROL_LEASE_OK:
        return SWBT_IPC_OK;
    case SWBT_CONTROL_LEASE_ERROR_OWNER_BUSY:
        return SWBT_IPC_ERROR_OWNER_BUSY;
    case SWBT_CONTROL_LEASE_ERROR_NOT_OWNER:
        return SWBT_IPC_ERROR_NOT_OWNER;
    }
    return SWBT_IPC_ERROR_INVALID_ARGUMENT;
}

static swbt_ipc_result_t swbt_ipc_publish_state_unlocked(swbt_ipc_session_t *session) {
    if (session->mailbox == NULL) {
        return SWBT_IPC_OK;
    }
    return swbt_state_mailbox_store(session->mailbox, &session->state) == SWBT_STATE_MAILBOX_OK
               ? SWBT_IPC_OK
               : SWBT_IPC_ERROR_INVALID_ARGUMENT;
}

static swbt_ipc_result_t
swbt_ipc_publish_neutral_after_revoke_unlocked(swbt_ipc_session_t *session) {
    swbt_ipc_apply_neutral_unlocked(session);
    return swbt_ipc_publish_state_unlocked(session);
}

static swbt_ipc_result_t swbt_ipc_clear_owner_unlocked(swbt_ipc_session_t *session) {
    swbt_control_lease_revoke(&session->lease);
    return swbt_ipc_publish_neutral_after_revoke_unlocked(session);
}

swbt_ipc_result_t swbt_ipc_session_init(swbt_ipc_session_t *session) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_init(&session->lock);
    swbt_control_lease_init(&session->lease);
    session->mailbox = NULL;
    swbt_ipc_apply_neutral_unlocked(session);
    if (swbt_switch_rumble_init(&session->rumble) != SWBT_SWITCH_RUMBLE_OK) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    return SWBT_IPC_OK;
}

swbt_ipc_result_t swbt_ipc_session_bind_mailbox(swbt_ipc_session_t *session,
                                                swbt_state_mailbox_t *mailbox) {
    if (session == NULL || mailbox == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&session->lock);
    session->mailbox = mailbox;
    const swbt_ipc_result_t result = swbt_ipc_publish_state_unlocked(session);
    swbt_spin_lock_release(&session->lock);
    return result;
}

swbt_ipc_result_t swbt_ipc_acquire(swbt_ipc_session_t *session, uint32_t client_id) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    swbt_spin_lock_acquire(&session->lock);
    const swbt_ipc_result_t result =
        swbt_ipc_map_lease_result(swbt_control_lease_acquire(&session->lease, client_id));
    swbt_spin_lock_release(&session->lock);
    return result;
}

swbt_ipc_result_t swbt_ipc_release(swbt_ipc_session_t *session, uint32_t client_id) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    swbt_spin_lock_acquire(&session->lock);
    if (swbt_control_lease_release(&session->lease, client_id) != SWBT_CONTROL_LEASE_OK) {
        swbt_spin_lock_release(&session->lock);
        return SWBT_IPC_ERROR_NOT_OWNER;
    }

    const swbt_ipc_result_t result = swbt_ipc_publish_neutral_after_revoke_unlocked(session);
    swbt_spin_lock_release(&session->lock);
    return result;
}

swbt_ipc_result_t swbt_ipc_clear_owner(swbt_ipc_session_t *session) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&session->lock);
    const swbt_ipc_result_t result = swbt_ipc_clear_owner_unlocked(session);
    swbt_spin_lock_release(&session->lock);
    return result;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_ipc_result_t swbt_ipc_set_state(swbt_ipc_session_t *session, uint32_t client_id,
                                     const swbt_state_t *state, uint64_t sequence) {
    if (session == NULL || state == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    swbt_spin_lock_acquire(&session->lock);
    const swbt_ipc_result_t owner_result = swbt_ipc_map_lease_result(
        swbt_control_lease_accept_sequence(&session->lease, client_id, sequence));
    if (owner_result != SWBT_IPC_OK) {
        swbt_spin_lock_release(&session->lock);
        return owner_result;
    }

    session->state = *state;
    const swbt_ipc_result_t result = swbt_ipc_publish_state_unlocked(session);
    swbt_spin_lock_release(&session->lock);
    return result;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

swbt_ipc_result_t swbt_ipc_get_status(const swbt_ipc_session_t *session,
                                      swbt_ipc_status_t *out_status) {
    if (session == NULL || out_status == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    swbt_ipc_session_t *mutable_session = (swbt_ipc_session_t *)session;
    swbt_spin_lock_acquire(&mutable_session->lock);
    const swbt_control_lease_snapshot_t lease = swbt_control_lease_snapshot(&session->lease);
    out_status->has_owner = lease.has_owner;
    out_status->owner_client_id = lease.owner_client_id;
    out_status->last_seq = lease.last_sequence;
    out_status->state = session->state;
    out_status->rumble = session->rumble;
    swbt_spin_lock_release(&mutable_session->lock);
    return SWBT_IPC_OK;
}

swbt_ipc_result_t swbt_ipc_record_rumble(swbt_ipc_session_t *session, const uint8_t *payload,
                                         uint64_t updated_at_ms) {
    if (session == NULL || payload == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&session->lock);
    const swbt_ipc_result_t result =
        swbt_switch_rumble_update(&session->rumble, payload, updated_at_ms) == SWBT_SWITCH_RUMBLE_OK
            ? SWBT_IPC_OK
            : SWBT_IPC_ERROR_INVALID_ARGUMENT;
    swbt_spin_lock_release(&session->lock);
    return result;
}

swbt_ipc_result_t
swbt_ipc_record_output_report_rumble(swbt_ipc_session_t *session,
                                     const swbt_switch_output_report_t *output_report,
                                     uint64_t updated_at_ms) {
    if (output_report == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    return swbt_ipc_record_rumble(session, output_report->rumble, updated_at_ms);
}

swbt_ipc_result_t swbt_ipc_disconnect(swbt_ipc_session_t *session, uint32_t client_id) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    swbt_spin_lock_acquire(&session->lock);
    if (swbt_control_lease_revoke_if_owner(&session->lease, client_id)) {
        const swbt_ipc_result_t result = swbt_ipc_publish_neutral_after_revoke_unlocked(session);
        swbt_spin_lock_release(&session->lock);
        return result;
    }
    swbt_spin_lock_release(&session->lock);
    return SWBT_IPC_OK;
}

swbt_ipc_result_t swbt_ipc_heartbeat_timeout(swbt_ipc_session_t *session, uint32_t client_id) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    swbt_spin_lock_acquire(&session->lock);
    if (swbt_control_lease_revoke_if_owner(&session->lease, client_id)) {
        const swbt_ipc_result_t result = swbt_ipc_publish_neutral_after_revoke_unlocked(session);
        swbt_spin_lock_release(&session->lock);
        return result;
    }
    swbt_spin_lock_release(&session->lock);
    return SWBT_IPC_OK;
}
