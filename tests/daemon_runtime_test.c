#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "daemon/config.h"
#include "daemon/runtime.h"
#include "ipc/ipc_session.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_device_info.h"
#include "switch/switch_subcommand.h"
#include "switch/switch_subcommand_reply.h"

typedef struct {
    int ipc_start_result;
    int hid_register_result;
    int report_timer_start_result;
    int ipc_start_calls;
    int ipc_stop_calls;
    int hid_register_calls;
    int hid_stop_calls;
    int output_handler_start_calls;
    int output_handler_stop_calls;
    int report_timer_start_calls;
    int report_timer_stop_calls;
    int report_timer_send_neutral_now_calls;
    int subcommand_reply_enqueue_calls;
    int read_device_info_calls;
    const swbt_ipc_session_t *ipc_session;
    const swbt_btstack_output_report_handler_t *output_handler;
    swbt_daemon_state_provider_t state_provider;
    void *state_context;
    uint16_t reply_hid_cid;
    uint8_t reply_report[SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE];
    size_t reply_report_size;
    swbt_switch_device_info_t device_info;
} fake_backend_t;

static int expect_true(bool value) {
    return value ? 0 : 1;
}

static int expect_false(bool value) {
    return value ? 1 : 0;
}

static int expect_eq_int(int actual, int expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_str_eq(const char *actual, const char *expected) {
    if (actual == NULL || expected == NULL) {
        return actual == expected ? 0 : 1;
    }
    return strcmp(actual, expected) == 0 ? 0 : 1;
}

static int expect_config_eq(const swbt_daemon_config_t *actual,
                            const swbt_daemon_config_t *expected) {
    int failed = 0;
    failed += expect_eq_u32(actual->report_period_us, expected->report_period_us);
    failed += expect_str_eq(actual->ipc_host, expected->ipc_host);
    failed += expect_eq_u16(actual->ipc_port, expected->ipc_port);
    failed += expect_eq_int(actual->ipc_backlog, expected->ipc_backlog);
    failed += expect_eq_u32(actual->ipc_heartbeat_timeout_ms,
                            expected->ipc_heartbeat_timeout_ms);
    failed += expect_eq_u8(actual->report_options.battery_connection,
                           expected->report_options.battery_connection);
    failed += expect_eq_u8(actual->report_options.vibrator_report,
                           expected->report_options.vibrator_report);
    failed += expect_eq_u8(actual->device_info.firmware_version[0],
                           expected->device_info.firmware_version[0]);
    failed += expect_eq_u8(actual->device_info.firmware_version[1],
                           expected->device_info.firmware_version[1]);
    failed += expect_eq_u8(actual->device_info.controller_type, expected->device_info.controller_type);
    failed += expect_eq_u8(actual->device_info.tail_unknown, expected->device_info.tail_unknown);
    failed += expect_eq_u8(actual->device_info.color_source, expected->device_info.color_source);
    return failed;
}

static void fake_backend_init(fake_backend_t *fake) {
    *fake = (fake_backend_t){0};
    fake->device_info = swbt_switch_device_info_default();
}

static int fake_ipc_start(void *context, swbt_ipc_session_t *session) {
    fake_backend_t *fake = context;
    fake->ipc_start_calls += 1;
    fake->ipc_session = session;
    return fake->ipc_start_result;
}

static void fake_ipc_stop(void *context) {
    fake_backend_t *fake = context;
    fake->ipc_stop_calls += 1;
}

static int fake_hid_register(void *context) {
    fake_backend_t *fake = context;
    fake->hid_register_calls += 1;
    return fake->hid_register_result;
}

static void fake_hid_stop(void *context) {
    fake_backend_t *fake = context;
    fake->hid_stop_calls += 1;
}

static void fake_output_handler_start(void *context,
                                      swbt_btstack_output_report_handler_t *handler) {
    fake_backend_t *fake = context;
    fake->output_handler_start_calls += 1;
    fake->output_handler = handler;
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
    return fake->report_timer_start_result;
}

static void fake_report_timer_stop(void *context) {
    fake_backend_t *fake = context;
    fake->report_timer_stop_calls += 1;
}

static int fake_report_timer_send_neutral_now(void *context) {
    fake_backend_t *fake = context;
    fake->report_timer_send_neutral_now_calls += 1;
    return 0;
}

static int fake_subcommand_reply_enqueue(void *context, uint16_t hid_cid, const uint8_t *report,
                                         size_t report_size) {
    fake_backend_t *fake = context;
    fake->subcommand_reply_enqueue_calls += 1;
    fake->reply_hid_cid = hid_cid;
    fake->reply_report_size = report_size;
    for (size_t index = 0; index < report_size && index < sizeof(fake->reply_report); ++index) {
        fake->reply_report[index] = report[index];
    }
    return 0;
}

static int fake_read_device_info(void *context, swbt_switch_device_info_t *out_device_info) {
    fake_backend_t *fake = context;
    fake->read_device_info_calls += 1;
    *out_device_info = fake->device_info;
    return 0;
}

static swbt_daemon_runtime_backend_t fake_backend_ops(void) {
    swbt_daemon_runtime_backend_t backend = {
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
        .read_device_info = fake_read_device_info,
    };
    return backend;
}

static swbt_state_t sample_state(void) {
    swbt_state_t state = swbt_state_neutral();
    state.buttons = SWBT_BUTTON_A | SWBT_BUTTON_X;
    state.lx = 1234u;
    state.ly = 2345u;
    state.client_seq = 7u;
    return state;
}

static int invalid_config_rejects_without_opening_backends(void) {
    swbt_daemon_runtime_t runtime;
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_backend_t fake;
    const swbt_daemon_runtime_backend_t backend = fake_backend_ops();

    fake_backend_init(&fake);
    config.report_period_us = 0u;

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_runtime_init(&runtime, &config, &backend, &fake),
                            SWBT_DAEMON_RUNTIME_ERROR_INVALID_ARGUMENT);
    failed += expect_eq_int(fake.ipc_start_calls, 0);
    failed += expect_eq_int(fake.hid_register_calls, 0);
    failed += expect_eq_int(fake.report_timer_start_calls, 0);
    return failed;
}

static int start_wires_session_mailbox_hid_output_and_timer(void) {
    swbt_daemon_runtime_t runtime;
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_backend_t fake;
    const swbt_daemon_runtime_backend_t backend = fake_backend_ops();
    swbt_state_mailbox_snapshot_t snapshot;
    const swbt_state_t state = sample_state();

    fake_backend_init(&fake);

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_runtime_init(&runtime, &config, &backend, &fake),
                            SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(swbt_daemon_runtime_start(&runtime), SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(fake.ipc_start_calls, 1);
    failed += expect_eq_int(fake.hid_register_calls, 1);
    failed += expect_eq_int(fake.output_handler_start_calls, 1);
    failed += expect_eq_int(fake.report_timer_start_calls, 1);
    failed += expect_true(fake.ipc_session == swbt_daemon_runtime_ipc_session(&runtime));
    failed += expect_true(fake.output_handler == swbt_daemon_runtime_output_handler(&runtime));
    failed += expect_true(fake.state_provider != NULL);
    failed += expect_true(fake.state_context == &runtime);

    failed += expect_eq_int(swbt_ipc_acquire(swbt_daemon_runtime_ipc_session(&runtime), 1001u),
                            SWBT_IPC_OK);
    failed += expect_eq_int(
        swbt_ipc_set_state(swbt_daemon_runtime_ipc_session(&runtime), 1001u, &state), SWBT_IPC_OK);
    failed +=
        expect_eq_int(swbt_state_mailbox_load(swbt_daemon_runtime_mailbox(&runtime), &snapshot),
                      SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u32(snapshot.state.buttons, SWBT_BUTTON_A | SWBT_BUTTON_X);
    failed += expect_eq_u16(snapshot.state.lx, 1234u);
    if (fake.state_provider == NULL) {
        failed += 1;
    } else {
        failed += expect_eq_u16(fake.state_provider(fake.state_context).ly, 2345u);
    }

    swbt_daemon_runtime_stop(&runtime);
    return failed;
}

static int output_report_dispatcher_response_enqueues_reply(void) {
    const uint8_t set_report_mode[] = {
        SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND,
        0x0Au,
        0x00u,
        0x01u,
        0x40u,
        0x40u,
        0x00u,
        0x01u,
        0x40u,
        0x40u,
        SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE,
        0x30u,
    };
    swbt_daemon_runtime_t runtime;
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_backend_t fake;
    const swbt_daemon_runtime_backend_t backend = fake_backend_ops();

    fake_backend_init(&fake);

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_runtime_init(&runtime, &config, &backend, &fake),
                            SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(swbt_daemon_runtime_start(&runtime), SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(
        swbt_btstack_output_report_handler_handle(swbt_daemon_runtime_output_handler(&runtime),
                                                  0x0042u, SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT, 0u,
                                                  set_report_mode, sizeof(set_report_mode)),
        SWBT_BTSTACK_OUTPUT_REPORT_OK);
    failed += expect_eq_int(fake.subcommand_reply_enqueue_calls, 1);
    failed += expect_eq_u16(fake.reply_hid_cid, 0x0042u);
    failed +=
        expect_eq_int((int)fake.reply_report_size, (int)SWBT_SWITCH_SUBCOMMAND_REPLY_REPORT_SIZE);
    failed += expect_eq_int(fake.reply_report[0], SWBT_SWITCH_INPUT_REPORT_SUBCOMMAND_REPLY);
    failed += expect_eq_int(fake.reply_report[13], SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_SIMPLE);
    failed += expect_eq_int(fake.reply_report[14], SWBT_SWITCH_SUBCOMMAND_SET_REPORT_MODE);

    swbt_daemon_runtime_stop(&runtime);
    return failed;
}

static int output_report_device_info_uses_backend_identity(void) {
    const uint8_t request_device_info[] = {
        SWBT_SWITCH_OUTPUT_REPORT_RUMBLE_AND_SUBCOMMAND,
        0x0Au,
        0x00u,
        0x00u,
        0x00u,
        0x00u,
        0x00u,
        0x00u,
        0x00u,
        0x00u,
        SWBT_SWITCH_SUBCOMMAND_REQUEST_DEVICE_INFO,
    };
    const uint8_t address[] = {0x00u, 0x1Bu, 0xDCu, 0xF9u, 0x9Fu, 0x7Du};
    swbt_daemon_runtime_t runtime;
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_backend_t fake;
    const swbt_daemon_runtime_backend_t backend = fake_backend_ops();

    fake_backend_init(&fake);
    for (size_t index = 0; index < sizeof(address); ++index) {
        fake.device_info.bluetooth_address[index] = address[index];
    }

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_runtime_init(&runtime, &config, &backend, &fake),
                            SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(swbt_daemon_runtime_start(&runtime), SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(
        swbt_btstack_output_report_handler_handle(swbt_daemon_runtime_output_handler(&runtime),
                                                  0x0042u, SWBT_BTSTACK_HID_REPORT_TYPE_OUTPUT, 0u,
                                                  request_device_info, sizeof(request_device_info)),
        SWBT_BTSTACK_OUTPUT_REPORT_OK);
    failed += expect_eq_int(fake.read_device_info_calls, 1);
    failed += expect_eq_int(fake.subcommand_reply_enqueue_calls, 1);
    failed += expect_eq_int(fake.reply_report[13], SWBT_SWITCH_SUBCOMMAND_REPLY_ACK_DEVICE_INFO);
    failed += expect_eq_int(fake.reply_report[14], SWBT_SWITCH_SUBCOMMAND_REQUEST_DEVICE_INFO);
    for (size_t index = 0; index < sizeof(address); ++index) {
        failed +=
            expect_eq_int(fake.reply_report[SWBT_SWITCH_SUBCOMMAND_REPLY_DATA_OFFSET + 4u + index],
                          address[index]);
    }

    swbt_daemon_runtime_stop(&runtime);
    return failed;
}

static int send_neutral_now_clears_owner_and_flushes_report_timer(void) {
    swbt_daemon_runtime_t runtime;
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_backend_t fake;
    const swbt_daemon_runtime_backend_t backend = fake_backend_ops();
    const swbt_state_t state = sample_state();
    swbt_state_mailbox_snapshot_t snapshot;

    fake_backend_init(&fake);

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_runtime_init(&runtime, &config, &backend, &fake),
                            SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(swbt_daemon_runtime_start(&runtime), SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(swbt_ipc_acquire(swbt_daemon_runtime_ipc_session(&runtime), 1001u),
                            SWBT_IPC_OK);
    failed += expect_eq_int(
        swbt_ipc_set_state(swbt_daemon_runtime_ipc_session(&runtime), 1001u, &state), SWBT_IPC_OK);

    failed += expect_eq_int(swbt_daemon_runtime_send_neutral_now(&runtime), SWBT_DAEMON_RUNTIME_OK);

    failed += expect_eq_int(fake.report_timer_send_neutral_now_calls, 1);
    failed +=
        expect_eq_int(swbt_state_mailbox_load(swbt_daemon_runtime_mailbox(&runtime), &snapshot),
                      SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u32(snapshot.state.buttons, 0u);
    failed += expect_eq_u16(snapshot.state.lx, 2048u);

    swbt_daemon_runtime_stop(&runtime);
    return failed;
}

static int shutdown_neutralizes_state_and_stops_resources_once(void) {
    swbt_daemon_runtime_t runtime;
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_backend_t fake;
    const swbt_daemon_runtime_backend_t backend = fake_backend_ops();
    const swbt_state_t state = sample_state();
    swbt_state_mailbox_snapshot_t snapshot;

    fake_backend_init(&fake);

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_runtime_init(&runtime, &config, &backend, &fake),
                            SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(swbt_daemon_runtime_start(&runtime), SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(swbt_ipc_acquire(swbt_daemon_runtime_ipc_session(&runtime), 1001u),
                            SWBT_IPC_OK);
    failed += expect_eq_int(
        swbt_ipc_set_state(swbt_daemon_runtime_ipc_session(&runtime), 1001u, &state), SWBT_IPC_OK);

    swbt_daemon_runtime_stop(&runtime);
    swbt_daemon_runtime_stop(&runtime);

    failed += expect_eq_int(fake.report_timer_stop_calls, 1);
    failed += expect_eq_int(fake.output_handler_stop_calls, 1);
    failed += expect_eq_int(fake.hid_stop_calls, 1);
    failed += expect_eq_int(fake.ipc_stop_calls, 1);
    failed +=
        expect_eq_int(swbt_state_mailbox_load(swbt_daemon_runtime_mailbox(&runtime), &snapshot),
                      SWBT_STATE_MAILBOX_OK);
    failed += expect_eq_u32(snapshot.state.buttons, 0u);
    failed += expect_eq_u16(snapshot.state.lx, 2048u);
    failed += expect_false(swbt_daemon_runtime_is_running(&runtime));
    return failed;
}

static int backend_failure_cleans_up_started_resources(void) {
    swbt_daemon_runtime_t runtime;
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_backend_t fake;
    const swbt_daemon_runtime_backend_t backend = fake_backend_ops();

    fake_backend_init(&fake);
    fake.report_timer_start_result = -7;

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_runtime_init(&runtime, &config, &backend, &fake),
                            SWBT_DAEMON_RUNTIME_OK);
    failed += expect_eq_int(swbt_daemon_runtime_start(&runtime), SWBT_DAEMON_RUNTIME_ERROR_BACKEND);
    failed += expect_eq_int(fake.ipc_start_calls, 1);
    failed += expect_eq_int(fake.hid_register_calls, 1);
    failed += expect_eq_int(fake.output_handler_start_calls, 1);
    failed += expect_eq_int(fake.report_timer_start_calls, 1);
    failed += expect_eq_int(fake.report_timer_stop_calls, 0);
    failed += expect_eq_int(fake.output_handler_stop_calls, 1);
    failed += expect_eq_int(fake.hid_stop_calls, 1);
    failed += expect_eq_int(fake.ipc_stop_calls, 1);
    failed += expect_false(swbt_daemon_runtime_is_running(&runtime));
    return failed;
}

static int main_with_backend_returns_runtime_exit_without_hardware_backend(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_backend_t fake;
    const swbt_daemon_runtime_backend_t backend = fake_backend_ops();

    fake_backend_init(&fake);

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_main_with_backend(&config, &backend, &fake), 0);
    failed += expect_eq_int(fake.ipc_start_calls, 1);
    failed += expect_eq_int(fake.hid_register_calls, 1);
    failed += expect_eq_int(fake.report_timer_start_calls, 1);
    failed += expect_eq_int(fake.ipc_stop_calls, 1);
    return failed;
}

static int default_config_uses_switch_facing_report_options(void) {
    const swbt_daemon_config_t config = swbt_daemon_config_default();

    int failed = 0;
    failed += expect_eq_u8(config.report_options.battery_connection, 0x8Eu);
    failed += expect_eq_u8(config.report_options.vibrator_report, 0x80u);
    return failed;
}

static int config_env_absent_uses_defaults(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_t defaults = swbt_daemon_config_default();
    const swbt_daemon_config_env_t env = {0};

    int failed = 0;
    failed += expect_true(swbt_daemon_config_apply_env(&config, &env));
    failed += expect_config_eq(&config, &defaults);
    return failed;
}

static int config_env_invalid_numeric_rejects_and_preserves_config(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_env_t env = {
        .report_period_us = "0",
    };
    const swbt_daemon_config_t expected = config;

    int failed = 0;
    failed += expect_false(swbt_daemon_config_apply_env(&config, &env));
    failed += expect_config_eq(&config, &expected);
    return failed;
}

static int config_applies_mizuyoukanao_pro_device_info_profile(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();

    int failed = 0;
    failed +=
        expect_true(swbt_daemon_config_apply_device_info_profile(&config, "mizuyoukanao-pro"));
    failed += expect_eq_u8(config.device_info.firmware_version[0], 0x03u);
    failed += expect_eq_u8(config.device_info.firmware_version[1], 0x48u);
    failed += expect_eq_u8(config.device_info.controller_type,
                           SWBT_SWITCH_DEVICE_INFO_CONTROLLER_TYPE_PRO_CONTROLLER);
    failed += expect_eq_u8(config.device_info.tail_unknown,
                           SWBT_SWITCH_DEVICE_INFO_MIZUYOUKANAO_PRO_TAIL_UNKNOWN);
    failed += expect_eq_u8(config.device_info.color_source,
                           SWBT_SWITCH_DEVICE_INFO_MIZUYOUKANAO_PRO_COLOR_SOURCE);
    return failed;
}

static int config_rejects_unknown_device_info_profile(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();

    return expect_false(swbt_daemon_config_apply_device_info_profile(&config, "unknown"));
}

int main(void) {
    int failed = 0;
    failed += default_config_uses_switch_facing_report_options();
    failed += config_env_absent_uses_defaults();
    failed += config_env_invalid_numeric_rejects_and_preserves_config();
    failed += config_applies_mizuyoukanao_pro_device_info_profile();
    failed += config_rejects_unknown_device_info_profile();
    failed += invalid_config_rejects_without_opening_backends();
    failed += start_wires_session_mailbox_hid_output_and_timer();
    failed += output_report_dispatcher_response_enqueues_reply();
    failed += output_report_device_info_uses_backend_identity();
    failed += send_neutral_now_clears_owner_and_flushes_report_timer();
    failed += shutdown_neutralizes_state_and_stops_resources_once();
    failed += backend_failure_cleans_up_started_resources();
    failed += main_with_backend_returns_runtime_exit_without_hardware_backend();
    return failed == 0 ? 0 : 1;
}
