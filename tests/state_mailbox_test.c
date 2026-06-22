#include <stdbool.h>
#include <stdint.h>

#include "core/state_mailbox.h"
#include "ipc/ipc_session.h"
#include "switch/switch_controller_state.h"

static int expect_false(bool value) {
    return value ? 1 : 0;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u64(uint64_t actual, uint64_t expected) {
    return actual == expected ? 0 : 1;
}

static int test_init_loads_neutral_generation_zero(void) {
    swbt_state_mailbox_t mailbox;
    swbt_state_mailbox_snapshot_t snapshot;

    int failed = 0;
    failed += expect_eq_int(swbt_state_mailbox_init(&mailbox), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_int(swbt_state_mailbox_load(&mailbox, &snapshot), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u64(snapshot.generation, 0u);
    failed += expect_eq_u64(snapshot.coalesced_updates, 0u);
    failed += expect_eq_u32(snapshot.state.buttons, 0u);
    failed += expect_eq_u16(snapshot.state.lx, 2048u);
    failed += expect_eq_u16(snapshot.state.ly, 2048u);
    failed += expect_eq_u16(snapshot.state.rx, 2048u);
    failed += expect_eq_u16(snapshot.state.ry, 2048u);
    failed += expect_false(snapshot.has_update);
    return failed;
}

static swbt_state_t sample_state(void) {
    swbt_state_t state = swbt_state_neutral();
    state.buttons = SWBT_BUTTON_A;
    state.lx = 1234u;
    return state;
}

static int test_store_loads_copied_latest_state(void) {
    swbt_state_mailbox_t mailbox;
    swbt_state_mailbox_snapshot_t snapshot;
    swbt_state_t state = sample_state();

    int failed = 0;
    failed += expect_eq_int(swbt_state_mailbox_init(&mailbox), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_int(swbt_state_mailbox_store(&mailbox, &state), SWBT_STATE_MAILBOX_OK);

    state.buttons = SWBT_BUTTON_B;
    state.lx = 4095u;

    failed += expect_eq_int(swbt_state_mailbox_load(&mailbox, &snapshot), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u64(snapshot.generation, 1u);
    failed += expect_eq_u64(snapshot.coalesced_updates, 0u);
    failed += expect_eq_u32(snapshot.state.buttons, SWBT_BUTTON_A);
    failed += expect_eq_u16(snapshot.state.lx, 1234u);
    return failed;
}

static int test_multiple_stores_report_coalesced_updates(void) {
    swbt_state_mailbox_t mailbox;
    swbt_state_mailbox_snapshot_t snapshot;
    swbt_state_t state = sample_state();

    int failed = 0;
    failed += expect_eq_int(swbt_state_mailbox_init(&mailbox), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_int(swbt_state_mailbox_store(&mailbox, &state), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_int(swbt_state_mailbox_load(&mailbox, &snapshot), SWBT_STATE_MAILBOX_OK);

    state.buttons = SWBT_BUTTON_B;
    failed += expect_eq_int(swbt_state_mailbox_store(&mailbox, &state), SWBT_STATE_MAILBOX_OK);
    state.buttons = SWBT_BUTTON_X;
    failed += expect_eq_int(swbt_state_mailbox_store(&mailbox, &state), SWBT_STATE_MAILBOX_OK);

    failed += expect_eq_int(swbt_state_mailbox_load(&mailbox, &snapshot), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u64(snapshot.generation, 3u);
    failed += expect_eq_u64(snapshot.coalesced_updates, 1u);
    failed += expect_eq_u32(snapshot.state.buttons, SWBT_BUTTON_X);

    failed += expect_eq_int(swbt_state_mailbox_load(&mailbox, &snapshot), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u64(snapshot.generation, 3u);
    failed += expect_eq_u64(snapshot.coalesced_updates, 0u);
    failed += expect_false(snapshot.has_update);
    return failed;
}

static int test_ipc_disconnect_and_timeout_store_neutral(void) {
    swbt_ipc_session_t session;
    swbt_state_mailbox_t mailbox;
    swbt_state_mailbox_snapshot_t snapshot;
    swbt_state_t state = sample_state();

    int failed = 0;
    failed += expect_eq_int(swbt_ipc_session_init(&session), SWBT_IPC_OK);
    failed += expect_eq_int(swbt_state_mailbox_init(&mailbox), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_int(swbt_ipc_session_bind_mailbox(&session, &mailbox), SWBT_IPC_OK);

    failed += expect_eq_int(swbt_ipc_acquire(&session, 1001u), SWBT_IPC_OK);
    failed += expect_eq_int(swbt_ipc_set_state(&session, 1001u, &state, 7u), SWBT_IPC_OK);
    failed += expect_eq_int(swbt_state_mailbox_load(&mailbox, &snapshot), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u32(snapshot.state.buttons, SWBT_BUTTON_A);

    const uint64_t active_generation = snapshot.generation;
    failed += expect_eq_int(swbt_ipc_disconnect(&session, 9999u), SWBT_IPC_OK);
    failed += expect_eq_int(swbt_state_mailbox_load(&mailbox, &snapshot), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u64(snapshot.generation, active_generation);
    failed += expect_false(snapshot.has_update);
    failed += expect_eq_u32(snapshot.state.buttons, SWBT_BUTTON_A);

    failed += expect_eq_int(swbt_ipc_heartbeat_timeout(&session, 9999u), SWBT_IPC_OK);
    failed += expect_eq_int(swbt_state_mailbox_load(&mailbox, &snapshot), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u64(snapshot.generation, active_generation);
    failed += expect_false(snapshot.has_update);
    failed += expect_eq_u32(snapshot.state.buttons, SWBT_BUTTON_A);

    failed += expect_eq_int(swbt_ipc_disconnect(&session, 1001u), SWBT_IPC_OK);
    failed += expect_eq_int(swbt_state_mailbox_load(&mailbox, &snapshot), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u32(snapshot.state.buttons, 0u);
    failed += expect_eq_u16(snapshot.state.lx, 2048u);

    failed += expect_eq_int(swbt_ipc_acquire(&session, 2002u), SWBT_IPC_OK);
    failed += expect_eq_int(swbt_ipc_set_state(&session, 2002u, &state, 8u), SWBT_IPC_OK);
    failed += expect_eq_int(swbt_state_mailbox_load(&mailbox, &snapshot), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u32(snapshot.state.buttons, SWBT_BUTTON_A);

    failed += expect_eq_int(swbt_ipc_heartbeat_timeout(&session, 2002u), SWBT_IPC_OK);
    failed += expect_eq_int(swbt_state_mailbox_load(&mailbox, &snapshot), SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u32(snapshot.state.buttons, 0u);
    failed += expect_eq_u16(snapshot.state.lx, 2048u);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += test_init_loads_neutral_generation_zero();
    failed += test_store_loads_copied_latest_state();
    failed += test_multiple_stores_report_coalesced_updates();
    failed += test_ipc_disconnect_and_timeout_store_neutral();
    return failed == 0 ? 0 : 1;
}
