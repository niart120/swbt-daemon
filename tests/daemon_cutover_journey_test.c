#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/output_report_handler.h"
#include "daemon/config.h"
#include "daemon/runtime.h"
#include "ipc/ipc_session.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_report.h"

typedef struct {
    int ipc_start_calls;
    int ipc_stop_calls;
    int hid_register_calls;
    int hid_stop_calls;
    int output_handler_start_calls;
    int output_handler_stop_calls;
    int report_timer_start_calls;
    int report_timer_stop_calls;
    swbt_ipc_session_t *ipc_session;
    swbt_daemon_state_provider_t state_provider;
    void *state_context;
} fake_backend_t;

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int fake_ipc_start(void *context, swbt_ipc_session_t *session) {
    fake_backend_t *fake = context;
    fake->ipc_start_calls += 1;
    fake->ipc_session = session;
    return 0;
}

static void fake_ipc_stop(void *context) {
    fake_backend_t *fake = context;
    fake->ipc_stop_calls += 1;
}

static int fake_hid_register(void *context) {
    fake_backend_t *fake = context;
    fake->hid_register_calls += 1;
    return 0;
}

static void fake_hid_stop(void *context) {
    fake_backend_t *fake = context;
    fake->hid_stop_calls += 1;
}

static void fake_output_handler_start(void *context,
                                      swbt_btstack_output_report_handler_t *handler) {
    fake_backend_t *fake = context;
    (void)handler;
    fake->output_handler_start_calls += 1;
}

static void fake_output_handler_stop(void *context) {
    fake_backend_t *fake = context;
    fake->output_handler_stop_calls += 1;
}

static int fake_report_timer_start(void *context, swbt_daemon_state_provider_t state_provider,
                                   void *state_context) {
    fake_backend_t *fake = context;
    fake->report_timer_start_calls += 1;
    fake->state_provider = state_provider;
    fake->state_context = state_context;
    return 0;
}

static void fake_report_timer_stop(void *context) {
    fake_backend_t *fake = context;
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
    return 123u;
}

static swbt_daemon_runtime_backend_t fake_backend_ops(void) {
    return (swbt_daemon_runtime_backend_t){
        .ipc_start = fake_ipc_start,
        .ipc_stop = fake_ipc_stop,
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

static swbt_state_t button_a_state(void) {
    swbt_state_t state = swbt_state_neutral();
    state.buttons = SWBT_BUTTON_A;
    return state;
}

static int expect_current_report_button_byte(const fake_backend_t *fake,
                                             const swbt_daemon_config_t *config,
                                             uint8_t expected_button_byte) {
    uint8_t report[SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE];
    size_t written = 0u;
    swbt_state_t state;
    swbt_switch_report_options_t options;

    if (fake == NULL || fake->state_provider == NULL || config == NULL) {
        return 1;
    }

    state = fake->state_provider(fake->state_context);
    options = config->report_options;
    options.timer = 0x42u;
    if (swbt_switch_build_standard_full_report(&state, &options, report, sizeof(report),
                                               &written) != SWBT_SWITCH_REPORT_OK) {
        return 2;
    }
    if (written != SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE) {
        return 3;
    }
    return expect_eq_u8(report[3], expected_button_byte);
}

static int expect_session_status(const swbt_ipc_session_t *session, bool has_owner,
                                 uint32_t owner_client_id, uint32_t buttons) {
    swbt_ipc_status_t status;
    int failed = 0;

    failed += expect_eq_int(swbt_ipc_get_status(session, &status), SWBT_IPC_OK);
    failed += expect_true(status.has_owner == has_owner);
    if (has_owner) {
        failed += expect_eq_u32(status.owner_client_id, owner_client_id);
    }
    failed += expect_eq_u32(status.state.buttons, buttons);
    return failed;
}

static int cutover_synthetic_journey_neutralizes_disconnect_and_shutdown(void) {
    swbt_daemon_runtime_t runtime;
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_backend_t fake = {0};
    const swbt_daemon_runtime_backend_t backend = fake_backend_ops();
    const swbt_state_t button_a = button_a_state();
    int failed = 0;

    failed += expect_eq_int(swbt_daemon_runtime_init(&runtime, &config, &backend, &fake),
                            SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(swbt_daemon_runtime_start(&runtime), SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(fake.ipc_start_calls, 1);
    failed += expect_eq_int(fake.hid_register_calls, 1);
    failed += expect_eq_int(fake.output_handler_start_calls, 1);
    failed += expect_eq_int(fake.report_timer_start_calls, 1);

    failed += expect_eq_int(swbt_ipc_acquire(fake.ipc_session, 1001u), SWBT_IPC_OK);
    failed +=
        expect_eq_int(swbt_ipc_set_state(fake.ipc_session, 1001u, &button_a, 1u), SWBT_IPC_OK);
    failed += expect_session_status(fake.ipc_session, true, 1001u, SWBT_BUTTON_A);
    failed += expect_current_report_button_byte(&fake, &config, 0x08u);

    failed += expect_eq_int(swbt_ipc_disconnect(fake.ipc_session, 1001u), SWBT_IPC_OK);
    failed += expect_session_status(fake.ipc_session, false, 0u, 0u);
    failed += expect_current_report_button_byte(&fake, &config, 0x00u);

    failed += expect_eq_int(swbt_ipc_acquire(fake.ipc_session, 2002u), SWBT_IPC_OK);
    failed +=
        expect_eq_int(swbt_ipc_set_state(fake.ipc_session, 2002u, &button_a, 2u), SWBT_IPC_OK);
    failed += expect_session_status(fake.ipc_session, true, 2002u, SWBT_BUTTON_A);
    failed += expect_current_report_button_byte(&fake, &config, 0x08u);

    swbt_daemon_runtime_stop(&runtime);
    failed += expect_eq_int(fake.report_timer_stop_calls, 1);
    failed += expect_eq_int(fake.output_handler_stop_calls, 1);
    failed += expect_eq_int(fake.hid_stop_calls, 1);
    failed += expect_eq_int(fake.ipc_stop_calls, 1);
    failed += expect_session_status(fake.ipc_session, false, 0u, 0u);
    failed += expect_current_report_button_byte(&fake, &config, 0x00u);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += cutover_synthetic_journey_neutralizes_disconnect_and_shutdown();
    return failed == 0 ? 0 : 1;
}
