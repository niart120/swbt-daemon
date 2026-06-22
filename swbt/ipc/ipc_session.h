#ifndef SWBT_IPC_SESSION_H
#define SWBT_IPC_SESSION_H

#include <stdbool.h>
#include <stdint.h>

#include "application/app.h"
#include "core/spin_lock.h"
#include "core/state_mailbox.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_rumble.h"
#include "switch/switch_subcommand.h"

typedef enum {
    SWBT_IPC_OK = 0,
    SWBT_IPC_ERROR_INVALID_ARGUMENT = -1,
    SWBT_IPC_ERROR_OWNER_BUSY = -2,
    SWBT_IPC_ERROR_NOT_OWNER = -3,
} swbt_ipc_result_t;

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    uint64_t last_seq;
    swbt_state_t state;
    swbt_switch_rumble_state_t rumble;
} swbt_ipc_status_t;

typedef struct {
    swbt_spin_lock_t lock;
    swbt_app_t app;
    swbt_switch_rumble_state_t rumble;
    swbt_state_mailbox_t *mailbox;
} swbt_ipc_session_t;

swbt_ipc_result_t swbt_ipc_session_init(swbt_ipc_session_t *session);

swbt_ipc_result_t swbt_ipc_session_bind_mailbox(swbt_ipc_session_t *session,
                                                swbt_state_mailbox_t *mailbox);

swbt_ipc_result_t swbt_ipc_acquire(swbt_ipc_session_t *session, uint32_t client_id);

swbt_ipc_result_t swbt_ipc_release(swbt_ipc_session_t *session, uint32_t client_id);

swbt_ipc_result_t swbt_ipc_clear_owner(swbt_ipc_session_t *session);

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_ipc_result_t swbt_ipc_set_state(swbt_ipc_session_t *session, uint32_t client_id,
                                     const swbt_state_t *state, uint64_t sequence);
// NOLINTEND(bugprone-easily-swappable-parameters)

swbt_ipc_result_t swbt_ipc_get_status(const swbt_ipc_session_t *session,
                                      swbt_ipc_status_t *out_status);

swbt_ipc_result_t swbt_ipc_record_rumble(swbt_ipc_session_t *session, const uint8_t *payload,
                                         uint64_t updated_at_ms);

swbt_ipc_result_t
swbt_ipc_record_output_report_rumble(swbt_ipc_session_t *session,
                                     const swbt_switch_output_report_t *output_report,
                                     uint64_t updated_at_ms);

swbt_ipc_result_t swbt_ipc_disconnect(swbt_ipc_session_t *session, uint32_t client_id);

swbt_ipc_result_t swbt_ipc_heartbeat_timeout(swbt_ipc_session_t *session, uint32_t client_id);

#endif
