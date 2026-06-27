#include <stdbool.h>
#include <stdint.h>

#include "application/app.h"
#include "control/control.h"
#include "switch/switch_controller_state.h"

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u64(uint64_t actual, uint64_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_status(const swbt_app_t *app, uint32_t buttons, uint16_t lx,
                         uint64_t last_sequence, uint64_t accepted, uint64_t rejected) {
    swbt_app_status_snapshot_t status;
    int failed = 0;

    failed += expect_eq_int(swbt_app_read_status(app, &status), SWBT_APP_OK);
    failed += expect_true(status.has_owner);
    failed += expect_eq_u32(status.owner_client_id, 1001u);
    failed += expect_eq_u64(status.last_sequence, last_sequence);
    failed += expect_eq_u32(status.state.buttons, buttons);
    failed += expect_eq_u16(status.state.lx, lx);
    failed += expect_eq_u64(status.metrics.ipc_state_accepted, accepted);
    failed += expect_eq_u64(status.metrics.ipc_state_rejected, rejected);
    return failed;
}

static int submit_client_state_preserves_owner_and_sequence_semantics(void) {
    swbt_app_t *app = swbt_app_create();
    swbt_control_t control;
    swbt_state_t state = swbt_state_neutral();
    int failed = 0;

    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_control_init(&control,
                                              &(swbt_control_config_t){
                                                  .app = app,
                                              }),
                            SWBT_CONTROL_OK);
    failed += expect_eq_int(swbt_control_acquire_client(&control, 1001u), SWBT_CONTROL_OK);
    failed +=
        expect_eq_int(swbt_control_acquire_client(&control, 2002u), SWBT_CONTROL_ERROR_OWNER_BUSY);

    state.buttons = SWBT_BUTTON_A;
    state.lx = 1234u;
    failed += expect_eq_int(swbt_control_submit_client_state(&control, 2002u, &state, 77u),
                            SWBT_CONTROL_ERROR_NOT_OWNER);
    failed += expect_status(app, 0u, 2048u, 0u, 0u, 1u);

    failed += expect_eq_int(swbt_control_submit_client_state(&control, 1001u, &state, 77u),
                            SWBT_CONTROL_OK);
    failed += expect_status(app, SWBT_BUTTON_A, 1234u, 77u, 1u, 1u);

    state.buttons = SWBT_BUTTON_B;
    state.lx = 3456u;
    failed += expect_eq_int(swbt_control_submit_client_state(&control, 1001u, &state, 76u),
                            SWBT_CONTROL_OK);
    failed += expect_status(app, SWBT_BUTTON_A, 1234u, 77u, 1u, 2u);

    swbt_app_destroy(app);
    return failed;
}

static int submit_state_uses_control_owned_owner_and_sequence(void) {
    swbt_app_t *app = swbt_app_create();
    swbt_control_t control;
    swbt_app_status_snapshot_t status;
    swbt_state_t state = swbt_state_neutral();
    int failed = 0;

    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_control_init(&control,
                                              &(swbt_control_config_t){
                                                  .app = app,
                                              }),
                            SWBT_CONTROL_OK);

    state.buttons = SWBT_BUTTON_A;
    failed += expect_eq_int(swbt_control_submit_state(&control, &state), SWBT_CONTROL_OK);
    failed += expect_eq_int(swbt_app_read_status(app, &status), SWBT_APP_OK);
    failed += expect_true(status.has_owner);
    failed += expect_eq_u32(status.owner_client_id, UINT32_MAX);
    failed += expect_eq_u64(status.last_sequence, 1u);
    failed += expect_eq_u32(status.state.buttons, SWBT_BUTTON_A);

    state.buttons = SWBT_BUTTON_B;
    failed += expect_eq_int(swbt_control_submit_state(&control, &state), SWBT_CONTROL_OK);
    failed += expect_eq_int(swbt_app_read_status(app, &status), SWBT_APP_OK);
    failed += expect_eq_u64(status.last_sequence, 2u);
    failed += expect_eq_u32(status.state.buttons, SWBT_BUTTON_B);
    failed += expect_eq_u64(status.metrics.ipc_state_accepted, 2u);
    failed += expect_eq_u64(status.metrics.ipc_state_rejected, 0u);

    swbt_app_destroy(app);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += submit_client_state_preserves_owner_and_sequence_semantics();
    failed += submit_state_uses_control_owned_owner_and_sequence();
    return failed == 0 ? 0 : 1;
}
