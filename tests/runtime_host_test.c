#include <stdbool.h>
#include <stdint.h>

#include "domain/domain.h"
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

static swbt_runtime_host_config_t fake_config(void) {
    return (swbt_runtime_host_config_t){
        .daemon_status =
            {
                .backend = SWBT_DOMAIN_DAEMON_BACKEND_NOOP,
                .lifecycle_state = SWBT_DOMAIN_DAEMON_LIFECYCLE_STOPPED,
                .hardware_approval = SWBT_DOMAIN_HARDWARE_APPROVAL_UNAVAILABLE,
            },
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
    fake_runtime_backend_t fake = {0};
    const swbt_runtime_host_backend_t backend = fake_backend();
    const swbt_runtime_host_config_t config = fake_config();
    const swbt_state_t state = sample_state();
    swbt_domain_t *app;

    int failed = 0;
    failed += expect_eq_int(swbt_runtime_host_init(&runtime, &config, &backend, &fake),
                            SWBT_RUNTIME_HOST_OK);
    app = swbt_runtime_host_app(&runtime);
    failed += expect_true(app != NULL);
    failed += expect_true(swbt_runtime_host_control(&runtime) != NULL);
    failed += expect_eq_int(swbt_runtime_host_start(&runtime), SWBT_RUNTIME_HOST_OK);
    failed += expect_eq_int(fake.hid_register_calls, 1);
    failed += expect_eq_int(fake.output_handler_start_calls, 1);
    failed += expect_eq_int(fake.report_timer_start_calls, 1);
    failed += expect_true(fake.output_handler != NULL);
    failed += expect_true(fake.state_provider != NULL);
    failed += expect_true(fake.state_context == &runtime);

    failed += expect_eq_int(swbt_domain_acquire(app, 1001u), SWBT_DOMAIN_OK);
    failed += expect_eq_int(swbt_domain_set_state(app,
                                                  (swbt_domain_set_state_options_t){
                                                      .client_id = 1001u,
                                                      .state = &state,
                                                      .sequence = 7u,
                                                  }),
                            SWBT_DOMAIN_OK);
    if (fake.state_provider == NULL) {
        failed += 1;
    } else {
        failed += expect_eq_u16(fake.state_provider(fake.state_context).ly, 2345u);
    }

    swbt_runtime_host_destroy(&runtime);
    return failed;
}

static int runtime_owns_control_and_combines_runtime_status(void) {
    swbt_runtime_host_t runtime;
    fake_runtime_backend_t fake = {0};
    const swbt_runtime_host_backend_t backend = fake_backend();
    const swbt_runtime_host_config_t config = fake_config();
    const swbt_state_t state = sample_state();
    swbt_control_status_t status;
    swbt_control_t *control;

    int failed = 0;
    failed += expect_eq_int(swbt_runtime_host_init(&runtime, &config, &backend, &fake),
                            SWBT_RUNTIME_HOST_OK);
    control = swbt_runtime_host_control(&runtime);
    failed += expect_true(control != NULL);
    failed += expect_eq_int(swbt_control_acquire_client(control, 1001u), SWBT_CONTROL_OK);
    failed += expect_eq_int(swbt_control_submit_client_state(control, 1001u, &state, 7u),
                            SWBT_CONTROL_OK);

    failed += expect_eq_int(swbt_runtime_host_start(&runtime), SWBT_RUNTIME_HOST_OK);
    failed += expect_eq_int(swbt_control_get_status(control, &status), SWBT_CONTROL_OK);
    failed += expect_true(status.app.has_owner);
    failed += expect_eq_int((int)status.app.owner_client_id, 1001);
    failed += expect_eq_int((int)status.app.last_sequence, 7);
    failed += expect_true(status.has_runtime_status);
    failed += expect_true(status.runtime.initialized);
    failed += expect_true(status.runtime.running);
    failed += expect_true(status.runtime.hid_registered);
    failed += expect_true(status.runtime.output_handler_started);
    failed += expect_true(status.runtime.report_timer_started);

    swbt_runtime_host_destroy(&runtime);
    return failed;
}

static int shutdown_neutralizes_state_and_stops_runtime_resources_once(void) {
    swbt_runtime_host_t runtime;
    fake_runtime_backend_t fake = {0};
    const swbt_runtime_host_backend_t backend = fake_backend();
    const swbt_runtime_host_config_t config = fake_config();
    const swbt_state_t state = sample_state();
    swbt_state_t read_state;
    swbt_domain_t *app;

    int failed = 0;
    failed += expect_eq_int(swbt_runtime_host_init(&runtime, &config, &backend, &fake),
                            SWBT_RUNTIME_HOST_OK);
    app = swbt_runtime_host_app(&runtime);
    failed += expect_true(app != NULL);
    failed += expect_eq_int(swbt_runtime_host_start(&runtime), SWBT_RUNTIME_HOST_OK);
    failed += expect_eq_int(swbt_domain_acquire(app, 1001u), SWBT_DOMAIN_OK);
    failed += expect_eq_int(swbt_domain_set_state(app,
                                                  (swbt_domain_set_state_options_t){
                                                      .client_id = 1001u,
                                                      .state = &state,
                                                      .sequence = 7u,
                                                  }),
                            SWBT_DOMAIN_OK);

    swbt_runtime_host_stop(&runtime);
    swbt_runtime_host_stop(&runtime);

    failed += expect_eq_int(fake.report_timer_stop_calls, 1);
    failed += expect_eq_int(fake.output_handler_stop_calls, 1);
    failed += expect_eq_int(fake.hid_stop_calls, 1);
    failed += expect_true(!swbt_runtime_host_is_running(&runtime));
    failed += expect_eq_int(swbt_domain_read_controller_state(app, &read_state), SWBT_DOMAIN_OK);
    failed += expect_eq_int((int)read_state.buttons, 0);
    failed += expect_eq_u16(read_state.lx, 2048u);

    swbt_runtime_host_destroy(&runtime);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += start_wires_hid_output_and_report_runtime_without_ipc_callback();
    failed += runtime_owns_control_and_combines_runtime_status();
    failed += shutdown_neutralizes_state_and_stops_runtime_resources_once();
    return failed == 0 ? 0 : 1;
}
