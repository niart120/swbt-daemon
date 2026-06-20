#include "btstack_bridge/production_btstack.h"

#include <stddef.h>

#include "btstack_bridge/hid_device_btstack_adapter.h"
#include "btstack_bridge/input_report_timer_adapter.h"
#include "btstack_bridge/output_report_callbacks.h"
#include "btstack_memory.h"
#include "btstack_run_loop.h"
#include "classic/hid_device.h"
#include "daemon/ipc_runner.h"
#include "hci.h"
#include "hci_transport_usb.h"
#include "l2cap.h"

#if defined(_WIN32)
#include "btstack_run_loop_windows.h"
#else
#include "btstack_run_loop_posix.h"
#endif

static int swbt_btstack_production_ipc_start(void *context, swbt_daemon_ipc_runner_t *runner,
                                             swbt_ipc_session_t *session,
                                             const swbt_daemon_ipc_runner_config_t *config) {
    (void)context;
    return swbt_daemon_ipc_runner_start(runner, session, config) == SWBT_DAEMON_IPC_RUNNER_OK ? 0
                                                                                              : -1;
}

static void swbt_btstack_production_ipc_stop(void *context, swbt_daemon_ipc_runner_t *runner) {
    (void)context;
    swbt_daemon_ipc_runner_stop(runner);
}

static int swbt_btstack_production_platform_start(void *context) {
    (void)context;
    btstack_memory_init();
#if defined(_WIN32)
    btstack_run_loop_init(btstack_run_loop_windows_get_instance());
#else
    btstack_run_loop_init(btstack_run_loop_posix_get_instance());
#endif
    hci_init(hci_transport_usb_instance(), NULL);
    l2cap_init();
    return 0;
}

static void swbt_btstack_production_platform_stop(void *context) {
    (void)context;
    hci_close();
    btstack_run_loop_deinit();
}

static int
swbt_btstack_production_hid_register(void *context, uint8_t *service_buffer,
                                     size_t service_buffer_size,
                                     const swbt_btstack_hid_registration_config_t *config) {
    (void)context;
    return swbt_btstack_hid_device_register(swbt_btstack_hid_registration_backend_btstack(), NULL,
                                            service_buffer, service_buffer_size,
                                            config) == SWBT_BTSTACK_HID_REGISTRATION_OK
               ? 0
               : -1;
}

static void swbt_btstack_production_hid_stop(void *context) {
    (void)context;
    hid_device_deinit();
}

static void
swbt_btstack_production_output_handler_start(void *context,
                                             swbt_btstack_output_report_handler_t *handler) {
    (void)context;
    (void)swbt_btstack_output_report_callbacks_register(handler);
}

static void swbt_btstack_production_output_handler_stop(void *context) {
    (void)context;
    swbt_btstack_output_report_callbacks_unregister();
}

static int swbt_btstack_production_report_timer_init(
    void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
    const swbt_btstack_input_report_timer_adapter_config_t *config) {
    swbt_btstack_input_report_timer_adapter_config_t btstack_config;
    (void)context;
    if (config == NULL) {
        return -1;
    }
    btstack_config = *config;
    btstack_config.backend = swbt_btstack_input_report_timer_backend_btstack();
    return swbt_btstack_input_report_timer_adapter_init(adapter, &btstack_config) ==
                   SWBT_BTSTACK_INPUT_REPORT_TIMER_OK
               ? 0
               : -1;
}

static int swbt_btstack_production_report_timer_start(
    void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
    swbt_btstack_input_report_timer_start_options_t options) {
    (void)context;
    return swbt_btstack_input_report_timer_adapter_start(adapter, options) ==
                   SWBT_BTSTACK_INPUT_REPORT_TIMER_OK
               ? 0
               : -1;
}

static int swbt_btstack_production_report_timer_on_can_send_now(
    void *context, swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    return swbt_btstack_input_report_timer_adapter_on_can_send_now(adapter) ==
                   SWBT_BTSTACK_INPUT_REPORT_TIMER_OK
               ? 0
               : -1;
}

static int swbt_btstack_production_report_timer_enqueue_reply(
    void *context, swbt_btstack_input_report_timer_adapter_t *adapter, uint16_t hid_cid,
    const uint8_t *report, size_t report_size) {
    (void)context;
    return swbt_btstack_input_report_timer_adapter_enqueue_subcommand_reply(
               adapter, hid_cid, report, report_size) == SWBT_BTSTACK_INPUT_REPORT_TIMER_OK
               ? 0
               : -1;
}

static void
swbt_btstack_production_report_timer_stop(void *context,
                                          swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    swbt_btstack_input_report_timer_adapter_stop(adapter);
}

static uint32_t swbt_btstack_production_time_ms(void *context) {
    (void)context;
    return btstack_run_loop_get_time_ms();
}

static int swbt_btstack_production_power_on(void *context) {
    (void)context;
    return hci_power_control(HCI_POWER_ON);
}

static void swbt_btstack_production_power_off(void *context) {
    (void)context;
    (void)hci_power_control(HCI_POWER_OFF);
}

static void swbt_btstack_production_run_loop_execute(void *context) {
    (void)context;
    btstack_run_loop_execute();
}

static void swbt_btstack_production_run_loop_trigger_exit(void *context) {
    (void)context;
    btstack_run_loop_trigger_exit();
}

const swbt_daemon_production_backend_ops_t *swbt_btstack_production_backend_ops(void) {
    static const swbt_daemon_production_backend_ops_t ops = {
        .ipc_start = swbt_btstack_production_ipc_start,
        .ipc_stop = swbt_btstack_production_ipc_stop,
        .platform_start = swbt_btstack_production_platform_start,
        .platform_stop = swbt_btstack_production_platform_stop,
        .hid_register = swbt_btstack_production_hid_register,
        .hid_stop = swbt_btstack_production_hid_stop,
        .output_handler_start = swbt_btstack_production_output_handler_start,
        .output_handler_stop = swbt_btstack_production_output_handler_stop,
        .report_timer_init = swbt_btstack_production_report_timer_init,
        .report_timer_start = swbt_btstack_production_report_timer_start,
        .report_timer_on_can_send_now = swbt_btstack_production_report_timer_on_can_send_now,
        .report_timer_enqueue_subcommand_reply = swbt_btstack_production_report_timer_enqueue_reply,
        .report_timer_stop = swbt_btstack_production_report_timer_stop,
        .time_ms = swbt_btstack_production_time_ms,
        .power_on = swbt_btstack_production_power_on,
        .power_off = swbt_btstack_production_power_off,
        .run_loop_execute = swbt_btstack_production_run_loop_execute,
        .run_loop_trigger_exit = swbt_btstack_production_run_loop_trigger_exit,
    };
    return &ops;
}
