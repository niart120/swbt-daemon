#include "ipc/ipc_session.h"

#include <stddef.h>

static swbt_ipc_result_t swbt_ipc_map_app_result(swbt_app_result_t result) {
    switch (result) {
    case SWBT_APP_OK:
    case SWBT_APP_ERROR_STALE_SEQUENCE:
        return SWBT_IPC_OK;
    case SWBT_APP_ERROR_OWNER_BUSY:
        return SWBT_IPC_ERROR_OWNER_BUSY;
    case SWBT_APP_ERROR_NOT_OWNER:
        return SWBT_IPC_ERROR_NOT_OWNER;
    case SWBT_APP_ERROR_INVALID_ARGUMENT:
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    return SWBT_IPC_ERROR_INVALID_ARGUMENT;
}

static swbt_ipc_daemon_status_t swbt_ipc_daemon_status_default(void) {
    return (swbt_ipc_daemon_status_t){
        .backend = SWBT_IPC_DAEMON_BACKEND_UNKNOWN,
        .lifecycle_state = SWBT_IPC_DAEMON_LIFECYCLE_STOPPED,
        .hardware_approval = SWBT_IPC_HARDWARE_APPROVAL_UNAVAILABLE,
    };
}

static swbt_ipc_hardware_status_t swbt_ipc_hardware_status_default(void) {
    return (swbt_ipc_hardware_status_t){
        .adapter_state = SWBT_IPC_HARDWARE_CHANNEL_UNAVAILABLE,
        .switch_connection_state = SWBT_IPC_HARDWARE_CHANNEL_UNAVAILABLE,
        .hid_channel_state = SWBT_IPC_HARDWARE_CHANNEL_UNAVAILABLE,
    };
}

static swbt_ipc_result_t swbt_ipc_publish_state_unlocked(swbt_ipc_session_t *session) {
    swbt_app_status_t app_status;

    if (session->mailbox == NULL) {
        return SWBT_IPC_OK;
    }
    if (swbt_app_get_status(&session->app, &app_status) != SWBT_APP_OK) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    return swbt_state_mailbox_store(session->mailbox, &app_status.state) == SWBT_STATE_MAILBOX_OK
               ? SWBT_IPC_OK
               : SWBT_IPC_ERROR_INVALID_ARGUMENT;
}

static swbt_ipc_result_t swbt_ipc_publish_app_state_unlocked(swbt_ipc_session_t *session) {
    return swbt_ipc_publish_state_unlocked(session);
}

static swbt_ipc_result_t swbt_ipc_revoke_owner_event_unlocked(swbt_ipc_session_t *session,
                                                              swbt_app_revoke_reason_t reason,
                                                              uint32_t client_id) {
    swbt_app_status_t before_revoke;
    if (swbt_app_get_status(&session->app, &before_revoke) != SWBT_APP_OK) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    const bool was_owner = before_revoke.has_owner && before_revoke.owner_client_id == client_id;
    const swbt_ipc_result_t result =
        swbt_ipc_map_app_result(swbt_app_revoke(&session->app, reason, client_id));
    if (result != SWBT_IPC_OK || !was_owner) {
        return result;
    }

    return swbt_ipc_publish_app_state_unlocked(session);
}

static swbt_ipc_result_t swbt_ipc_clear_owner_unlocked(swbt_ipc_session_t *session) {
    if (swbt_app_revoke(&session->app, SWBT_APP_REVOKE_SHUTDOWN, 0u) != SWBT_APP_OK) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    return swbt_ipc_publish_app_state_unlocked(session);
}

swbt_ipc_result_t swbt_ipc_session_init(swbt_ipc_session_t *session) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_init(&session->lock);
    if (swbt_app_init(&session->app) != SWBT_APP_OK ||
        swbt_metrics_init(&session->metrics) != SWBT_METRICS_OK) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    session->mailbox = NULL;
    session->daemon = swbt_ipc_daemon_status_default();
    session->hardware = swbt_ipc_hardware_status_default();
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

swbt_ipc_result_t
swbt_ipc_session_set_daemon_status(swbt_ipc_session_t *session,
                                   const swbt_ipc_daemon_status_t *daemon_status) {
    if (session == NULL || daemon_status == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&session->lock);
    session->daemon = *daemon_status;
    swbt_spin_lock_release(&session->lock);
    return SWBT_IPC_OK;
}

swbt_ipc_result_t
swbt_ipc_session_set_daemon_lifecycle(swbt_ipc_session_t *session,
                                      swbt_ipc_daemon_lifecycle_state_t lifecycle_state) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&session->lock);
    session->daemon.lifecycle_state = lifecycle_state;
    swbt_spin_lock_release(&session->lock);
    return SWBT_IPC_OK;
}

swbt_ipc_result_t
swbt_ipc_session_set_hardware_approval(swbt_ipc_session_t *session,
                                       swbt_ipc_hardware_approval_t hardware_approval) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&session->lock);
    session->daemon.hardware_approval = hardware_approval;
    swbt_spin_lock_release(&session->lock);
    return SWBT_IPC_OK;
}

swbt_ipc_result_t swbt_ipc_acquire(swbt_ipc_session_t *session, uint32_t client_id) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    swbt_spin_lock_acquire(&session->lock);
    const swbt_ipc_result_t result =
        swbt_ipc_map_app_result(swbt_app_acquire(&session->app, client_id));
    swbt_spin_lock_release(&session->lock);
    return result;
}

swbt_ipc_result_t swbt_ipc_release(swbt_ipc_session_t *session, uint32_t client_id) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    swbt_spin_lock_acquire(&session->lock);
    if (swbt_app_revoke(&session->app, SWBT_APP_REVOKE_RELEASE, client_id) != SWBT_APP_OK) {
        swbt_spin_lock_release(&session->lock);
        return SWBT_IPC_ERROR_NOT_OWNER;
    }

    const swbt_ipc_result_t result = swbt_ipc_publish_app_state_unlocked(session);
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
    const swbt_app_result_t app_result =
        swbt_app_set_state(&session->app, client_id, state, sequence);
    const swbt_ipc_result_t result = swbt_ipc_map_app_result(app_result);
    if (result != SWBT_IPC_OK) {
        swbt_spin_lock_release(&session->lock);
        return result;
    }
    if (app_result == SWBT_APP_ERROR_STALE_SEQUENCE) {
        swbt_spin_lock_release(&session->lock);
        return SWBT_IPC_OK;
    }

    const swbt_ipc_result_t publish_result = swbt_ipc_publish_state_unlocked(session);
    swbt_spin_lock_release(&session->lock);
    return publish_result;
}
// NOLINTEND(bugprone-easily-swappable-parameters)

swbt_ipc_result_t swbt_ipc_get_status(const swbt_ipc_session_t *session,
                                      swbt_ipc_status_t *out_status) {
    if (session == NULL || out_status == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    swbt_ipc_session_t *mutable_session = (swbt_ipc_session_t *)session;
    swbt_spin_lock_acquire(&mutable_session->lock);
    swbt_app_status_t app_status;
    if (swbt_app_get_status(&session->app, &app_status) != SWBT_APP_OK) {
        swbt_spin_lock_release(&mutable_session->lock);
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    out_status->has_owner = app_status.has_owner;
    out_status->owner_client_id = app_status.owner_client_id;
    out_status->last_seq = app_status.last_sequence;
    out_status->state = app_status.state;
    out_status->rumble = session->rumble;
    if (swbt_metrics_snapshot(&session->metrics, &out_status->metrics) != SWBT_METRICS_OK) {
        swbt_spin_lock_release(&mutable_session->lock);
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    out_status->daemon = session->daemon;
    out_status->hardware = session->hardware;
    swbt_spin_lock_release(&mutable_session->lock);
    return SWBT_IPC_OK;
}

swbt_ipc_result_t swbt_ipc_record_report_tick(swbt_ipc_session_t *session, uint64_t now_us,
                                              swbt_metrics_report_send_result_t send_result) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    swbt_spin_lock_acquire(&session->lock);
    const swbt_metrics_result_t result =
        swbt_metrics_record_report_tick(&session->metrics, now_us, send_result);
    swbt_spin_lock_release(&session->lock);
    return result == SWBT_METRICS_OK ? SWBT_IPC_OK : SWBT_IPC_ERROR_INVALID_ARGUMENT;
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
    const swbt_ipc_result_t result =
        swbt_ipc_revoke_owner_event_unlocked(session, SWBT_APP_REVOKE_DISCONNECT, client_id);
    swbt_spin_lock_release(&session->lock);
    return result;
}

swbt_ipc_result_t swbt_ipc_heartbeat_timeout(swbt_ipc_session_t *session, uint32_t client_id) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    swbt_spin_lock_acquire(&session->lock);
    const swbt_ipc_result_t result =
        swbt_ipc_revoke_owner_event_unlocked(session, SWBT_APP_REVOKE_HEARTBEAT_TIMEOUT, client_id);
    swbt_spin_lock_release(&session->lock);
    return result;
}
