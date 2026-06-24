#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "application/app.h"
#include "switch/switch_controller_state.h"

typedef struct {
    uint32_t owner_id;
    uint64_t sequence;
} expected_active_state_t;

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_false(bool value) {
    return value ? 1 : 0;
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

static int application_acquires_and_rejects_owners(void) {
    swbt_app_t *app = swbt_app_create();
    swbt_app_snapshot_t status;
    int failed = 0;

    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_app_snapshot(app, &status), SWBT_APP_OK);
    failed += expect_false(status.has_owner);
    failed += expect_eq_u32(status.owner_client_id, 0u);
    failed += expect_eq_u64(status.last_sequence, 0u);

    failed += expect_eq_int(swbt_app_acquire(app, 1001u), SWBT_APP_OK);
    failed += expect_eq_int(swbt_app_acquire(app, 1001u), SWBT_APP_OK);
    failed += expect_eq_int(swbt_app_acquire(app, 2002u), SWBT_APP_ERROR_OWNER_BUSY);

    failed += expect_eq_int(swbt_app_snapshot(app, &status), SWBT_APP_OK);
    failed += expect_true(status.has_owner);
    failed += expect_eq_u32(status.owner_client_id, 1001u);
    failed += expect_eq_u64(status.last_sequence, 0u);

    swbt_app_destroy(app);
    return failed;
}

static int stale_sequence_does_not_update_state(void) {
    swbt_app_t *app = swbt_app_create();
    swbt_app_snapshot_t status;
    swbt_state_t state = swbt_state_neutral();
    int failed = 0;

    state.buttons = SWBT_BUTTON_A;
    state.lx = 1234u;

    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_app_acquire(app, 1001u), SWBT_APP_OK);
    failed += expect_eq_int(swbt_app_set_state(app,
                                               (swbt_app_set_state_options_t){
                                                   .client_id = 1001u,
                                                   .state = &state,
                                                   .sequence = 77u,
                                               }),
                            SWBT_APP_OK);

    state.buttons = SWBT_BUTTON_B;
    state.lx = 3456u;
    failed += expect_eq_int(swbt_app_set_state(app,
                                               (swbt_app_set_state_options_t){
                                                   .client_id = 1001u,
                                                   .state = &state,
                                                   .sequence = 76u,
                                               }),
                            SWBT_APP_ERROR_STALE_SEQUENCE);

    failed += expect_eq_int(swbt_app_snapshot(app, &status), SWBT_APP_OK);
    failed += expect_eq_u32(status.state.buttons, SWBT_BUTTON_A);
    failed += expect_eq_u16(status.state.lx, 1234u);
    failed += expect_eq_u64(status.last_sequence, 77u);

    swbt_app_destroy(app);
    return failed;
}

static int sequential_updates_do_not_report_coalesced_state_updates(void) {
    swbt_app_t *app = swbt_app_create();
    swbt_app_snapshot_t status;
    swbt_state_t state = swbt_state_neutral();
    int failed = 0;

    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_app_acquire(app, 1001u), SWBT_APP_OK);

    state.buttons = SWBT_BUTTON_A;
    failed += expect_eq_int(swbt_app_set_state(app,
                                               (swbt_app_set_state_options_t){
                                                   .client_id = 1001u,
                                                   .state = &state,
                                                   .sequence = 1u,
                                               }),
                            SWBT_APP_OK);

    state.buttons = SWBT_BUTTON_B;
    failed += expect_eq_int(swbt_app_set_state(app,
                                               (swbt_app_set_state_options_t){
                                                   .client_id = 1001u,
                                                   .state = &state,
                                                   .sequence = 2u,
                                               }),
                            SWBT_APP_OK);

    failed += expect_eq_int(swbt_app_snapshot(app, &status), SWBT_APP_OK);
    failed += expect_eq_u32(status.state.buttons, SWBT_BUTTON_B);
    failed += expect_eq_u64(status.last_sequence, 2u);
    failed += expect_eq_u64(status.metrics.ipc_state_accepted, 2u);
    failed += expect_eq_u64(status.metrics.ipc_state_rejected, 0u);
    failed += expect_eq_u64(status.metrics.ipc_state_coalesced, 0u);

    swbt_app_destroy(app);
    return failed;
}

static int expect_active_state(const swbt_app_t *app, expected_active_state_t expected) {
    swbt_app_snapshot_t status;
    int failed = 0;

    failed += expect_eq_int(swbt_app_snapshot(app, &status), SWBT_APP_OK);
    failed += expect_true(status.has_owner);
    failed += expect_eq_u32(status.owner_client_id, expected.owner_id);
    failed += expect_eq_u64(status.last_sequence, expected.sequence);
    failed += expect_eq_u32(status.state.buttons, SWBT_BUTTON_A);
    failed += expect_eq_u16(status.state.lx, 1234u);
    return failed;
}

static int expect_neutral_state(const swbt_app_t *app) {
    swbt_app_snapshot_t status;
    int failed = 0;

    failed += expect_eq_int(swbt_app_snapshot(app, &status), SWBT_APP_OK);
    failed += expect_false(status.has_owner);
    failed += expect_eq_u32(status.owner_client_id, 0u);
    failed += expect_eq_u64(status.last_sequence, 0u);
    failed += expect_eq_u32(status.state.buttons, 0u);
    failed += expect_eq_u16(status.state.lx, 2048u);
    return failed;
}

static int set_active_state(swbt_app_t *app) {
    swbt_state_t state = swbt_state_neutral();
    int failed = 0;

    state.buttons = SWBT_BUTTON_A;
    state.lx = 1234u;
    failed += expect_eq_int(swbt_app_acquire(app, 1001u), SWBT_APP_OK);
    failed += expect_eq_int(swbt_app_set_state(app,
                                               (swbt_app_set_state_options_t){
                                                   .client_id = 1001u,
                                                   .state = &state,
                                                   .sequence = 11u,
                                               }),
                            SWBT_APP_OK);
    return failed;
}

static int revoke_reasons_share_neutral_policy(void) {
    static const swbt_app_revoke_reason_t reasons[] = {
        SWBT_APP_REVOKE_RELEASE,
        SWBT_APP_REVOKE_DISCONNECT,
        SWBT_APP_REVOKE_HEARTBEAT_TIMEOUT,
        SWBT_APP_REVOKE_SHUTDOWN,
    };
    swbt_app_t *app = swbt_app_create();
    int failed = 0;

    failed += expect_true(app != NULL);
    failed += set_active_state(app);
    failed += expect_eq_int(swbt_app_revoke(app,
                                            (swbt_app_revoke_options_t){
                                                .reason = SWBT_APP_REVOKE_RELEASE,
                                                .client_id = 2002u,
                                            }),
                            SWBT_APP_ERROR_NOT_OWNER);
    failed += expect_active_state(app, (expected_active_state_t){
                                           .owner_id = 1001u,
                                           .sequence = 11u,
                                       });

    for (size_t index = 0; index < sizeof(reasons) / sizeof(reasons[0]); ++index) {
        swbt_app_destroy(app);
        app = swbt_app_create();
        failed += expect_true(app != NULL);
        failed += set_active_state(app);
        failed += expect_eq_int(swbt_app_revoke(app,
                                                (swbt_app_revoke_options_t){
                                                    .reason = reasons[index],
                                                    .client_id = 1001u,
                                                }),
                                SWBT_APP_OK);
        failed += expect_neutral_state(app);
    }

    swbt_app_destroy(app);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += application_acquires_and_rejects_owners();
    failed += stale_sequence_does_not_update_state();
    failed += sequential_updates_do_not_report_coalesced_state_updates();
    failed += revoke_reasons_share_neutral_policy();
    return failed == 0 ? 0 : 1;
}
