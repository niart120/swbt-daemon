#include <stdbool.h>
#include <stdint.h>

#include "application/app.h"
#include "btstack_bridge/output_report_handler.h"
#include "runtime/host.h"
#include "switch/switch_controller_state.h"

typedef struct {
    int hid_register_calls;
    int hid_stop_calls;
    int output_handler_start_calls;
    int output_handler_stop_calls;
    int report_timer_start_calls;
    int report_timer_stop_calls;
    int report_timer_send_neutral_now_calls;
    int subcommand_reply_enqueue_calls;
    swbt_runtime_state_provider_t state_provider;
    void *state_context;
    const swbt_btstack_output_report_handler_t *output_handler;
} fake_runtime_backend_t;

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected) {
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
    fake->output_handler_start_calls += 1;
    fake->output_handler = handler;
}

static void fake_output_handler_stop(void *context) {
    fake_runtime_backend_t *fake = context;
    fake->output_handler_stop_calls += 1;
}

static int fake_report_timer_start(void *context, swbt_runtime_state_provider_t state_provider,
                                   void *state_context) {
    fake_runtime_backend_t *fake = context;
    fake->report_timer_start_calls += 1;
    fake->state_provider = state_provider;
    fake->state_context = state_context;
    return 0;
}

static void fake_report_timer_stop(void *context) {
    fake_runtime_backend_t *fake = context;
    fake->report_timer_stop_calls += 1;
}

static int fake_report_timer_send_neutral_now(void *context) {
    fake_runtime_backend_t *fake = context;
    fake->report_timer_send_neutral_now_calls += 1;
    return 0;
}

static int fake_subcommand_reply_enqueue(void *context, uint16_t hid_cid, const uint8_t *report,
                                         size_t report_size) {
    fake_runtime_backend_t *fake = context;
    (void)hid_cid;
    (void)report;
    (void)report_size;
    fake->subcommand_reply_enqueue_calls += 1;
    return 0;
}

static uint32_t fake_time_ms(void *context) {
    (void)context;
    return 0u;
}

static swbt_runtime_host_backend_t fake_backend(void) {
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

static swbt_state_t sample_state(void) {
    swbt_state_t state = swbt_state_neutral();
    state.buttons = SWBT_BUTTON_A;
    state.ly = 2345u;
    return state;
}

static int start_wires_hid_output_and_report_runtime_without_ipc_callback(void) {
    swbt_runtime_host_t runtime;
    swbt_app_t *app = swbt_app_create();
    fake_runtime_backend_t fake = {0};
    const swbt_runtime_host_backend_t backend = fake_backend();
    const swbt_state_t state = sample_state();

    int failed = 0;
    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_runtime_host_init(&runtime,
                                                   &(swbt_runtime_host_config_t){
                                                       .app = app,
                                                   },
                                                   &backend, &fake),
                            SWBT_RUNTIME_HOST_OK);
    failed += expect_eq_int(swbt_runtime_host_start(&runtime), SWBT_RUNTIME_HOST_OK);
    failed += expect_eq_int(fake.hid_register_calls, 1);
    failed += expect_eq_int(fake.output_handler_start_calls, 1);
    failed += expect_eq_int(fake.report_timer_start_calls, 1);
    failed += expect_true(fake.output_handler != NULL);
    failed += expect_true(fake.state_provider != NULL);
    failed += expect_true(fake.state_context == &runtime);

    failed += expect_eq_int(swbt_app_acquire(app, 1001u), SWBT_APP_OK);
    failed += expect_eq_int(swbt_app_set_state(app,
                                               (swbt_app_set_state_options_t){
                                                   .client_id = 1001u,
                                                   .state = &state,
                                                   .sequence = 7u,
                                               }),
                            SWBT_APP_OK);
    failed += expect_eq_u16(fake.state_provider(fake.state_context).ly, 2345u);

    swbt_runtime_host_stop(&runtime);
    swbt_app_destroy(app);
    return failed;
}

static int runtime_status_tracks_resources_without_owning_application_state(void) {
    swbt_runtime_host_t runtime;
    swbt_app_t *app = swbt_app_create();
    fake_runtime_backend_t fake = {0};
    const swbt_runtime_host_backend_t backend = fake_backend();
    const swbt_state_t state = sample_state();
    swbt_runtime_host_status_t status;
    swbt_app_status_snapshot_t app_status;

    int failed = 0;
    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_app_acquire(app, 1001u), SWBT_APP_OK);
    failed += expect_eq_int(swbt_app_set_state(app,
                                               (swbt_app_set_state_options_t){
                                                   .client_id = 1001u,
                                                   .state = &state,
                                                   .sequence = 7u,
                                               }),
                            SWBT_APP_OK);
    failed += expect_eq_int(swbt_runtime_host_init(&runtime,
                                                   &(swbt_runtime_host_config_t){
                                                       .app = app,
                                                   },
                                                   &backend, &fake),
                            SWBT_RUNTIME_HOST_OK);

    failed += expect_eq_int(swbt_runtime_host_status(&runtime, &status), SWBT_RUNTIME_HOST_OK);
    failed += expect_true(status.initialized);
    failed += expect_true(!status.running);
    failed += expect_true(!status.hid_registered);
    failed += expect_true(!status.output_handler_started);
    failed += expect_true(!status.report_timer_started);

    failed += expect_eq_int(swbt_runtime_host_start(&runtime), SWBT_RUNTIME_HOST_OK);
    failed += expect_eq_int(swbt_runtime_host_status(&runtime, &status), SWBT_RUNTIME_HOST_OK);
    failed += expect_true(status.running);
    failed += expect_true(status.hid_registered);
    failed += expect_true(status.output_handler_started);
    failed += expect_true(status.report_timer_started);

    failed += expect_eq_int(swbt_app_read_status(app, &app_status), SWBT_APP_OK);
    failed += expect_true(app_status.has_owner);
    failed += expect_eq_int((int)app_status.owner_client_id, 1001);
    failed += expect_eq_int((int)app_status.last_sequence, 7);

    swbt_runtime_host_stop(&runtime);
    failed += expect_eq_int(swbt_runtime_host_status(&runtime, &status), SWBT_RUNTIME_HOST_OK);
    failed += expect_true(!status.running);
    failed += expect_true(!status.hid_registered);
    failed += expect_true(!status.output_handler_started);
    failed += expect_true(!status.report_timer_started);
    failed += expect_eq_int(swbt_app_read_status(app, &app_status), SWBT_APP_OK);

    swbt_app_destroy(app);
    return failed;
}

static int shutdown_neutralizes_state_and_stops_runtime_resources_once(void) {
    swbt_runtime_host_t runtime;
    swbt_app_t *app = swbt_app_create();
    fake_runtime_backend_t fake = {0};
    const swbt_runtime_host_backend_t backend = fake_backend();
    const swbt_state_t state = sample_state();
    swbt_state_t read_state;

    int failed = 0;
    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_runtime_host_init(&runtime,
                                                   &(swbt_runtime_host_config_t){
                                                       .app = app,
                                                   },
                                                   &backend, &fake),
                            SWBT_RUNTIME_HOST_OK);
    failed += expect_eq_int(swbt_runtime_host_start(&runtime), SWBT_RUNTIME_HOST_OK);
    failed += expect_eq_int(swbt_app_acquire(app, 1001u), SWBT_APP_OK);
    failed += expect_eq_int(swbt_app_set_state(app,
                                               (swbt_app_set_state_options_t){
                                                   .client_id = 1001u,
                                                   .state = &state,
                                                   .sequence = 7u,
                                               }),
                            SWBT_APP_OK);

    swbt_runtime_host_stop(&runtime);
    swbt_runtime_host_stop(&runtime);

    failed += expect_eq_int(fake.report_timer_stop_calls, 1);
    failed += expect_eq_int(fake.output_handler_stop_calls, 1);
    failed += expect_eq_int(fake.hid_stop_calls, 1);
    failed += expect_true(!swbt_runtime_host_is_running(&runtime));
    failed += expect_eq_int(swbt_app_read_controller_state(app, &read_state), SWBT_APP_OK);
    failed += expect_eq_int((int)read_state.buttons, 0);
    failed += expect_eq_u16(read_state.lx, 2048u);

    swbt_app_destroy(app);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += start_wires_hid_output_and_report_runtime_without_ipc_callback();
    failed += runtime_status_tracks_resources_without_owning_application_state();
    failed += shutdown_neutralizes_state_and_stops_runtime_resources_once();
    return failed == 0 ? 0 : 1;
}
