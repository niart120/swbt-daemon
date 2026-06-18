#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ipc/ipc_session.h"
#include "switch/switch_controller_state.h"

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_false(bool value) {
    return value ? 1 : 0;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

int main(void) {
    swbt_ipc_session_t session;
    swbt_ipc_status_t status;
    swbt_state_t state = swbt_state_neutral();

    state.buttons = SWBT_BUTTON_A;
    state.lx = 1234;
    state.client_seq = 7;

    if (swbt_ipc_session_init(&session) != SWBT_IPC_OK) {
        return 1;
    }
    if (swbt_ipc_get_status(&session, &status) != SWBT_IPC_OK) {
        return 2;
    }
    if (expect_false(status.has_owner) || expect_eq_u16(status.state.lx, 2048)) {
        return 3;
    }
    if (swbt_ipc_acquire(&session, 1001) != SWBT_IPC_OK) {
        return 4;
    }
    if (swbt_ipc_acquire(&session, 2002) != SWBT_IPC_ERROR_OWNER_BUSY) {
        return 5;
    }
    if (swbt_ipc_set_state(&session, 2002, &state) != SWBT_IPC_ERROR_NOT_OWNER) {
        return 6;
    }
    if (swbt_ipc_set_state(&session, 1001, &state) != SWBT_IPC_OK) {
        return 7;
    }
    if (swbt_ipc_get_status(&session, &status) != SWBT_IPC_OK) {
        return 8;
    }
    if (expect_true(status.has_owner) || expect_eq_u32(status.owner_client_id, 1001) ||
        expect_eq_u32(status.state.buttons, SWBT_BUTTON_A) ||
        expect_eq_u16(status.state.lx, 1234)) {
        return 9;
    }
    if (swbt_ipc_release(&session, 2002) != SWBT_IPC_ERROR_NOT_OWNER) {
        return 10;
    }
    if (swbt_ipc_release(&session, 1001) != SWBT_IPC_OK) {
        return 11;
    }
    if (swbt_ipc_get_status(&session, &status) != SWBT_IPC_OK) {
        return 12;
    }
    if (expect_false(status.has_owner) || expect_eq_u32(status.state.buttons, 0) ||
        expect_eq_u16(status.state.lx, 2048)) {
        return 13;
    }
    if (swbt_ipc_acquire(&session, 3003) != SWBT_IPC_OK) {
        return 14;
    }
    if (swbt_ipc_set_state(&session, 3003, &state) != SWBT_IPC_OK) {
        return 15;
    }
    if (swbt_ipc_disconnect(&session, 3003) != SWBT_IPC_OK) {
        return 16;
    }
    if (swbt_ipc_get_status(&session, &status) != SWBT_IPC_OK) {
        return 17;
    }
    if (expect_false(status.has_owner) || expect_eq_u32(status.state.buttons, 0)) {
        return 18;
    }
    if (swbt_ipc_acquire(&session, 4004) != SWBT_IPC_OK) {
        return 19;
    }
    if (swbt_ipc_set_state(&session, 4004, &state) != SWBT_IPC_OK) {
        return 20;
    }
    if (swbt_ipc_heartbeat_timeout(&session, 4004) != SWBT_IPC_OK) {
        return 21;
    }
    if (swbt_ipc_get_status(&session, &status) != SWBT_IPC_OK) {
        return 22;
    }
    if (expect_false(status.has_owner) || expect_eq_u32(status.state.buttons, 0)) {
        return 23;
    }
    if (swbt_ipc_session_init(NULL) != SWBT_IPC_ERROR_INVALID_ARGUMENT) {
        return 24;
    }
    if (swbt_ipc_get_status(NULL, &status) != SWBT_IPC_ERROR_INVALID_ARGUMENT) {
        return 25;
    }
    if (swbt_ipc_set_state(&session, 1, NULL) != SWBT_IPC_ERROR_INVALID_ARGUMENT) {
        return 26;
    }

    return 0;
}
