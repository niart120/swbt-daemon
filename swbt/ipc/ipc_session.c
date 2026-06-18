#include "ipc/ipc_session.h"

#include <stddef.h>

static void swbt_ipc_apply_neutral(swbt_ipc_session_t *session) {
    session->state = swbt_state_neutral();
}

static int swbt_ipc_is_owner(const swbt_ipc_session_t *session, uint32_t client_id) {
    return session->has_owner && session->owner_client_id == client_id;
}

static void swbt_ipc_clear_owner(swbt_ipc_session_t *session) {
    session->has_owner = false;
    session->owner_client_id = 0;
    swbt_ipc_apply_neutral(session);
}

swbt_ipc_result_t swbt_ipc_session_init(swbt_ipc_session_t *session) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    session->has_owner = false;
    session->owner_client_id = 0;
    swbt_ipc_apply_neutral(session);
    return SWBT_IPC_OK;
}

swbt_ipc_result_t swbt_ipc_acquire(swbt_ipc_session_t *session, uint32_t client_id) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    if (session->has_owner && session->owner_client_id != client_id) {
        return SWBT_IPC_ERROR_OWNER_BUSY;
    }

    session->has_owner = true;
    session->owner_client_id = client_id;
    return SWBT_IPC_OK;
}

swbt_ipc_result_t swbt_ipc_release(swbt_ipc_session_t *session, uint32_t client_id) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    if (!swbt_ipc_is_owner(session, client_id)) {
        return SWBT_IPC_ERROR_NOT_OWNER;
    }

    swbt_ipc_clear_owner(session);
    return SWBT_IPC_OK;
}

swbt_ipc_result_t swbt_ipc_set_state(swbt_ipc_session_t *session, uint32_t client_id,
                                     const swbt_state_t *state) {
    if (session == NULL || state == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    if (!swbt_ipc_is_owner(session, client_id)) {
        return SWBT_IPC_ERROR_NOT_OWNER;
    }

    session->state = *state;
    return SWBT_IPC_OK;
}

swbt_ipc_result_t swbt_ipc_get_status(const swbt_ipc_session_t *session,
                                      swbt_ipc_status_t *out_status) {
    if (session == NULL || out_status == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }

    out_status->has_owner = session->has_owner;
    out_status->owner_client_id = session->owner_client_id;
    out_status->state = session->state;
    return SWBT_IPC_OK;
}

swbt_ipc_result_t swbt_ipc_disconnect(swbt_ipc_session_t *session, uint32_t client_id) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    if (swbt_ipc_is_owner(session, client_id)) {
        swbt_ipc_clear_owner(session);
    }
    return SWBT_IPC_OK;
}

swbt_ipc_result_t swbt_ipc_heartbeat_timeout(swbt_ipc_session_t *session, uint32_t client_id) {
    if (session == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    if (swbt_ipc_is_owner(session, client_id)) {
        swbt_ipc_clear_owner(session);
    }
    return SWBT_IPC_OK;
}
