#ifndef SWBT_IPC_SESSION_H
#define SWBT_IPC_SESSION_H

#include <stdbool.h>
#include <stdint.h>

#include "core/state_mailbox.h"
#include "switch/switch_controller_state.h"

typedef enum {
    SWBT_IPC_OK = 0,
    SWBT_IPC_ERROR_INVALID_ARGUMENT = -1,
    SWBT_IPC_ERROR_OWNER_BUSY = -2,
    SWBT_IPC_ERROR_NOT_OWNER = -3,
} swbt_ipc_result_t;

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    swbt_state_t state;
} swbt_ipc_status_t;

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    swbt_state_t state;
    swbt_state_mailbox_t *mailbox;
} swbt_ipc_session_t;

swbt_ipc_result_t swbt_ipc_session_init(swbt_ipc_session_t *session);

swbt_ipc_result_t swbt_ipc_session_bind_mailbox(swbt_ipc_session_t *session,
                                                swbt_state_mailbox_t *mailbox);

swbt_ipc_result_t swbt_ipc_acquire(swbt_ipc_session_t *session, uint32_t client_id);

swbt_ipc_result_t swbt_ipc_release(swbt_ipc_session_t *session, uint32_t client_id);

swbt_ipc_result_t swbt_ipc_set_state(swbt_ipc_session_t *session, uint32_t client_id,
                                     const swbt_state_t *state);

swbt_ipc_result_t swbt_ipc_get_status(const swbt_ipc_session_t *session,
                                      swbt_ipc_status_t *out_status);

swbt_ipc_result_t swbt_ipc_disconnect(swbt_ipc_session_t *session, uint32_t client_id);

swbt_ipc_result_t swbt_ipc_heartbeat_timeout(swbt_ipc_session_t *session, uint32_t client_id);

#endif
