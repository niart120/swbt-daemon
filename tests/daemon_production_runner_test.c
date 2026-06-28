#include "daemon/production_runner_internal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ipc/ipc_adapter.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_hid_descriptor.h"
#include "switch/switch_report.h"
#include "daemon/btstack_process_backend.h"
#include "support/diagnostics.h"

enum {
    STEP_IPC_START = 1,
    STEP_PLATFORM_START = 2,
    STEP_HID_REGISTER = 3,
    STEP_OUTPUT_START = 4,
    STEP_TIMER_INIT = 5,
    STEP_POWER_ON = 6,
    STEP_RUN_LOOP_EXECUTE = 7,
    STEP_POWER_OFF = 8,
    STEP_TIMER_STOP = 9,
    STEP_OUTPUT_STOP = 10,
    STEP_HID_STOP = 11,
    STEP_PLATFORM_STOP = 12,
    STEP_IPC_STOP = 13,
    STEP_SHUTDOWN_INSTALL = 14,
    STEP_RUN_LOOP_TRIGGER_EXIT = 15,
    STEP_SHUTDOWN_UNINSTALL = 16,
    STEP_TIMER_SEND_NEUTRAL_NOW = 17,
    STEP_RUN_LOOP_EXECUTE_ON_MAIN_THREAD = 18,
    STEP_TIMER_CAN_SEND_NOW = 19,
    STEP_ACTIVE_RECONNECT_CONNECT = 20,
    STEP_DEVICE_DISCONNECT = 21,
    STEP_HID_CONNECTION_CLOSED = 22,
    STEP_SHUTDOWN_DISCONNECT_TIMEOUT = 23,
    STEP_ACTIVE_RECONNECT_TIMEOUT = 24,
};

typedef struct {
    int steps[32];
    size_t step_count;
    int timer_init_result;
    int power_on_result;
    int ipc_start_calls;
    int ipc_pump_running_at_start;
    int platform_start_calls;
    int hid_register_calls;
    int timer_start_calls;
    int timer_can_send_calls;
    int timer_can_send_result;
    int timer_send_neutral_now_calls;
    int timer_send_neutral_now_result;
    int timer_stop_calls;
    int inject_json_state_during_run_loop;
    int inject_disconnect_reacquire_during_run_loop;
    int notify_report_tick_failure;
    int injected_json_result;
    int hid_send_calls;
    int device_send_calls;
    int device_disconnect_calls;
    int device_disconnect_result;
    int power_off_calls;
    int active_reconnect_connect_calls;
    int active_reconnect_connect_result;
    int read_controller_address_calls;
    int ssp_confirmation_calls;
    int run_loop_trigger_exit_calls;
    int shutdown_requests_to_fire;
    int fire_active_reconnect_timeout_during_run_loop;
    int fire_active_reconnect_timeout_after_hid_open_completed_and_closed;
    int fire_can_send_after_shutdown_request;
    int fire_closed_after_shutdown_request;
    int fire_disconnect_timeout_after_shutdown_request;
    int skip_shutdown_opened_event;
    int assert_shutdown_waits_for_hid_closed;
    int shutdown_wait_failed;
    int shutdown_disconnect_timer_add_calls;
    int shutdown_disconnect_timer_remove_calls;
    uint32_t shutdown_disconnect_timer_timeout_ms;
    void (*shutdown_disconnect_timer_handler)(btstack_timer_source_t *timer);
    btstack_timer_source_t *shutdown_disconnect_timer;
    uint16_t timer_hid_cid;
    uint16_t last_hid_cid;
    uint16_t last_device_hid_cid;
    uint16_t device_disconnect_hid_cid;
    uint16_t last_hid_message_len;
    size_t last_device_message_size;
    uint8_t last_hid_message[1u + SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE];
    uint8_t last_device_message[1u + SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE];
    uint8_t hid_send_button_bytes[8];
    uint8_t controller_address[6];
    uint8_t ssp_confirmation_address[6];
    swbt_btstack_device_connect_request_t captured_active_reconnect_request;
    const swbt_daemon_ipc_runner_t *ipc_runner;
    swbt_ipc_status_t status_during_run_loop;
    int status_during_run_loop_result;
    swbt_btstack_hid_registration_config_t captured_hid_config;
    swbt_daemon_ipc_runner_config_t captured_ipc_config;
    swbt_btstack_input_report_timer_adapter_config_t captured_timer_config;
    swbt_daemon_shutdown_request_t shutdown_request;
    void *shutdown_request_context;
    swbt_ipc_status_t status_after_report;
    int status_after_report_result;
} fake_ops_t;

static void record_step(fake_ops_t *fake, int step) {
    if (fake->step_count < (sizeof(fake->steps) / sizeof(fake->steps[0]))) {
        fake->steps[fake->step_count] = step;
    }
    fake->step_count += 1u;
}

static int expect_true(bool value, const char *label) {
    if (!value) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "expected true: %s\n", label);
        return 1;
    }
    return 0;
}

static int expect_eq_int(int actual, int expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %d, got %d\n", label, expected, actual);
        return 1;
    }
    return 0;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %u, got %u\n", label, (unsigned)expected, (unsigned)actual);
        return 1;
    }
    return 0;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected, const char *label) {
    if (actual != expected) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %u, got %u\n", label, (unsigned)expected, (unsigned)actual);
        return 1;
    }
    return 0;
}

static int expect_str_eq(const char *actual, const char *expected, const char *label) {
    if (actual == NULL || expected == NULL || strcmp(actual, expected) != 0) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %s, got %s\n", label, expected == NULL ? "<null>" : expected,
                actual == NULL ? "<null>" : actual);
        return 1;
    }
    return 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static int expect_file_contains(const char *path, const char *needle, const char *label) {
    FILE *file = fopen(path, "rb");
    char buffer[8192];
    size_t read = 0;
    int result = 1;

    if (file == NULL) {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: trace file missing\n", label);
        return 1;
    }
    read = fread(buffer, 1, sizeof(buffer) - 1u, file);
    fclose(file);
    buffer[read] = '\0';
    if (strstr(buffer, needle) != NULL) {
        result = 0;
    } else {
        // Test diagnostics write to stderr with no retained buffer.
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: missing %s\n", label, needle);
    }
    return result;
}

static int fake_ipc_pump_start(void *context, const swbt_btstack_production_ipc_pump_t *pump) {
    fake_ops_t *fake = context;
    fake->ipc_start_calls += 1;
    fake->ipc_pump_running_at_start =
        pump != NULL && pump->is_running != NULL && pump->is_running(pump->context);
    fake->ipc_runner = pump == NULL ? NULL : (const swbt_daemon_ipc_runner_t *)pump->context;
    record_step(fake, STEP_IPC_START);
    return 0;
}

static void fake_ipc_pump_stop(void *context) {
    record_step((fake_ops_t *)context, STEP_IPC_STOP);
}

static int fake_platform_start(void *context) {
    fake_ops_t *fake = context;
    fake->platform_start_calls += 1;
    record_step(fake, STEP_PLATFORM_START);
    return 0;
}

static void fake_platform_stop(void *context) {
    record_step((fake_ops_t *)context, STEP_PLATFORM_STOP);
}

static int fake_hid_register(void *context, uint8_t *service_buffer, size_t service_buffer_size,
                             const swbt_btstack_hid_registration_config_t *config) {
    fake_ops_t *fake = context;
    (void)service_buffer;
    (void)service_buffer_size;
    fake->hid_register_calls += 1;
    fake->captured_hid_config = *config;
    record_step(fake, STEP_HID_REGISTER);
    return 0;
}

static void fake_hid_stop(void *context) {
    record_step((fake_ops_t *)context, STEP_HID_STOP);
}

static void fake_output_handler_start(void *context,
                                      swbt_btstack_output_report_handler_t *handler) {
    (void)handler;
    record_step((fake_ops_t *)context, STEP_OUTPUT_START);
}

static void fake_output_handler_stop(void *context) {
    record_step((fake_ops_t *)context, STEP_OUTPUT_STOP);
}

static int fake_timer_init(void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
                           const swbt_btstack_input_report_timer_adapter_config_t *config) {
    fake_ops_t *fake = context;
    (void)adapter;
    fake->captured_timer_config = *config;
    record_step(fake, STEP_TIMER_INIT);
    return fake->timer_init_result;
}

static int fake_timer_start(void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
                            swbt_btstack_input_report_timer_start_options_t options) {
    fake_ops_t *fake = context;
    fake->timer_start_calls += 1;
    fake->timer_hid_cid = options.hid_cid;
    if (adapter != NULL) {
        adapter->hid_cid = options.hid_cid;
        adapter->running = true;
    }
    return 0;
}

static void fake_record_hid_input_report(fake_ops_t *fake, uint16_t hid_cid, const uint8_t *report,
                                         size_t written) {
    if (fake == NULL || report == NULL || written + 1u > sizeof(fake->last_hid_message)) {
        return;
    }

    const int send_index = fake->hid_send_calls;
    fake->last_hid_cid = hid_cid;
    fake->last_hid_message[0] = 0xa1u;
    // The fake HID buffer capacity is checked before copying report bytes.
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memcpy(&fake->last_hid_message[1], report, written);
    fake->last_hid_message_len = (uint16_t)(written + 1u);
    if (send_index >= 0 && send_index < (int)(sizeof(fake->hid_send_button_bytes) /
                                              sizeof(fake->hid_send_button_bytes[0]))) {
        fake->hid_send_button_bytes[send_index] = report[3];
    }
    fake->hid_send_calls += 1;
}

static int fake_timer_can_send_now(void *context,
                                   swbt_btstack_input_report_timer_adapter_t *adapter) {
    fake_ops_t *fake = context;
    (void)adapter;
    fake->timer_can_send_calls += 1;
    record_step(fake, STEP_TIMER_CAN_SEND_NOW);
    if (fake->timer_can_send_result == 0 && fake->captured_timer_config.state_provider != NULL) {
        swbt_state_t state =
            fake->captured_timer_config.state_provider(fake->captured_timer_config.state_context);
        swbt_switch_report_options_t options =
            fake->captured_timer_config.scheduler_config.report_options;
        uint8_t report[SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE];
        size_t written = 0u;
        options.timer = 0x42u;
        if (swbt_switch_build_standard_full_report(&state, &options, report, sizeof(report),
                                                   &written) == SWBT_SWITCH_REPORT_OK) {
            fake_record_hid_input_report(fake, 0x0042u, report, written);
            if (fake->captured_timer_config.report_tick_observer != NULL) {
                fake->captured_timer_config.report_tick_observer(
                    fake->captured_timer_config.report_tick_context, 123000u,
                    SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_OK);
            }
        }
    } else if (fake->notify_report_tick_failure &&
               fake->captured_timer_config.report_tick_observer != NULL) {
        fake->captured_timer_config.report_tick_observer(
            fake->captured_timer_config.report_tick_context, 123000u,
            SWBT_BTSTACK_INPUT_REPORT_TIMER_REPORT_SEND_FAILED);
    }
    return fake->timer_can_send_result;
}

static int fake_timer_enqueue_reply(void *context,
                                    swbt_btstack_input_report_timer_adapter_t *adapter,
                                    uint16_t hid_cid, const uint8_t *report, size_t report_size) {
    (void)context;
    (void)adapter;
    (void)hid_cid;
    (void)report;
    (void)report_size;
    return 0;
}

static int fake_timer_send_neutral_now(void *context,
                                       swbt_btstack_input_report_timer_adapter_t *adapter) {
    fake_ops_t *fake = context;
    (void)adapter;
    fake->timer_send_neutral_now_calls += 1;
    record_step(fake, STEP_TIMER_SEND_NEUTRAL_NOW);
    if (fake->timer_send_neutral_now_result == 0) {
        swbt_switch_report_options_t options =
            fake->captured_timer_config.scheduler_config.report_options;
        const swbt_state_t state = swbt_state_neutral();
        uint8_t report[SWBT_SWITCH_STANDARD_FULL_REPORT_SIZE];
        size_t written = 0u;
        options.timer = 0x42u;
        if (swbt_switch_build_standard_full_report(&state, &options, report, sizeof(report),
                                                   &written) == SWBT_SWITCH_REPORT_OK) {
            fake_record_hid_input_report(fake, 0x0042u, report, written);
        }
    }
    return fake->timer_send_neutral_now_result;
}

static void fake_timer_stop(void *context, swbt_btstack_input_report_timer_adapter_t *adapter) {
    fake_ops_t *fake = context;
    fake->timer_stop_calls += 1;
    if (adapter != NULL) {
        adapter->running = false;
    }
    record_step(fake, STEP_TIMER_STOP);
}

static int fake_ssp_confirm_user_confirmation(void *context, const uint8_t address[6]) {
    fake_ops_t *fake = context;
    fake->ssp_confirmation_calls += 1;
    for (size_t index = 0; index < 6u; ++index) {
        fake->ssp_confirmation_address[index] = address[index];
    }
    return 0;
}

static int fake_read_controller_address(void *context, uint8_t address[6]) {
    fake_ops_t *fake = context;
    fake->read_controller_address_calls += 1;
    for (size_t index = 0; index < 6u; ++index) {
        address[index] = fake->controller_address[index];
    }
    return 0;
}

static uint32_t fake_time_ms(void *context) {
    (void)context;
    return 123u;
}

static int fake_power_on(void *context) {
    fake_ops_t *fake = context;
    record_step(fake, STEP_POWER_ON);
    return fake->power_on_result;
}

static void fake_power_off(void *context) {
    fake_ops_t *fake = context;
    fake->power_off_calls += 1;
    record_step(fake, STEP_POWER_OFF);
}

static void fake_handle_json_line(fake_ops_t *fake, uint32_t client_id, const char *line) {
    char response[SWBT_IPC_JSON_RESPONSE_MAX];

    if (fake == NULL || fake->ipc_runner == NULL || fake->ipc_runner->server.control == NULL ||
        swbt_ipc_adapter_handle_line(fake->ipc_runner->server.control, client_id, line, response,
                                     sizeof(response)) != SWBT_IPC_JSON_OK) {
        if (fake != NULL) {
            fake->injected_json_result = 1;
        }
    }
}

static void fake_handle_acquire(fake_ops_t *fake, uint32_t client_id, const char *request_id) {
    char line[128];

    // Test JSON formatting is bounded by the local line buffer.
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    if (snprintf(line, sizeof(line), "{\"v\":1,\"type\":\"acquire\",\"request_id\":\"%s\"}\n",
                 request_id) < 0) {
        fake->injected_json_result = 1;
        return;
    }
    fake_handle_json_line(fake, client_id, line);
}

static void fake_handle_button_a_state(fake_ops_t *fake, uint32_t client_id, const char *owner_id,
                                       const char *request_id) {
    char line[256];

    // Test JSON formatting is bounded by the local line buffer.
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    if (snprintf(line, sizeof(line),
                 "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"%s\",\"seq\":1,"
                 "\"request_id\":\"%s\",\"state\":{\"buttons\":8,\"lx\":2048,"
                 "\"ly\":2048,\"rx\":2048,\"ry\":2048,\"accel_x\":0,\"accel_y\":0,"
                 "\"accel_z\":0,\"gyro_x\":0,\"gyro_y\":0,\"gyro_z\":0}}\n",
                 owner_id, request_id) < 0) {
        fake->injected_json_result = 1;
        return;
    }
    fake_handle_json_line(fake, client_id, line);
}

static void fake_emit_hid_open_completed(fake_ops_t *fake) {
    uint8_t opened_event[] = {0xefu, 13u, 0x02u, 0x42u, 0x00u, 0x00u, 0u, 0u,
                              0u,    0u,  0u,    0u,    0u,    0u,    1u};
    fake->captured_hid_config.packet_handler(0x04u, 0x0042u, opened_event, sizeof(opened_event));
}

static void fake_emit_can_send(fake_ops_t *fake) {
    uint8_t can_send_event[] = {0xefu, 3u, 0x04u, 0x42u, 0x00u};
    fake->captured_hid_config.packet_handler(0x04u, 0x0042u, can_send_event,
                                             sizeof(can_send_event));
}

static void fake_emit_hid_closed(fake_ops_t *fake) {
    uint8_t closed_event[] = {0xefu, 3u, 0x03u, 0x42u, 0x00u};
    record_step(fake, STEP_HID_CONNECTION_CLOSED);
    fake->captured_hid_config.packet_handler(0x04u, 0x0042u, closed_event, sizeof(closed_event));
}

static void fake_fire_shutdown_disconnect_timeout(fake_ops_t *fake) {
    if (fake->shutdown_disconnect_timer_handler == NULL ||
        fake->shutdown_disconnect_timer == NULL) {
        return;
    }
    record_step(fake, STEP_SHUTDOWN_DISCONNECT_TIMEOUT);
    fake->shutdown_disconnect_timer_handler(fake->shutdown_disconnect_timer);
}

static void fake_fire_active_reconnect_timeout(fake_ops_t *fake) {
    if (fake->shutdown_disconnect_timer_handler == NULL ||
        fake->shutdown_disconnect_timer == NULL) {
        return;
    }
    record_step(fake, STEP_ACTIVE_RECONNECT_TIMEOUT);
    fake->shutdown_disconnect_timer_handler(fake->shutdown_disconnect_timer);
}

static void fake_run_loop_execute(void *context) {
    fake_ops_t *fake = context;
    record_step(fake, STEP_RUN_LOOP_EXECUTE);
    fake->status_during_run_loop_result =
        fake->ipc_runner == NULL || fake->ipc_runner->server.control == NULL
            ? SWBT_IPC_ERROR_INVALID_ARGUMENT
            : swbt_ipc_adapter_get_status(fake->ipc_runner->server.control,
                                          &fake->status_during_run_loop);
    if (fake->fire_active_reconnect_timeout_during_run_loop) {
        fake_fire_active_reconnect_timeout(fake);
        fake->status_after_report_result =
            fake->ipc_runner == NULL || fake->ipc_runner->server.control == NULL
                ? SWBT_IPC_ERROR_INVALID_ARGUMENT
                : swbt_ipc_adapter_get_status(fake->ipc_runner->server.control,
                                              &fake->status_after_report);
    }
    if (fake->fire_active_reconnect_timeout_after_hid_open_completed_and_closed) {
        fake_emit_hid_open_completed(fake);
        fake_emit_hid_closed(fake);
        fake_fire_active_reconnect_timeout(fake);
        fake->status_after_report_result =
            fake->ipc_runner == NULL || fake->ipc_runner->server.control == NULL
                ? SWBT_IPC_ERROR_INVALID_ARGUMENT
                : swbt_ipc_adapter_get_status(fake->ipc_runner->server.control,
                                              &fake->status_after_report);
    }
    if (fake->inject_json_state_during_run_loop && fake->ipc_runner != NULL &&
        fake->ipc_runner->server.control != NULL) {
        fake->injected_json_result = 0;
        fake_handle_acquire(fake, 1001u, "journey-acquire");
        fake_handle_button_a_state(fake, 1001u, "000003e9", "journey-state");
        fake_emit_hid_open_completed(fake);
        fake_emit_can_send(fake);
        fake->status_after_report_result = swbt_ipc_adapter_get_status(
            fake->ipc_runner->server.control, &fake->status_after_report);
    }
    if (fake->inject_disconnect_reacquire_during_run_loop && fake->ipc_runner != NULL &&
        fake->ipc_runner->server.control != NULL) {
        fake->injected_json_result = 0;
        fake_handle_acquire(fake, 1001u, "journey-acquire");
        fake_handle_button_a_state(fake, 1001u, "000003e9", "journey-state");
        fake_emit_hid_open_completed(fake);
        fake_emit_can_send(fake);

        if (swbt_ipc_adapter_handle_disconnect(fake->ipc_runner->server.control, 1001u) !=
            SWBT_IPC_OK) {
            fake->injected_json_result = 1;
        }
        fake_emit_can_send(fake);

        fake_handle_acquire(fake, 2002u, "journey-reacquire");
        fake_handle_button_a_state(fake, 2002u, "000007d2", "journey-reacquired-state");
        fake_emit_can_send(fake);
    }
    if (fake->shutdown_request != NULL) {
        if (!fake->skip_shutdown_opened_event) {
            fake_emit_hid_open_completed(fake);
        }

        const int request_count =
            fake->shutdown_requests_to_fire > 0 ? fake->shutdown_requests_to_fire : 1;
        for (int index = 0; index < request_count; ++index) {
            fake->shutdown_request(fake->shutdown_request_context);
        }
        if (fake->fire_can_send_after_shutdown_request) {
            uint8_t can_send_event[] = {0xefu, 3u, 0x04u, 0x42u, 0x00u};
            fake->captured_hid_config.packet_handler(0x04u, 0x0042u, can_send_event,
                                                     sizeof(can_send_event));
        }
        if (fake->assert_shutdown_waits_for_hid_closed &&
            (fake->power_off_calls != 0 || fake->run_loop_trigger_exit_calls != 0)) {
            fake->shutdown_wait_failed = 1;
        }
        if (fake->fire_closed_after_shutdown_request) {
            fake_emit_hid_closed(fake);
        }
        if (fake->fire_disconnect_timeout_after_shutdown_request) {
            fake_fire_shutdown_disconnect_timeout(fake);
        }
    }
}

static void fake_run_loop_execute_on_main_thread(
    void *context, btstack_context_callback_registration_t *callback_registration) {
    fake_ops_t *fake = context;
    record_step(fake, STEP_RUN_LOOP_EXECUTE_ON_MAIN_THREAD);
    if (callback_registration != NULL && callback_registration->callback != NULL) {
        callback_registration->callback(callback_registration->context);
    }
}

static void fake_run_loop_set_timer_handler(void *context, btstack_timer_source_t *timer,
                                            void (*process)(btstack_timer_source_t *timer)) {
    fake_ops_t *fake = context;
    fake->shutdown_disconnect_timer = timer;
    fake->shutdown_disconnect_timer_handler = process;
    if (timer != NULL) {
        timer->process = process;
    }
}

static void fake_run_loop_set_timer_context(void *context, btstack_timer_source_t *timer,
                                            void *timer_context) {
    fake_ops_t *fake = context;
    fake->shutdown_disconnect_timer = timer;
    if (timer != NULL) {
        timer->context = timer_context;
    }
}

static void fake_run_loop_set_timer(void *context, btstack_timer_source_t *timer,
                                    uint32_t timeout_ms) {
    fake_ops_t *fake = context;
    fake->shutdown_disconnect_timer = timer;
    fake->shutdown_disconnect_timer_timeout_ms = timeout_ms;
}

static void fake_run_loop_add_timer(void *context, btstack_timer_source_t *timer) {
    fake_ops_t *fake = context;
    fake->shutdown_disconnect_timer = timer;
    fake->shutdown_disconnect_timer_add_calls += 1;
}

static int fake_run_loop_remove_timer(void *context, btstack_timer_source_t *timer) {
    fake_ops_t *fake = context;
    fake->shutdown_disconnect_timer_remove_calls += 1;
    if (fake->shutdown_disconnect_timer == timer) {
        fake->shutdown_disconnect_timer = NULL;
        fake->shutdown_disconnect_timer_handler = NULL;
    }
    return 0;
}

static void fake_run_loop_trigger_exit(void *context) {
    fake_ops_t *fake = context;
    fake->run_loop_trigger_exit_calls += 1;
    record_step(fake, STEP_RUN_LOOP_TRIGGER_EXIT);
}

static int fake_shutdown_install(void *context, swbt_daemon_shutdown_request_t shutdown_request,
                                 void *shutdown_context) {
    fake_ops_t *fake = context;
    fake->shutdown_request = shutdown_request;
    fake->shutdown_request_context = shutdown_context;
    record_step(fake, STEP_SHUTDOWN_INSTALL);
    return 0;
}

static void fake_shutdown_uninstall(void *context) {
    record_step((fake_ops_t *)context, STEP_SHUTDOWN_UNINSTALL);
}

static int fake_active_reconnect_connect(void *context,
                                         const swbt_btstack_device_connect_request_t *request,
                                         uint16_t *out_hid_cid) {
    fake_ops_t *fake = context;
    fake->active_reconnect_connect_calls += 1;
    if (request != NULL) {
        fake->captured_active_reconnect_request = *request;
    }
    if (out_hid_cid != NULL) {
        *out_hid_cid = 0x0042u;
    }
    record_step(fake, STEP_ACTIVE_RECONNECT_CONNECT);
    return fake->active_reconnect_connect_result;
}

static int fake_device_disconnect(void *context, uint16_t hid_cid) {
    fake_ops_t *fake = context;
    fake->device_disconnect_calls += 1;
    fake->device_disconnect_hid_cid = hid_cid;
    record_step(fake, STEP_DEVICE_DISCONNECT);
    return fake->device_disconnect_result;
}

static swbt_btstack_production_ipc_pump_port_t fake_ipc_pump_port(void) {
    return (swbt_btstack_production_ipc_pump_port_t){
        .start = fake_ipc_pump_start,
        .stop = fake_ipc_pump_stop,
    };
}

static int fake_device_send(void *context, uint16_t hid_cid, const uint8_t *message,
                            size_t message_size) {
    fake_ops_t *fake = context;
    if (fake == NULL || message == NULL || message_size > sizeof(fake->last_device_message)) {
        return -1;
    }

    fake->device_send_calls += 1;
    fake->last_device_hid_cid = hid_cid;
    fake->last_device_message_size = message_size;
    // The fake HID buffer capacity is checked before copying message bytes.
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    memcpy(fake->last_device_message, message, message_size);
    return 0;
}

static swbt_btstack_device_port_t fake_device_port(void) {
    return (swbt_btstack_device_port_t){
        .platform_start = fake_platform_start,
        .platform_stop = fake_platform_stop,
        .hid_register = fake_hid_register,
        .hid_stop = fake_hid_stop,
        .connect = fake_active_reconnect_connect,
        .disconnect = fake_device_disconnect,
        .send = fake_device_send,
    };
}

static swbt_btstack_production_output_handler_port_t fake_output_handler_port(void) {
    return (swbt_btstack_production_output_handler_port_t){
        .start = fake_output_handler_start,
        .stop = fake_output_handler_stop,
    };
}

static swbt_btstack_production_report_timer_port_t fake_report_timer_port(void) {
    return (swbt_btstack_production_report_timer_port_t){
        .init = fake_timer_init,
        .start = fake_timer_start,
        .on_can_send_now = fake_timer_can_send_now,
        .enqueue_subcommand_reply = fake_timer_enqueue_reply,
        .send_neutral_now = fake_timer_send_neutral_now,
        .stop = fake_timer_stop,
    };
}

static swbt_btstack_production_controller_port_t fake_controller_port(void) {
    return (swbt_btstack_production_controller_port_t){
        .confirm_ssp_user_confirmation = fake_ssp_confirm_user_confirmation,
        .read_controller_address = fake_read_controller_address,
    };
}

static swbt_btstack_production_clock_port_t fake_clock_port(void) {
    return (swbt_btstack_production_clock_port_t){
        .time_ms = fake_time_ms,
    };
}

static swbt_btstack_production_power_port_t fake_power_port(void) {
    return (swbt_btstack_production_power_port_t){
        .on = fake_power_on,
        .off = fake_power_off,
    };
}

static swbt_btstack_production_run_loop_port_t fake_run_loop_port(void) {
    return (swbt_btstack_production_run_loop_port_t){
        .execute = fake_run_loop_execute,
        .execute_on_main_thread = fake_run_loop_execute_on_main_thread,
        .set_timer_handler = fake_run_loop_set_timer_handler,
        .set_timer_context = fake_run_loop_set_timer_context,
        .set_timer = fake_run_loop_set_timer,
        .add_timer = fake_run_loop_add_timer,
        .remove_timer = fake_run_loop_remove_timer,
        .trigger_exit = fake_run_loop_trigger_exit,
    };
}

static swbt_btstack_production_ports_t fake_ipc_pump_only_ports(void) {
    return (swbt_btstack_production_ports_t){
        .ipc_pump = fake_ipc_pump_port(),
    };
}

static swbt_btstack_production_ports_t fake_backend_ports(void) {
    return (swbt_btstack_production_ports_t){
        .ipc_pump = fake_ipc_pump_port(),
        .device = fake_device_port(),
        .output_handler = fake_output_handler_port(),
        .report_timer = fake_report_timer_port(),
        .controller = fake_controller_port(),
        .clock = fake_clock_port(),
        .power = fake_power_port(),
        .run_loop = fake_run_loop_port(),
    };
}

static int mark_adapter_location_configured(swbt_daemon_production_runner_t *backend) {
    return expect_true(swbt_daemon_production_runner_set_adapter_location_configured(backend),
                       "adapter location configured");
}

static int expect_steps(const fake_ops_t *fake, const int *expected, size_t expected_count) {
    int failed = 0;
    failed += expect_eq_int((int)fake->step_count, (int)expected_count, "step count");
    for (size_t index = 0; index < expected_count && index < fake->step_count; ++index) {
        failed += expect_eq_int(fake->steps[index], expected[index], "step");
    }
    return failed;
}

static int production_runner_requires_adapter_location_before_platform_start(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_ERROR_ADAPTER_LOCATION_REQUIRED, "main result");
    failed += expect_eq_int((int)fake.step_count, 0, "no adapter steps");
    failed += expect_eq_int(fake.platform_start_calls, 0, "platform not started");
    failed += expect_eq_int(fake.power_off_calls, 0, "power not touched");
    return failed;
}

static int production_runner_starts_without_code_level_hardware_approval_gate(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,  STEP_PLATFORM_START, STEP_HID_REGISTER,     STEP_OUTPUT_START,
        STEP_TIMER_INIT, STEP_POWER_ON,       STEP_RUN_LOOP_EXECUTE, STEP_POWER_OFF,
        STEP_TIMER_STOP, STEP_OUTPUT_STOP,    STEP_HID_STOP,         STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    return failed;
}

static int production_run_wrapper_starts_with_public_config(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    const swbt_daemon_production_run_config_t run_config = {
        .config = &config,
        .ports = &adapter,
        .ports_context = &fake,
        .adapter_location_configured = true,
    };
    const int expected[] = {
        STEP_IPC_START,  STEP_PLATFORM_START, STEP_HID_REGISTER,     STEP_OUTPUT_START,
        STEP_TIMER_INIT, STEP_POWER_ON,       STEP_RUN_LOOP_EXECUTE, STEP_POWER_OFF,
        STEP_TIMER_STOP, STEP_OUTPUT_STOP,    STEP_HID_STOP,         STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_run(&run_config), SWBT_DAEMON_PRODUCTION_OK,
                            "run result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    return failed;
}

static int ipc_pump_port_starts_without_unrelated_btstack_abilities(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t adapter = fake_ipc_pump_only_ports();
    swbt_daemon_production_runner_t backend;
    swbt_domain_t *app = swbt_domain_create();
    swbt_control_t control;
    const swbt_daemon_process_backend_t *host_backend = swbt_daemon_btstack_process_backend();
    const swbt_daemon_production_result_t init_result =
        swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake);

    int failed = 0;
    failed += expect_eq_int(init_result, SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += expect_true(app != NULL, "app created");
    failed += expect_eq_int(swbt_control_init(&control,
                                              &(swbt_control_config_t){
                                                  .app = app,
                                              }),
                            SWBT_CONTROL_OK, "control init");
    failed += expect_true(host_backend != NULL, "host backend");
    if (init_result == SWBT_DAEMON_PRODUCTION_OK && app != NULL && host_backend != NULL) {
        failed += expect_eq_int(host_backend->ipc_start(&backend, &control), 0, "ipc start");
        failed += expect_eq_int(fake.ipc_start_calls, 1, "ipc start calls");
        failed += expect_eq_int(fake.platform_start_calls, 0, "platform not started");
        failed += expect_eq_int(fake.hid_register_calls, 0, "hid not registered");
        host_backend->ipc_stop(&backend);
    }
    swbt_domain_destroy(app);
    return failed;
}

static int approved_backend_starts_hardware_and_cleans_up_in_order(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,  STEP_PLATFORM_START, STEP_HID_REGISTER,     STEP_OUTPUT_START,
        STEP_TIMER_INIT, STEP_POWER_ON,       STEP_RUN_LOOP_EXECUTE, STEP_POWER_OFF,
        STEP_TIMER_STOP, STEP_OUTPUT_STOP,    STEP_HID_STOP,         STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.ipc_pump_running_at_start, 1, "ipc pump running at start");
    failed +=
        expect_true(fake.captured_hid_config.hid_descriptor == swbt_switch_hid_descriptor_data(),
                    "hid descriptor");
    failed += expect_eq_int((int)fake.captured_hid_config.hid_descriptor_size,
                            (int)swbt_switch_hid_descriptor_size(), "hid descriptor size");
    return failed;
}

static int approved_backend_requests_active_reconnect_when_switch_address_is_configured(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;
    const uint8_t expected_address[] = {0x01u, 0x23u, 0x45u, 0x67u, 0x89u, 0xabu};
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_ACTIVE_RECONNECT_CONNECT,
        STEP_RUN_LOOP_EXECUTE,
        STEP_POWER_OFF,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_true(swbt_daemon_config_set_active_reconnect_learned_switch_address(
                              &config, "01:23:45:67:89:ab"),
                          "set learned address");
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.active_reconnect_connect_calls, 1, "active reconnect calls");
    failed += expect_eq_u16(fake.captured_active_reconnect_request.control_psm,
                            SWBT_BTSTACK_PRODUCTION_HID_CONTROL_PSM, "control psm");
    failed += expect_eq_u16(fake.captured_active_reconnect_request.interrupt_psm,
                            SWBT_BTSTACK_PRODUCTION_HID_INTERRUPT_PSM, "interrupt psm");
    for (size_t index = 0; index < sizeof(expected_address); ++index) {
        failed += expect_eq_u8(fake.captured_active_reconnect_request.address[index],
                               expected_address[index], "active reconnect address");
    }
    return failed;
}

static int active_reconnect_failure_reports_failed_state_without_stopping_run_loop(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .active_reconnect_connect_result = -7,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_ACTIVE_RECONNECT_CONNECT,
        STEP_RUN_LOOP_EXECUTE,
        STEP_POWER_OFF,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_true(swbt_daemon_config_set_active_reconnect_learned_switch_address(
                              &config, "01:23:45:67:89:ab"),
                          "set learned address");
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.status_during_run_loop_result, SWBT_IPC_OK, "status read");
    failed +=
        expect_eq_int((int)fake.status_during_run_loop.hardware.switch_connection_state,
                      (int)SWBT_IPC_HARDWARE_CHANNEL_FAILED, "switch connection failed state");
    failed += expect_eq_int((int)fake.status_during_run_loop.hardware.hid_channel_state,
                            (int)SWBT_IPC_HARDWARE_CHANNEL_FAILED, "hid failed state");
    return failed;
}

static int active_reconnect_timeout_reports_failed_state_without_stopping_run_loop(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .fire_active_reconnect_timeout_during_run_loop = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_ACTIVE_RECONNECT_CONNECT,
        STEP_RUN_LOOP_EXECUTE,
        STEP_ACTIVE_RECONNECT_TIMEOUT,
        STEP_POWER_OFF,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_true(swbt_daemon_config_set_active_reconnect_learned_switch_address(
                              &config, "01:23:45:67:89:ab"),
                          "set learned address");
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.status_after_report_result, SWBT_IPC_OK, "status read");
    failed +=
        expect_eq_int((int)fake.status_after_report.hardware.switch_connection_state,
                      (int)SWBT_IPC_HARDWARE_CHANNEL_FAILED, "switch connection failed state");
    failed += expect_eq_int((int)fake.status_after_report.hardware.hid_channel_state,
                            (int)SWBT_IPC_HARDWARE_CHANNEL_FAILED, "hid failed state");
    return failed;
}

static int active_reconnect_timeout_is_canceled_after_hid_open_completed(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .fire_active_reconnect_timeout_after_hid_open_completed_and_closed = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_ACTIVE_RECONNECT_CONNECT,
        STEP_RUN_LOOP_EXECUTE,
        STEP_HID_CONNECTION_CLOSED,
        STEP_TIMER_STOP,
        STEP_POWER_OFF,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_true(swbt_daemon_config_set_active_reconnect_learned_switch_address(
                              &config, "01:23:45:67:89:ab"),
                          "set learned address");
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.status_after_report_result, SWBT_IPC_OK, "status read");
    failed += expect_eq_int((int)fake.status_after_report.hardware.switch_connection_state,
                            (int)SWBT_IPC_HARDWARE_CHANNEL_UNAVAILABLE, "switch connection state");
    failed += expect_eq_int((int)fake.status_after_report.hardware.hid_channel_state,
                            (int)SWBT_IPC_HARDWARE_CHANNEL_UNAVAILABLE, "hid state");
    return failed;
}

static int run_loop_json_state_reaches_fake_hid_send(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .inject_json_state_during_run_loop = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_eq_int(fake.injected_json_result, 0, "json commands");
    failed += expect_eq_int(fake.timer_can_send_calls, 1, "timer can send calls");
    failed += expect_eq_int(fake.hid_send_calls, 1, "hid send calls");
    failed += expect_eq_u16(fake.last_hid_cid, 0x0042u, "hid cid");
    failed += expect_eq_u8(fake.last_hid_message[0], 0xa1u, "hidp report header");
    failed += expect_eq_u8(fake.last_hid_message[4], 0x08u, "button byte");
    return failed;
}

static int production_report_success_updates_status_metrics_without_hardware_measurement(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .inject_json_state_during_run_loop = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_eq_int(fake.injected_json_result, 0, "json commands");
    failed += expect_eq_int(fake.status_after_report_result, SWBT_IPC_OK, "status after report");
    failed += expect_eq_int((int)fake.status_after_report.metrics.report_ticks, 1, "report ticks");
    failed +=
        expect_eq_int((int)fake.status_after_report.metrics.report_send_ok, 1, "report send ok");
    failed += expect_eq_int((int)fake.status_after_report.metrics.report_send_failed, 0,
                            "report send failed");
    failed += expect_eq_int((int)fake.status_after_report.metrics.hardware_status,
                            (int)SWBT_METRICS_HARDWARE_UNAVAILABLE, "hardware metrics");
    failed += expect_eq_int((int)fake.status_after_report.metrics.actual_report_rate_hz, 0,
                            "actual report rate");
    failed += expect_eq_int((int)fake.status_after_report.metrics.jitter_max_us, 0, "jitter");
    return failed;
}

static int production_report_failure_updates_send_failure_metrics_and_cleans_up(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .inject_json_state_during_run_loop = 1,
        .notify_report_tick_failure = 1,
        .timer_can_send_result = -7,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_eq_int(fake.injected_json_result, 0, "json commands");
    failed += expect_eq_int(fake.timer_can_send_calls, 1, "timer can send calls");
    failed += expect_eq_int(fake.hid_send_calls, 0, "hid send calls");
    failed += expect_eq_int(fake.status_after_report_result, SWBT_IPC_OK, "status after report");
    failed += expect_eq_int((int)fake.status_after_report.metrics.report_ticks, 1, "report ticks");
    failed +=
        expect_eq_int((int)fake.status_after_report.metrics.report_send_ok, 0, "report send ok");
    failed += expect_eq_int((int)fake.status_after_report.metrics.report_send_failed, 1,
                            "report send failed");
    failed += expect_eq_int(fake.power_off_calls, 1, "power off cleanup");
    failed += expect_eq_int(fake.timer_stop_calls, 1, "timer stop cleanup");
    return failed;
}

static int run_loop_disconnect_emits_neutral_before_reacquire(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .inject_disconnect_reacquire_during_run_loop = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_eq_int(fake.injected_json_result, 0, "json commands");
    failed += expect_eq_int(fake.hid_send_calls, 3, "hid send calls");
    failed += expect_eq_u8(fake.hid_send_button_bytes[0], 0x08u, "initial button byte");
    failed += expect_eq_u8(fake.hid_send_button_bytes[1], 0x00u, "disconnect neutral byte");
    failed += expect_eq_u8(fake.hid_send_button_bytes[2], 0x08u, "reacquired button byte");
    return failed;
}

static int approved_backend_status_exposes_production_without_hardware_metrics(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_eq_int(fake.status_during_run_loop_result, SWBT_IPC_OK, "status read");
    failed += expect_eq_int((int)fake.status_during_run_loop.daemon.backend,
                            (int)SWBT_IPC_DAEMON_BACKEND_PRODUCTION, "daemon backend");
    failed += expect_eq_int((int)fake.status_during_run_loop.daemon.lifecycle_state,
                            (int)SWBT_IPC_DAEMON_LIFECYCLE_RUNNING, "lifecycle state");
    failed += expect_eq_int((int)fake.status_during_run_loop.daemon.hardware_approval,
                            (int)SWBT_IPC_HARDWARE_APPROVAL_APPROVED, "hardware approval");
    failed += expect_eq_int((int)fake.status_during_run_loop.metrics.hardware_status,
                            (int)SWBT_METRICS_HARDWARE_UNAVAILABLE, "hardware metrics");
    return failed;
}

static int start_failure_cleans_started_resources_only(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .timer_init_result = -7,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,    STEP_PLATFORM_START, STEP_HID_REGISTER,
        STEP_OUTPUT_START, STEP_TIMER_INIT,     STEP_OUTPUT_STOP,
        STEP_HID_STOP,     STEP_PLATFORM_STOP,  STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(swbt_daemon_production_main_with_runner(&backend),
                            SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    return failed;
}

static int stop_request_sends_neutral_before_power_off_and_run_loop_exit(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .fire_closed_after_shutdown_request = 1,
        .assert_shutdown_waits_for_hid_closed = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_SHUTDOWN_INSTALL,
        STEP_RUN_LOOP_EXECUTE,
        STEP_RUN_LOOP_EXECUTE_ON_MAIN_THREAD,
        STEP_TIMER_SEND_NEUTRAL_NOW,
        STEP_DEVICE_DISCONNECT,
        STEP_HID_CONNECTION_CLOSED,
        STEP_TIMER_STOP,
        STEP_POWER_OFF,
        STEP_RUN_LOOP_TRIGGER_EXIT,
        STEP_SHUTDOWN_UNINSTALL,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(
        swbt_daemon_production_main_with_runner_and_shutdown(&backend, &shutdown, &fake),
        SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.timer_send_neutral_now_calls, 1, "neutral send calls");
    failed += expect_eq_int(fake.device_disconnect_calls, 1, "disconnect calls");
    failed += expect_eq_u16(fake.device_disconnect_hid_cid, 0x0042u, "disconnect hid cid");
    failed += expect_eq_int(fake.shutdown_wait_failed, 0, "waits for hid close");
    failed += expect_eq_int(fake.power_off_calls, 1, "power off calls");
    failed += expect_eq_int(fake.run_loop_trigger_exit_calls, 1, "trigger exit calls");
    return failed;
}

static int shutdown_after_json_state_sends_trailing_neutral_before_power_off(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .inject_json_state_during_run_loop = 1,
        .fire_closed_after_shutdown_request = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_SHUTDOWN_INSTALL,
        STEP_RUN_LOOP_EXECUTE,
        STEP_TIMER_CAN_SEND_NOW,
        STEP_RUN_LOOP_EXECUTE_ON_MAIN_THREAD,
        STEP_TIMER_SEND_NEUTRAL_NOW,
        STEP_DEVICE_DISCONNECT,
        STEP_HID_CONNECTION_CLOSED,
        STEP_TIMER_STOP,
        STEP_POWER_OFF,
        STEP_RUN_LOOP_TRIGGER_EXIT,
        STEP_SHUTDOWN_UNINSTALL,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(
        swbt_daemon_production_main_with_runner_and_shutdown(&backend, &shutdown, &fake),
        SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.injected_json_result, 0, "json commands");
    failed += expect_eq_int(fake.hid_send_calls, 2, "hid send calls");
    failed += expect_eq_u8(fake.hid_send_button_bytes[0], 0x08u, "state button byte");
    failed += expect_eq_u8(fake.hid_send_button_bytes[1], 0x00u, "shutdown neutral byte");
    failed += expect_eq_int(fake.device_disconnect_calls, 1, "disconnect calls");
    failed += expect_eq_u16(fake.device_disconnect_hid_cid, 0x0042u, "disconnect hid cid");
    return failed;
}

static int pending_stop_request_finishes_after_can_send_event(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .timer_send_neutral_now_result = 1,
        .fire_can_send_after_shutdown_request = 1,
        .fire_closed_after_shutdown_request = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_SHUTDOWN_INSTALL,
        STEP_RUN_LOOP_EXECUTE,
        STEP_RUN_LOOP_EXECUTE_ON_MAIN_THREAD,
        STEP_TIMER_SEND_NEUTRAL_NOW,
        STEP_TIMER_CAN_SEND_NOW,
        STEP_DEVICE_DISCONNECT,
        STEP_HID_CONNECTION_CLOSED,
        STEP_TIMER_STOP,
        STEP_POWER_OFF,
        STEP_RUN_LOOP_TRIGGER_EXIT,
        STEP_SHUTDOWN_UNINSTALL,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(
        swbt_daemon_production_main_with_runner_and_shutdown(&backend, &shutdown, &fake),
        SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.timer_send_neutral_now_calls, 1, "neutral send calls");
    failed += expect_eq_int(fake.timer_can_send_calls, 1, "can send calls");
    failed += expect_eq_int(fake.device_disconnect_calls, 1, "disconnect calls");
    failed += expect_eq_u16(fake.device_disconnect_hid_cid, 0x0042u, "disconnect hid cid");
    failed += expect_eq_int(fake.power_off_calls, 1, "power off calls");
    failed += expect_eq_int(fake.run_loop_trigger_exit_calls, 1, "trigger exit calls");
    return failed;
}

static int pending_stop_request_finishes_after_failed_can_send_event(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .timer_can_send_result = -1,
        .timer_send_neutral_now_result = 1,
        .fire_can_send_after_shutdown_request = 1,
        .fire_closed_after_shutdown_request = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_SHUTDOWN_INSTALL,
        STEP_RUN_LOOP_EXECUTE,
        STEP_RUN_LOOP_EXECUTE_ON_MAIN_THREAD,
        STEP_TIMER_SEND_NEUTRAL_NOW,
        STEP_TIMER_CAN_SEND_NOW,
        STEP_DEVICE_DISCONNECT,
        STEP_HID_CONNECTION_CLOSED,
        STEP_TIMER_STOP,
        STEP_POWER_OFF,
        STEP_RUN_LOOP_TRIGGER_EXIT,
        STEP_SHUTDOWN_UNINSTALL,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(
        swbt_daemon_production_main_with_runner_and_shutdown(&backend, &shutdown, &fake),
        SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.timer_send_neutral_now_calls, 1, "neutral send calls");
    failed += expect_eq_int(fake.timer_can_send_calls, 1, "can send calls");
    failed += expect_eq_int(fake.device_disconnect_calls, 1, "disconnect calls");
    failed += expect_eq_u16(fake.device_disconnect_hid_cid, 0x0042u, "disconnect hid cid");
    failed += expect_eq_int(fake.power_off_calls, 1, "power off calls");
    failed += expect_eq_int(fake.run_loop_trigger_exit_calls, 1, "trigger exit calls");
    return failed;
}

static int shutdown_listener_is_installed_without_code_level_hardware_approval_gate(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .fire_closed_after_shutdown_request = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_SHUTDOWN_INSTALL,
        STEP_RUN_LOOP_EXECUTE,
        STEP_RUN_LOOP_EXECUTE_ON_MAIN_THREAD,
        STEP_TIMER_SEND_NEUTRAL_NOW,
        STEP_DEVICE_DISCONNECT,
        STEP_HID_CONNECTION_CLOSED,
        STEP_TIMER_STOP,
        STEP_POWER_OFF,
        STEP_RUN_LOOP_TRIGGER_EXIT,
        STEP_SHUTDOWN_UNINSTALL,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(
        swbt_daemon_production_main_with_runner_and_shutdown(&backend, &shutdown, &fake),
        SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.device_disconnect_calls, 1, "disconnect calls");
    failed += expect_eq_u16(fake.device_disconnect_hid_cid, 0x0042u, "disconnect hid cid");
    failed += expect_eq_int(fake.power_off_calls, 1, "power off calls");
    failed += expect_eq_int(fake.run_loop_trigger_exit_calls, 1, "trigger exit calls");
    return failed;
}

static int repeated_stop_request_does_not_power_off_twice(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .shutdown_requests_to_fire = 2,
        .fire_closed_after_shutdown_request = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_SHUTDOWN_INSTALL,
        STEP_RUN_LOOP_EXECUTE,
        STEP_RUN_LOOP_EXECUTE_ON_MAIN_THREAD,
        STEP_TIMER_SEND_NEUTRAL_NOW,
        STEP_DEVICE_DISCONNECT,
        STEP_HID_CONNECTION_CLOSED,
        STEP_TIMER_STOP,
        STEP_POWER_OFF,
        STEP_RUN_LOOP_TRIGGER_EXIT,
        STEP_SHUTDOWN_UNINSTALL,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(
        swbt_daemon_production_main_with_runner_and_shutdown(&backend, &shutdown, &fake),
        SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.timer_send_neutral_now_calls, 1, "neutral send calls");
    failed += expect_eq_int(fake.device_disconnect_calls, 1, "disconnect calls");
    failed += expect_eq_u16(fake.device_disconnect_hid_cid, 0x0042u, "disconnect hid cid");
    failed += expect_eq_int(fake.power_off_calls, 1, "power off calls");
    failed += expect_eq_int(fake.run_loop_trigger_exit_calls, 1, "trigger exit calls");
    return failed;
}

static int shutdown_disconnect_timeout_still_powers_off_and_exits(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .fire_disconnect_timeout_after_shutdown_request = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_SHUTDOWN_INSTALL,
        STEP_RUN_LOOP_EXECUTE,
        STEP_RUN_LOOP_EXECUTE_ON_MAIN_THREAD,
        STEP_TIMER_SEND_NEUTRAL_NOW,
        STEP_DEVICE_DISCONNECT,
        STEP_SHUTDOWN_DISCONNECT_TIMEOUT,
        STEP_TIMER_STOP,
        STEP_POWER_OFF,
        STEP_RUN_LOOP_TRIGGER_EXIT,
        STEP_SHUTDOWN_UNINSTALL,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(
        swbt_daemon_production_main_with_runner_and_shutdown(&backend, &shutdown, &fake),
        SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.device_disconnect_calls, 1, "disconnect calls");
    failed += expect_eq_int(fake.shutdown_disconnect_timer_add_calls, 1, "timer add calls");
    failed += expect_eq_int((int)fake.shutdown_disconnect_timer_timeout_ms, 250,
                            "shutdown disconnect timeout ms");
    failed += expect_eq_int(fake.power_off_calls, 1, "power off calls");
    failed += expect_eq_int(fake.run_loop_trigger_exit_calls, 1, "trigger exit calls");
    return failed;
}

static int shutdown_disconnect_failure_still_powers_off_and_exits(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .device_disconnect_result = -1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_SHUTDOWN_INSTALL,
        STEP_RUN_LOOP_EXECUTE,
        STEP_RUN_LOOP_EXECUTE_ON_MAIN_THREAD,
        STEP_TIMER_SEND_NEUTRAL_NOW,
        STEP_DEVICE_DISCONNECT,
        STEP_TIMER_STOP,
        STEP_POWER_OFF,
        STEP_RUN_LOOP_TRIGGER_EXIT,
        STEP_SHUTDOWN_UNINSTALL,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(
        swbt_daemon_production_main_with_runner_and_shutdown(&backend, &shutdown, &fake),
        SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.device_disconnect_calls, 1, "disconnect calls");
    failed += expect_eq_int(fake.shutdown_disconnect_timer_add_calls, 0, "timer add calls");
    failed += expect_eq_int(fake.power_off_calls, 1, "power off calls");
    failed += expect_eq_int(fake.run_loop_trigger_exit_calls, 1, "trigger exit calls");
    return failed;
}

static int shutdown_without_active_hid_connection_skips_disconnect_and_exits(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .skip_shutdown_opened_event = 1,
    };
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_runner_t backend;
    const int expected[] = {
        STEP_IPC_START,
        STEP_PLATFORM_START,
        STEP_HID_REGISTER,
        STEP_OUTPUT_START,
        STEP_TIMER_INIT,
        STEP_POWER_ON,
        STEP_SHUTDOWN_INSTALL,
        STEP_RUN_LOOP_EXECUTE,
        STEP_RUN_LOOP_EXECUTE_ON_MAIN_THREAD,
        STEP_POWER_OFF,
        STEP_RUN_LOOP_TRIGGER_EXIT,
        STEP_SHUTDOWN_UNINSTALL,
        STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,
        STEP_HID_STOP,
        STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(
        swbt_daemon_production_main_with_runner_and_shutdown(&backend, &shutdown, &fake),
        SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.device_disconnect_calls, 0, "disconnect calls");
    failed += expect_eq_int(fake.shutdown_disconnect_timer_add_calls, 0, "timer add calls");
    failed += expect_eq_int(fake.power_off_calls, 1, "power off calls");
    failed += expect_eq_int(fake.run_loop_trigger_exit_calls, 1, "trigger exit calls");
    return failed;
}

static int run_shutdown_trace_scenario(const char *path, fake_ops_t *fake) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_runner_t backend;

    int failed = 0;
    (void)remove(path);
    swbt_diagnostic_trace_set_path(path);
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, fake),
                            SWBT_DAEMON_PRODUCTION_OK, "trace init");
    failed += mark_adapter_location_configured(&backend);
    failed += expect_eq_int(
        swbt_daemon_production_main_with_runner_and_shutdown(&backend, &shutdown, fake),
        SWBT_DAEMON_PRODUCTION_OK, "trace main result");
    swbt_diagnostic_trace_set_path(NULL);
    return failed;
}

static int shutdown_disconnect_trace_distinguishes_terminal_states(void) {
    const char *closed_path = "daemon-production-shutdown-disconnect-closed-trace.log";
    const char *timeout_path = "daemon-production-shutdown-disconnect-timeout-trace.log";
    const char *unavailable_path = "daemon-production-shutdown-disconnect-unavailable-trace.log";
    fake_ops_t closed = {
        .fire_closed_after_shutdown_request = 1,
    };
    fake_ops_t timeout = {
        .fire_disconnect_timeout_after_shutdown_request = 1,
    };
    fake_ops_t unavailable = {
        .device_disconnect_result = -1,
    };

    int failed = 0;
    failed += run_shutdown_trace_scenario(closed_path, &closed);
    failed += expect_file_contains(closed_path, "production: shutdown hid disconnect requested",
                                   "requested trace");
    failed += expect_file_contains(closed_path, "production: shutdown hid disconnect closed",
                                   "closed trace");
    failed += run_shutdown_trace_scenario(timeout_path, &timeout);
    failed += expect_file_contains(timeout_path, "production: shutdown hid disconnect timeout",
                                   "timeout trace");
    failed += run_shutdown_trace_scenario(unavailable_path, &unavailable);
    failed += expect_file_contains(
        unavailable_path, "production: shutdown hid disconnect unavailable", "unavailable trace");

    (void)remove(closed_path);
    (void)remove(timeout_path);
    (void)remove(unavailable_path);
    return failed;
}

static int hid_packet_handler_starts_sends_and_stops_timer(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;
    swbt_daemon_process_t host;
    uint8_t opened_event[] = {0xefu, 13u, 0x02u, 0x42u, 0x00u, 0x00u, 0u, 0u,
                              0u,    0u,  0u,    0u,    0u,    0u,    1u};
    uint8_t can_send_event[] = {0xefu, 3u, 0x04u, 0x42u, 0x00u};
    uint8_t closed_event[] = {0xefu, 3u, 0x03u, 0x42u, 0x00u};

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += expect_eq_int(
        swbt_daemon_process_init(&host, &config, swbt_daemon_btstack_process_backend(), &backend),
        SWBT_DAEMON_PROCESS_OK, "host init");
    failed += expect_eq_int(swbt_daemon_process_start(&host), SWBT_DAEMON_PROCESS_OK, "host start");
    fake.captured_hid_config.packet_handler(0x04u, 0x0042u, opened_event, sizeof(opened_event));
    fake.captured_hid_config.packet_handler(0x04u, 0x0042u, can_send_event, sizeof(can_send_event));
    fake.captured_hid_config.packet_handler(0x04u, 0x0042u, closed_event, sizeof(closed_event));

    failed += expect_eq_int(fake.timer_start_calls, 1, "timer start");
    failed += expect_eq_u16(fake.timer_hid_cid, 0x0042u, "timer hid cid");
    failed += expect_eq_int(fake.timer_can_send_calls, 1, "timer can send");
    failed += expect_eq_int(fake.timer_stop_calls, 1, "timer stop");

    swbt_daemon_process_destroy(&host);
    return failed;
}

static int btstack_report_timer_bridge_sender_uses_device_send(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;
    swbt_daemon_process_t host;
    const uint8_t message[] = {0xa1u, 0x30u, 0x01u, 0x02u};

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += expect_eq_int(
        swbt_daemon_process_init(&host, &config, swbt_daemon_btstack_process_backend(), &backend),
        SWBT_DAEMON_PROCESS_OK, "host init");
    failed += expect_eq_int(swbt_daemon_process_start(&host), SWBT_DAEMON_PROCESS_OK, "host start");

    failed +=
        expect_true(fake.captured_timer_config.hid_sender != NULL, "timer hid sender configured");
    if (fake.captured_timer_config.hid_sender != NULL) {
        failed += expect_eq_int(
            fake.captured_timer_config.hid_sender(fake.captured_timer_config.hid_sender_context,
                                                  0x0042u, message, sizeof(message)),
            0, "timer hid sender result");
    }
    failed += expect_eq_int(fake.device_send_calls, 1, "device send calls");
    failed += expect_eq_u16(fake.last_device_hid_cid, 0x0042u, "device send cid");
    failed += expect_eq_int((int)fake.last_device_message_size, (int)sizeof(message),
                            "device send message size");
    for (size_t index = 0; index < sizeof(message); ++index) {
        failed += expect_eq_u8(fake.last_device_message[index], message[index],
                               "device send message byte");
    }

    swbt_daemon_process_destroy(&host);
    return failed;
}

static int hid_connection_opened_persists_learned_switch_address_to_config_target(void) {
    const char *path = "daemon-production-learned-switch-address.toml";
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;
    swbt_daemon_process_t host;
    const swbt_daemon_config_file_target_t target = {
        .path = path,
    };
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };
    uint8_t opened_event[] = {0xefu, 13u,   0x02u, 0x42u, 0x00u, 0x00u, 0xabu, 0x89u,
                              0x67u, 0x45u, 0x23u, 0x01u, 0u,    0u,    1u};

    (void)remove(path);

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += expect_true(
        swbt_daemon_production_runner_set_learned_switch_address_target(&backend, &target),
        "set learned address target");
    failed += expect_eq_int(
        swbt_daemon_process_init(&host, &config, swbt_daemon_btstack_process_backend(), &backend),
        SWBT_DAEMON_PROCESS_OK, "host init");
    failed += expect_eq_int(swbt_daemon_process_start(&host), SWBT_DAEMON_PROCESS_OK, "host start");
    fake.captured_hid_config.packet_handler(0x04u, 0x0042u, opened_event, sizeof(opened_event));

    swbt_daemon_config_t reloaded = swbt_daemon_config_default();
    failed += expect_eq_int(swbt_daemon_config_apply_file(&reloaded, &source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "reload file");
    failed += expect_str_eq(reloaded.active_reconnect_learned_switch_address, "01:23:45:67:89:AB",
                            "learned address");
    failed += expect_eq_int(fake.timer_start_calls, 1, "timer start");

    swbt_daemon_process_destroy(&host);
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int hid_packet_handler_confirms_ssp_user_confirmation(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_btstack_production_ports_t adapter = fake_backend_ports();
    swbt_daemon_production_runner_t backend;
    swbt_daemon_process_t host;
    uint8_t user_confirmation_event[] = {0x33u, 0x0au, 0x21u, 0xb5u, 0xf7u, 0x05u,
                                         0x48u, 0xc8u, 0xc4u, 0xdcu, 0x09u, 0x00u};
    const uint8_t expected_address[] = {0xc8u, 0x48u, 0x05u, 0xf7u, 0xb5u, 0x21u};

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_runner_init(&backend, &config, &adapter, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += expect_eq_int(
        swbt_daemon_process_init(&host, &config, swbt_daemon_btstack_process_backend(), &backend),
        SWBT_DAEMON_PROCESS_OK, "host init");
    failed += expect_eq_int(swbt_daemon_process_start(&host), SWBT_DAEMON_PROCESS_OK, "host start");
    fake.captured_hid_config.packet_handler(0x04u, 0x0000u, user_confirmation_event,
                                            sizeof(user_confirmation_event));

    failed += expect_eq_int(fake.ssp_confirmation_calls, 1, "ssp confirmation calls");
    for (size_t index = 0; index < sizeof(expected_address); ++index) {
        failed += expect_eq_u8(fake.ssp_confirmation_address[index], expected_address[index],
                               "ssp confirmation address");
    }

    swbt_daemon_process_destroy(&host);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += production_runner_requires_adapter_location_before_platform_start();
    failed += production_runner_starts_without_code_level_hardware_approval_gate();
    failed += production_run_wrapper_starts_with_public_config();
    failed += ipc_pump_port_starts_without_unrelated_btstack_abilities();
    failed += approved_backend_starts_hardware_and_cleans_up_in_order();
    failed += approved_backend_requests_active_reconnect_when_switch_address_is_configured();
    failed += active_reconnect_failure_reports_failed_state_without_stopping_run_loop();
    failed += active_reconnect_timeout_reports_failed_state_without_stopping_run_loop();
    failed += active_reconnect_timeout_is_canceled_after_hid_open_completed();
    failed += run_loop_json_state_reaches_fake_hid_send();
    failed += production_report_success_updates_status_metrics_without_hardware_measurement();
    failed += production_report_failure_updates_send_failure_metrics_and_cleans_up();
    failed += run_loop_disconnect_emits_neutral_before_reacquire();
    failed += approved_backend_status_exposes_production_without_hardware_metrics();
    failed += start_failure_cleans_started_resources_only();
    failed += stop_request_sends_neutral_before_power_off_and_run_loop_exit();
    failed += shutdown_after_json_state_sends_trailing_neutral_before_power_off();
    failed += pending_stop_request_finishes_after_can_send_event();
    failed += pending_stop_request_finishes_after_failed_can_send_event();
    failed += shutdown_listener_is_installed_without_code_level_hardware_approval_gate();
    failed += repeated_stop_request_does_not_power_off_twice();
    failed += shutdown_disconnect_timeout_still_powers_off_and_exits();
    failed += shutdown_disconnect_failure_still_powers_off_and_exits();
    failed += shutdown_without_active_hid_connection_skips_disconnect_and_exits();
    failed += shutdown_disconnect_trace_distinguishes_terminal_states();
    failed += hid_packet_handler_starts_sends_and_stops_timer();
    failed += btstack_report_timer_bridge_sender_uses_device_send();
    failed += hid_connection_opened_persists_learned_switch_address_to_config_target();
    failed += hid_packet_handler_confirms_ssp_user_confirmation();
    return failed == 0 ? 0 : 1;
}
