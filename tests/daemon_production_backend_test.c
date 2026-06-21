#include "daemon/production_backend.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "switch/switch_hid_descriptor.h"

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
};

typedef struct {
    int steps[32];
    size_t step_count;
    int timer_init_result;
    int power_on_result;
    int ipc_start_calls;
    int platform_start_calls;
    int hid_register_calls;
    int timer_start_calls;
    int timer_can_send_calls;
    int timer_stop_calls;
    int power_off_calls;
    int read_controller_address_calls;
    int ssp_confirmation_calls;
    int run_loop_trigger_exit_calls;
    int shutdown_requests_to_fire;
    uint16_t timer_hid_cid;
    uint8_t controller_address[6];
    uint8_t ssp_confirmation_address[6];
    swbt_btstack_hid_registration_config_t captured_hid_config;
    swbt_daemon_ipc_runner_config_t captured_ipc_config;
    swbt_daemon_shutdown_request_t shutdown_request;
    void *shutdown_request_context;
} fake_ops_t;

static void record_step(fake_ops_t *fake, int step) {
    if (fake->step_count < (sizeof(fake->steps) / sizeof(fake->steps[0]))) {
        fake->steps[fake->step_count] = step;
    }
    fake->step_count += 1u;
}

static int expect_true(bool value, const char *label) {
    if (!value) {
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "expected true: %s\n", label);
        return 1;
    }
    return 0;
}

static int expect_eq_int(int actual, int expected, const char *label) {
    if (actual != expected) {
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %d, got %d\n", label, expected, actual);
        return 1;
    }
    return 0;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected, const char *label) {
    if (actual != expected) {
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %u, got %u\n", label, (unsigned)expected, (unsigned)actual);
        return 1;
    }
    return 0;
}

static int expect_eq_u8(uint8_t actual, uint8_t expected, const char *label) {
    if (actual != expected) {
        // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
        fprintf(stderr, "%s: expected %u, got %u\n", label, (unsigned)expected, (unsigned)actual);
        return 1;
    }
    return 0;
}

static int fake_ipc_start(void *context, swbt_daemon_ipc_runner_t *runner,
                          swbt_ipc_session_t *session,
                          const swbt_daemon_ipc_runner_config_t *config) {
    fake_ops_t *fake = context;
    (void)runner;
    (void)session;
    fake->ipc_start_calls += 1;
    fake->captured_ipc_config = *config;
    record_step(fake, STEP_IPC_START);
    return 0;
}

static void fake_ipc_stop(void *context, swbt_daemon_ipc_runner_t *runner) {
    (void)runner;
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
    (void)config;
    record_step(fake, STEP_TIMER_INIT);
    return fake->timer_init_result;
}

static int fake_timer_start(void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
                            swbt_btstack_input_report_timer_start_options_t options) {
    fake_ops_t *fake = context;
    (void)adapter;
    fake->timer_start_calls += 1;
    fake->timer_hid_cid = options.hid_cid;
    return 0;
}

static int fake_timer_can_send_now(void *context,
                                   swbt_btstack_input_report_timer_adapter_t *adapter) {
    fake_ops_t *fake = context;
    (void)adapter;
    fake->timer_can_send_calls += 1;
    return 0;
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

static void fake_timer_stop(void *context, swbt_btstack_input_report_timer_adapter_t *adapter) {
    fake_ops_t *fake = context;
    (void)adapter;
    fake->timer_stop_calls += 1;
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

static void fake_run_loop_execute(void *context) {
    fake_ops_t *fake = context;
    record_step(fake, STEP_RUN_LOOP_EXECUTE);
    if (fake->shutdown_request != NULL) {
        const int request_count =
            fake->shutdown_requests_to_fire > 0 ? fake->shutdown_requests_to_fire : 1;
        for (int index = 0; index < request_count; ++index) {
            fake->shutdown_request(fake->shutdown_request_context);
        }
    }
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

static swbt_daemon_production_backend_ops_t fake_backend_ops(void) {
    return (swbt_daemon_production_backend_ops_t){
        .ipc_start = fake_ipc_start,
        .ipc_stop = fake_ipc_stop,
        .platform_start = fake_platform_start,
        .platform_stop = fake_platform_stop,
        .hid_register = fake_hid_register,
        .hid_stop = fake_hid_stop,
        .output_handler_start = fake_output_handler_start,
        .output_handler_stop = fake_output_handler_stop,
        .report_timer_init = fake_timer_init,
        .report_timer_start = fake_timer_start,
        .report_timer_on_can_send_now = fake_timer_can_send_now,
        .report_timer_enqueue_subcommand_reply = fake_timer_enqueue_reply,
        .report_timer_stop = fake_timer_stop,
        .ssp_confirm_user_confirmation = fake_ssp_confirm_user_confirmation,
        .read_controller_address = fake_read_controller_address,
        .time_ms = fake_time_ms,
        .power_on = fake_power_on,
        .power_off = fake_power_off,
        .run_loop_execute = fake_run_loop_execute,
        .run_loop_trigger_exit = fake_run_loop_trigger_exit,
    };
}

static int expect_steps(const fake_ops_t *fake, const int *expected, size_t expected_count) {
    int failed = 0;
    failed += expect_eq_int((int)fake->step_count, (int)expected_count, "step count");
    for (size_t index = 0; index < expected_count && index < fake->step_count; ++index) {
        failed += expect_eq_int(fake->steps[index], expected[index], "step");
    }
    return failed;
}

static int missing_hardware_approval_rejects_before_backend_start(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_daemon_production_backend_ops_t ops = fake_backend_ops();
    swbt_daemon_production_backend_t backend;
    const swbt_daemon_hardware_approval_t approval = {
        .run_hardware = false,
        .hardware_approved = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_backend_init(&backend, &config, &ops, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed +=
        expect_eq_int(swbt_daemon_production_main_with_backend(&backend, &approval),
                      SWBT_DAEMON_PRODUCTION_ERROR_HARDWARE_APPROVAL_REQUIRED, "approval result");
    failed += expect_eq_int(fake.ipc_start_calls, 0, "ipc not started");
    failed += expect_eq_int(fake.platform_start_calls, 0, "platform not started");
    failed += expect_eq_int(fake.hid_register_calls, 0, "hid not registered");
    return failed;
}

static int approved_backend_starts_hardware_and_cleans_up_in_order(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_daemon_production_backend_ops_t ops = fake_backend_ops();
    swbt_daemon_production_backend_t backend;
    const swbt_daemon_hardware_approval_t approval = {
        .run_hardware = true,
        .hardware_approved = true,
    };
    const int expected[] = {
        STEP_IPC_START,  STEP_PLATFORM_START, STEP_HID_REGISTER,     STEP_OUTPUT_START,
        STEP_TIMER_INIT, STEP_POWER_ON,       STEP_RUN_LOOP_EXECUTE, STEP_POWER_OFF,
        STEP_TIMER_STOP, STEP_OUTPUT_STOP,    STEP_HID_STOP,         STEP_PLATFORM_STOP,
        STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_backend_init(&backend, &config, &ops, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += expect_eq_int(swbt_daemon_production_main_with_backend(&backend, &approval),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed +=
        expect_true(fake.captured_hid_config.hid_descriptor == swbt_switch_hid_descriptor_data(),
                    "hid descriptor");
    failed += expect_eq_int((int)fake.captured_hid_config.hid_descriptor_size,
                            (int)swbt_switch_hid_descriptor_size(), "hid descriptor size");
    return failed;
}

static int start_failure_cleans_started_resources_only(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .timer_init_result = -7,
    };
    const swbt_daemon_production_backend_ops_t ops = fake_backend_ops();
    swbt_daemon_production_backend_t backend;
    const swbt_daemon_hardware_approval_t approval = {
        .run_hardware = true,
        .hardware_approved = true,
    };
    const int expected[] = {
        STEP_IPC_START,    STEP_PLATFORM_START, STEP_HID_REGISTER,
        STEP_OUTPUT_START, STEP_TIMER_INIT,     STEP_OUTPUT_STOP,
        STEP_HID_STOP,     STEP_PLATFORM_STOP,  STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_backend_init(&backend, &config, &ops, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += expect_eq_int(swbt_daemon_production_main_with_backend(&backend, &approval),
                            SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    return failed;
}

static int stop_request_powers_off_and_triggers_run_loop_exit_before_cleanup(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_daemon_production_backend_ops_t ops = fake_backend_ops();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_backend_t backend;
    const swbt_daemon_hardware_approval_t approval = {
        .run_hardware = true,
        .hardware_approved = true,
    };
    const int expected[] = {
        STEP_IPC_START,          STEP_PLATFORM_START,
        STEP_HID_REGISTER,       STEP_OUTPUT_START,
        STEP_TIMER_INIT,         STEP_POWER_ON,
        STEP_SHUTDOWN_INSTALL,   STEP_RUN_LOOP_EXECUTE,
        STEP_POWER_OFF,          STEP_RUN_LOOP_TRIGGER_EXIT,
        STEP_SHUTDOWN_UNINSTALL, STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,        STEP_HID_STOP,
        STEP_PLATFORM_STOP,      STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_backend_init(&backend, &config, &ops, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += expect_eq_int(swbt_daemon_production_main_with_backend_and_shutdown(
                                &backend, &approval, &shutdown, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.power_off_calls, 1, "power off calls");
    failed += expect_eq_int(fake.run_loop_trigger_exit_calls, 1, "trigger exit calls");
    return failed;
}

static int shutdown_listener_is_not_installed_when_hardware_approval_is_missing(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_daemon_production_backend_ops_t ops = fake_backend_ops();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_backend_t backend;
    const swbt_daemon_hardware_approval_t approval = {
        .run_hardware = false,
        .hardware_approved = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_backend_init(&backend, &config, &ops, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed +=
        expect_eq_int(swbt_daemon_production_main_with_backend_and_shutdown(&backend, &approval,
                                                                            &shutdown, &fake),
                      SWBT_DAEMON_PRODUCTION_ERROR_HARDWARE_APPROVAL_REQUIRED, "approval result");
    failed += expect_eq_int((int)fake.step_count, 0, "no backend steps");
    failed += expect_eq_int(fake.power_off_calls, 0, "power off calls");
    failed += expect_eq_int(fake.run_loop_trigger_exit_calls, 0, "trigger exit calls");
    return failed;
}

static int repeated_stop_request_does_not_power_off_twice(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {
        .shutdown_requests_to_fire = 2,
    };
    const swbt_daemon_production_backend_ops_t ops = fake_backend_ops();
    const swbt_daemon_shutdown_listener_t shutdown = {
        .install = fake_shutdown_install,
        .uninstall = fake_shutdown_uninstall,
    };
    swbt_daemon_production_backend_t backend;
    const swbt_daemon_hardware_approval_t approval = {
        .run_hardware = true,
        .hardware_approved = true,
    };
    const int expected[] = {
        STEP_IPC_START,          STEP_PLATFORM_START,
        STEP_HID_REGISTER,       STEP_OUTPUT_START,
        STEP_TIMER_INIT,         STEP_POWER_ON,
        STEP_SHUTDOWN_INSTALL,   STEP_RUN_LOOP_EXECUTE,
        STEP_POWER_OFF,          STEP_RUN_LOOP_TRIGGER_EXIT,
        STEP_SHUTDOWN_UNINSTALL, STEP_TIMER_STOP,
        STEP_OUTPUT_STOP,        STEP_HID_STOP,
        STEP_PLATFORM_STOP,      STEP_IPC_STOP,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_backend_init(&backend, &config, &ops, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed += expect_eq_int(swbt_daemon_production_main_with_backend_and_shutdown(
                                &backend, &approval, &shutdown, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "main result");
    failed += expect_steps(&fake, expected, sizeof(expected) / sizeof(expected[0]));
    failed += expect_eq_int(fake.power_off_calls, 1, "power off calls");
    failed += expect_eq_int(fake.run_loop_trigger_exit_calls, 1, "trigger exit calls");
    return failed;
}

static int report_period_and_ipc_config_are_exposed(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_daemon_production_backend_ops_t ops = fake_backend_ops();
    swbt_daemon_production_backend_t backend;
    swbt_daemon_ipc_runner_config_t ipc_config;

    config.report_period_us = 8333u;
    config.ipc_host = "127.0.0.1";
    config.ipc_port = 34567u;
    config.ipc_backlog = 2;
    config.ipc_heartbeat_timeout_ms = 1234u;

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_backend_init(&backend, &config, &ops, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    ipc_config = swbt_daemon_production_backend_ipc_config(&backend);
    failed += expect_eq_int((int)swbt_daemon_production_backend_report_period_us(&backend), 8333,
                            "report period");
    failed += expect_true(ipc_config.host == config.ipc_host, "ipc host");
    failed += expect_eq_u16(ipc_config.port, 34567u, "ipc port");
    failed += expect_eq_int(ipc_config.backlog, 2, "ipc backlog");
    failed += expect_eq_int((int)ipc_config.heartbeat_timeout_ms, 1234,
                            "ipc heartbeat timeout");
    return failed;
}

static int hid_packet_handler_starts_sends_and_stops_timer(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_daemon_production_backend_ops_t ops = fake_backend_ops();
    swbt_daemon_production_backend_t backend;
    swbt_daemon_runtime_t runtime;
    uint8_t opened_event[] = {0xefu, 13u, 0x02u, 0x42u, 0x00u, 0x00u, 0u, 0u,
                              0u,    0u,  0u,    0u,    0u,    0u,    1u};
    uint8_t can_send_event[] = {0xefu, 3u, 0x04u, 0x42u, 0x00u};
    uint8_t closed_event[] = {0xefu, 3u, 0x03u, 0x42u, 0x00u};

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_backend_init(&backend, &config, &ops, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed +=
        expect_eq_int(swbt_daemon_runtime_init(&runtime, &config,
                                               swbt_daemon_production_runtime_backend(), &backend),
                      SWBT_DAEMON_RUNTIME_OK, "runtime init");
    failed +=
        expect_eq_int(swbt_daemon_runtime_start(&runtime), SWBT_DAEMON_RUNTIME_OK, "runtime start");
    fake.captured_hid_config.packet_handler(0x04u, 0x0042u, opened_event, sizeof(opened_event));
    fake.captured_hid_config.packet_handler(0x04u, 0x0042u, can_send_event, sizeof(can_send_event));
    fake.captured_hid_config.packet_handler(0x04u, 0x0042u, closed_event, sizeof(closed_event));

    failed += expect_eq_int(fake.timer_start_calls, 1, "timer start");
    failed += expect_eq_u16(fake.timer_hid_cid, 0x0042u, "timer hid cid");
    failed += expect_eq_int(fake.timer_can_send_calls, 1, "timer can send");
    failed += expect_eq_int(fake.timer_stop_calls, 1, "timer stop");

    swbt_daemon_runtime_stop(&runtime);
    return failed;
}

static int hid_packet_handler_confirms_ssp_user_confirmation(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    fake_ops_t fake = {0};
    const swbt_daemon_production_backend_ops_t ops = fake_backend_ops();
    swbt_daemon_production_backend_t backend;
    swbt_daemon_runtime_t runtime;
    uint8_t user_confirmation_event[] = {0x33u, 0x0au, 0x21u, 0xb5u, 0xf7u, 0x05u,
                                         0x48u, 0xc8u, 0xc4u, 0xdcu, 0x09u, 0x00u};
    const uint8_t expected_address[] = {0xc8u, 0x48u, 0x05u, 0xf7u, 0xb5u, 0x21u};

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_production_backend_init(&backend, &config, &ops, &fake),
                            SWBT_DAEMON_PRODUCTION_OK, "init");
    failed +=
        expect_eq_int(swbt_daemon_runtime_init(&runtime, &config,
                                               swbt_daemon_production_runtime_backend(), &backend),
                      SWBT_DAEMON_RUNTIME_OK, "runtime init");
    failed +=
        expect_eq_int(swbt_daemon_runtime_start(&runtime), SWBT_DAEMON_RUNTIME_OK, "runtime start");
    fake.captured_hid_config.packet_handler(0x04u, 0x0000u, user_confirmation_event,
                                            sizeof(user_confirmation_event));

    failed += expect_eq_int(fake.ssp_confirmation_calls, 1, "ssp confirmation calls");
    for (size_t index = 0; index < sizeof(expected_address); ++index) {
        failed += expect_eq_u8(fake.ssp_confirmation_address[index], expected_address[index],
                               "ssp confirmation address");
    }

    swbt_daemon_runtime_stop(&runtime);
    return failed;
}

int main(void) {
    int failed = 0;
    failed += missing_hardware_approval_rejects_before_backend_start();
    failed += approved_backend_starts_hardware_and_cleans_up_in_order();
    failed += start_failure_cleans_started_resources_only();
    failed += stop_request_powers_off_and_triggers_run_loop_exit_before_cleanup();
    failed += shutdown_listener_is_not_installed_when_hardware_approval_is_missing();
    failed += repeated_stop_request_does_not_power_off_twice();
    failed += report_period_and_ipc_config_are_exposed();
    failed += hid_packet_handler_starts_sends_and_stops_timer();
    failed += hid_packet_handler_confirms_ssp_user_confirmation();
    return failed == 0 ? 0 : 1;
}
