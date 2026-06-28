#include <stdbool.h>
#include <stdint.h>

#include "domain/domain.h"
#include "control/control.h"
#include "switch/switch_controller_state.h"

typedef struct {
    int read_calls;
    int read_result;
    swbt_control_runtime_status_t status;
} fake_runtime_status_reader_t;

typedef struct {
    uint32_t buttons;
    uint16_t lx;
    uint64_t last_sequence;
    uint64_t accepted;
    uint64_t rejected;
} expected_app_status_t;

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

static int fake_read_runtime_status(void *context, swbt_control_runtime_status_t *out_status) {
    fake_runtime_status_reader_t *fake = context;

    if (fake == NULL || out_status == NULL) {
        return -1;
    }
    fake->read_calls += 1;
    if (fake->read_result != 0) {
        return fake->read_result;
    }
    *out_status = fake->status;
    return 0;
}

static int expect_status(const swbt_domain_t *app, expected_app_status_t expected) {
    swbt_domain_status_snapshot_t status;
    int failed = 0;

    failed += expect_eq_int(swbt_domain_read_status(app, &status), SWBT_DOMAIN_OK);
    failed += expect_true(status.has_owner);
    failed += expect_eq_u32(status.owner_client_id, 1001u);
    failed += expect_eq_u64(status.last_sequence, expected.last_sequence);
    failed += expect_eq_u32(status.state.buttons, expected.buttons);
    failed += expect_eq_u16(status.state.lx, expected.lx);
    failed += expect_eq_u64(status.metrics.ipc_state_accepted, expected.accepted);
    failed += expect_eq_u64(status.metrics.ipc_state_rejected, expected.rejected);
    return failed;
}

static int submit_client_state_preserves_owner_and_sequence_semantics(void) {
    swbt_domain_t *app = swbt_domain_create();
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
    failed += expect_status(app, (expected_app_status_t){
                                     .buttons = 0u,
                                     .lx = 2048u,
                                     .last_sequence = 0u,
                                     .accepted = 0u,
                                     .rejected = 1u,
                                 });

    failed += expect_eq_int(swbt_control_submit_client_state(&control, 1001u, &state, 77u),
                            SWBT_CONTROL_OK);
    failed += expect_status(app, (expected_app_status_t){
                                     .buttons = SWBT_BUTTON_A,
                                     .lx = 1234u,
                                     .last_sequence = 77u,
                                     .accepted = 1u,
                                     .rejected = 1u,
                                 });

    state.buttons = SWBT_BUTTON_B;
    state.lx = 3456u;
    failed += expect_eq_int(swbt_control_submit_client_state(&control, 1001u, &state, 76u),
                            SWBT_CONTROL_OK);
    failed += expect_status(app, (expected_app_status_t){
                                     .buttons = SWBT_BUTTON_A,
                                     .lx = 1234u,
                                     .last_sequence = 77u,
                                     .accepted = 1u,
                                     .rejected = 2u,
                                 });

    swbt_domain_destroy(app);
    return failed;
}

static int submit_state_uses_control_owned_owner_and_sequence(void) {
    swbt_domain_t *app = swbt_domain_create();
    swbt_control_t control;
    swbt_domain_status_snapshot_t status;
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
    failed += expect_eq_int(swbt_domain_read_status(app, &status), SWBT_DOMAIN_OK);
    failed += expect_true(status.has_owner);
    failed += expect_eq_u32(status.owner_client_id, UINT32_MAX);
    failed += expect_eq_u64(status.last_sequence, 1u);
    failed += expect_eq_u32(status.state.buttons, SWBT_BUTTON_A);

    state.buttons = SWBT_BUTTON_B;
    failed += expect_eq_int(swbt_control_submit_state(&control, &state), SWBT_CONTROL_OK);
    failed += expect_eq_int(swbt_domain_read_status(app, &status), SWBT_DOMAIN_OK);
    failed += expect_eq_u64(status.last_sequence, 2u);
    failed += expect_eq_u32(status.state.buttons, SWBT_BUTTON_B);
    failed += expect_eq_u64(status.metrics.ipc_state_accepted, 2u);
    failed += expect_eq_u64(status.metrics.ipc_state_rejected, 0u);

    swbt_domain_destroy(app);
    return failed;
}

static int get_status_combines_app_and_runtime_status_from_reader(void) {
    swbt_domain_t *app = swbt_domain_create();
    swbt_control_t control;
    swbt_control_status_t status;
    fake_runtime_status_reader_t fake = {
        .status =
            {
                .initialized = true,
                .running = true,
                .hid_registered = true,
                .output_handler_started = true,
                .report_timer_started = true,
            },
    };
    swbt_state_t state = swbt_state_neutral();
    int failed = 0;

    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_control_init(&control,
                                              &(swbt_control_config_t){
                                                  .app = app,
                                                  .read_runtime_status = fake_read_runtime_status,
                                                  .runtime_status_context = &fake,
                                              }),
                            SWBT_CONTROL_OK);

    state.buttons = SWBT_BUTTON_A;
    failed += expect_eq_int(swbt_control_submit_client_state(&control, 1001u, &state, 7u),
                            SWBT_CONTROL_ERROR_NOT_OWNER);
    failed += expect_eq_int(swbt_control_acquire_client(&control, 1001u), SWBT_CONTROL_OK);
    failed += expect_eq_int(swbt_control_submit_client_state(&control, 1001u, &state, 7u),
                            SWBT_CONTROL_OK);

    failed += expect_eq_int(swbt_control_get_status(&control, &status), SWBT_CONTROL_OK);
    failed += expect_eq_int(fake.read_calls, 1);
    failed += expect_true(status.app.has_owner);
    failed += expect_eq_u32(status.app.owner_client_id, 1001u);
    failed += expect_eq_u64(status.app.last_sequence, 7u);
    failed += expect_eq_u32(status.app.state.buttons, SWBT_BUTTON_A);
    failed += expect_true(status.has_runtime_status);
    failed += expect_true(status.runtime.initialized);
    failed += expect_true(status.runtime.running);
    failed += expect_true(status.runtime.hid_registered);
    failed += expect_true(status.runtime.output_handler_started);
    failed += expect_true(status.runtime.report_timer_started);

    swbt_domain_destroy(app);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += submit_client_state_preserves_owner_and_sequence_semantics();
    failed += submit_state_uses_control_owned_owner_and_sequence();
    failed += get_status_combines_app_and_runtime_status_from_reader();
    return failed == 0 ? 0 : 1;
}
