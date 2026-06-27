#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "application/app.h"
#include "btstack_bridge/output_report_handler.h"
#include "control/control.h"
#include "switch/switch_controller_state.h"

typedef struct {
    int hid_register_calls;
    int hid_stop_calls;
    int output_handler_start_calls;
    int output_handler_stop_calls;
    int report_timer_start_calls;
    int report_timer_stop_calls;
} fake_runtime_backend_t;

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

static int fake_hid_register(void *context) {
    fake_runtime_backend_t *fake = context;
    fake->hid_register_calls += 1;
    return 0;
}

static void fake_hid_stop(void *context) {
    fake_runtime_backend_t *fake = context;
    fake->hid_stop_calls += 1;
}

static void fake_output_handler_start(void *context,
                                      swbt_btstack_output_report_handler_t *handler) {
    fake_runtime_backend_t *fake = context;
    (void)handler;
    fake->output_handler_start_calls += 1;
}

static void fake_output_handler_stop(void *context) {
    fake_runtime_backend_t *fake = context;
    fake->output_handler_stop_calls += 1;
}

static int fake_report_timer_start(void *context, swbt_runtime_state_provider_t state_provider,
                                   void *state_context) {
    fake_runtime_backend_t *fake = context;
    (void)state_provider;
    (void)state_context;
    fake->report_timer_start_calls += 1;
    return 0;
}

static void fake_report_timer_stop(void *context) {
    fake_runtime_backend_t *fake = context;
    fake->report_timer_stop_calls += 1;
}

static int fake_report_timer_send_neutral_now(void *context) {
    (void)context;
    return 0;
}

static int fake_subcommand_reply_enqueue(void *context, uint16_t hid_cid, const uint8_t *report,
                                         size_t report_size) {
    (void)context;
    (void)hid_cid;
    (void)report;
    (void)report_size;
    return 0;
}

static uint32_t fake_time_ms(void *context) {
    (void)context;
    return 0u;
}

static swbt_runtime_host_backend_t fake_runtime_backend(void) {
    return (swbt_runtime_host_backend_t){
        .hid_register = fake_hid_register,
        .hid_stop = fake_hid_stop,
        .output_handler_start = fake_output_handler_start,
        .output_handler_stop = fake_output_handler_stop,
        .report_timer_start = fake_report_timer_start,
        .report_timer_stop = fake_report_timer_stop,
        .report_timer_send_neutral_now = fake_report_timer_send_neutral_now,
        .subcommand_reply_enqueue = fake_subcommand_reply_enqueue,
        .time_ms = fake_time_ms,
    };
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

static int get_status_combines_app_and_runtime_status(void) {
    swbt_app_t *app = swbt_app_create();
    swbt_runtime_host_t runtime;
    swbt_control_t control;
    swbt_control_status_t status;
    fake_runtime_backend_t fake = {0};
    const swbt_runtime_host_backend_t backend = fake_runtime_backend();
    swbt_state_t state = swbt_state_neutral();
    int failed = 0;

    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_runtime_host_init(&runtime,
                                                   &(swbt_runtime_host_config_t){
                                                       .app = app,
                                                   },
                                                   &backend, &fake),
                            SWBT_RUNTIME_HOST_OK);
    failed += expect_eq_int(swbt_runtime_host_start(&runtime), SWBT_RUNTIME_HOST_OK);
    failed += expect_eq_int(swbt_control_init(&control,
                                              &(swbt_control_config_t){
                                                  .app = app,
                                                  .runtime = &runtime,
                                              }),
                            SWBT_CONTROL_OK);

    state.buttons = SWBT_BUTTON_A;
    failed += expect_eq_int(swbt_control_submit_client_state(&control, 1001u, &state, 7u),
                            SWBT_CONTROL_ERROR_NOT_OWNER);
    failed += expect_eq_int(swbt_control_acquire_client(&control, 1001u), SWBT_CONTROL_OK);
    failed += expect_eq_int(swbt_control_submit_client_state(&control, 1001u, &state, 7u),
                            SWBT_CONTROL_OK);

    failed += expect_eq_int(swbt_control_get_status(&control, &status), SWBT_CONTROL_OK);
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

    swbt_runtime_host_stop(&runtime);
    swbt_app_destroy(app);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += submit_client_state_preserves_owner_and_sequence_semantics();
    failed += submit_state_uses_control_owned_owner_and_sequence();
    failed += get_status_combines_app_and_runtime_status();
    return failed == 0 ? 0 : 1;
}
