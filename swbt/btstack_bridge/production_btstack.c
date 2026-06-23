#include "btstack_bridge/production_btstack.h"

#include <stddef.h>

#include "bluetooth.h"
#include "btstack_bridge/classic_discovery.h"
#include "btstack_bridge/classic_discovery_btstack_adapter.h"
#include "btstack_bridge/hci_dump_text.h"
#include "btstack_bridge/hid_device_btstack_adapter.h"
#include "btstack_bridge/input_report_timer_adapter.h"
#include "btstack_bridge/output_report_callbacks.h"
#include "btstack_memory.h"
#include "btstack_run_loop.h"
#include "classic/hid_device.h"
#include "core/diagnostics.h"
#include "gap.h"
#include "hci_dump.h"
#include "hci.h"
#include "hci_transport_usb.h"
#include "l2cap.h"

#include <stdlib.h>

#if defined(_WIN32)
#include "btstack_run_loop_windows.h"
#else
#include "btstack_run_loop_posix.h"
#endif

#define SWBT_BTSTACK_IPC_PUMP_PERIOD_MS 1u

static bool g_swbt_btstack_production_hci_dump_open;
static swbt_btstack_production_ipc_pump_t g_swbt_btstack_production_ipc_pump;
static bool g_swbt_btstack_production_ipc_pump_started;
static btstack_timer_source_t g_swbt_btstack_production_ipc_pump_timer;
static bool g_swbt_btstack_production_ipc_pump_timer_pending;

static void swbt_btstack_production_ipc_pump_schedule(void);

static void swbt_btstack_production_ipc_pump_timer_handler(btstack_timer_source_t *timer) {
    swbt_btstack_production_ipc_pump_t *pump = (swbt_btstack_production_ipc_pump_t *)timer->context;

    g_swbt_btstack_production_ipc_pump_timer_pending = false;
    if (pump != NULL && pump->is_running != NULL && pump->poll_once_at != NULL &&
        pump->is_running(pump->context)) {
        pump->poll_once_at(pump->context, btstack_run_loop_get_time_ms());
    }
    swbt_btstack_production_ipc_pump_schedule();
}

static void swbt_btstack_production_ipc_pump_schedule(void) {
    if (!g_swbt_btstack_production_ipc_pump_started ||
        g_swbt_btstack_production_ipc_pump_timer_pending) {
        return;
    }

    btstack_run_loop_set_timer(&g_swbt_btstack_production_ipc_pump_timer,
                               SWBT_BTSTACK_IPC_PUMP_PERIOD_MS);
    btstack_run_loop_add_timer(&g_swbt_btstack_production_ipc_pump_timer);
    g_swbt_btstack_production_ipc_pump_timer_pending = true;
}

static int swbt_btstack_production_ipc_pump_start(void *context,
                                                  const swbt_btstack_production_ipc_pump_t *pump) {
    (void)context;
    if (pump == NULL || pump->is_running == NULL || pump->poll_once_at == NULL) {
        return -1;
    }
    swbt_diagnostic_trace("btstack: ipc pump start");
    g_swbt_btstack_production_ipc_pump = *pump;
    g_swbt_btstack_production_ipc_pump_started = true;
    g_swbt_btstack_production_ipc_pump_timer_pending = false;
    btstack_run_loop_set_timer_handler(&g_swbt_btstack_production_ipc_pump_timer,
                                       swbt_btstack_production_ipc_pump_timer_handler);
    btstack_run_loop_set_timer_context(&g_swbt_btstack_production_ipc_pump_timer,
                                       &g_swbt_btstack_production_ipc_pump);
    swbt_diagnostic_trace("btstack: ipc pump start ok");
    return 0;
}

static void swbt_btstack_production_ipc_pump_stop(void *context) {
    (void)context;
    if (g_swbt_btstack_production_ipc_pump_timer_pending) {
        (void)btstack_run_loop_remove_timer(&g_swbt_btstack_production_ipc_pump_timer);
        g_swbt_btstack_production_ipc_pump_timer_pending = false;
    }
    g_swbt_btstack_production_ipc_pump_started = false;
    g_swbt_btstack_production_ipc_pump = (swbt_btstack_production_ipc_pump_t){0};
}

static swbt_btstack_classic_discovery_config_t swbt_btstack_production_discovery_config(void) {
    const swbt_btstack_hid_registration_config_t hid_config =
        swbt_btstack_production_hid_registration_config();
    return (swbt_btstack_classic_discovery_config_t){
        .class_of_device = hid_config.hid_device_subclass,
        .local_name = hid_config.device_name,
        .link_policy_settings =
            LM_LINK_POLICY_ENABLE_ROLE_SWITCH | LM_LINK_POLICY_ENABLE_SNIFF_MODE,
        .allow_role_switch = true,
        .discoverable = true,
    };
}

int swbt_btstack_production_hci_dump_start(const char *path) {
    swbt_btstack_hci_dump_text_result_t result;

    if (!swbt_diagnostic_path_is_enabled(path)) {
        return 0;
    }

    swbt_diagnostic_trace("btstack: hci dump open");
    result = swbt_btstack_hci_dump_text_open(path);
    if (result != SWBT_BTSTACK_HCI_DUMP_TEXT_OK) {
        swbt_diagnostic_trace("btstack: hci dump open failed");
        return -1;
    }
    hci_dump_init(swbt_btstack_hci_dump_text_instance());
    g_swbt_btstack_production_hci_dump_open = true;
    swbt_diagnostic_trace("btstack: hci dump open ok");
    return 0;
}

static int swbt_btstack_production_hci_dump_start_from_env(void) {
    return swbt_btstack_production_hci_dump_start(getenv("SWBT_HCI_DUMP_TRACE_PATH"));
}

static void swbt_btstack_production_hci_dump_stop(void) {
    if (!g_swbt_btstack_production_hci_dump_open) {
        return;
    }

    swbt_diagnostic_trace("btstack: hci dump close");
    hci_dump_init(NULL);
    swbt_btstack_hci_dump_text_close();
    g_swbt_btstack_production_hci_dump_open = false;
    swbt_diagnostic_trace("btstack: hci dump close done");
}

static int swbt_btstack_production_platform_start(void *context) {
    swbt_btstack_classic_discovery_result_t discovery_result;
    swbt_btstack_classic_discovery_config_t discovery_config;
    (void)context;
    if (swbt_btstack_production_hci_dump_start_from_env() != 0) {
        return -1;
    }
    swbt_diagnostic_trace("btstack: memory init");
    btstack_memory_init();
#if defined(_WIN32)
    swbt_diagnostic_trace("btstack: windows run loop init");
    btstack_run_loop_init(btstack_run_loop_windows_get_instance());
#else
    swbt_diagnostic_trace("btstack: posix run loop init");
    btstack_run_loop_init(btstack_run_loop_posix_get_instance());
#endif
    swbt_diagnostic_trace("btstack: hci init usb transport");
    hci_init(hci_transport_usb_instance(), NULL);
    swbt_diagnostic_trace("btstack: classic discovery configure");
    discovery_config = swbt_btstack_production_discovery_config();
    discovery_result = swbt_btstack_classic_discovery_configure(
        swbt_btstack_classic_discovery_backend_btstack(), NULL, &discovery_config);
    if (discovery_result != SWBT_BTSTACK_CLASSIC_DISCOVERY_OK) {
        swbt_diagnostic_trace("btstack: classic discovery configure failed");
        swbt_btstack_production_hci_dump_stop();
        return -1;
    }
    swbt_diagnostic_trace("btstack: classic discovery configure ok");
    swbt_diagnostic_trace("btstack: l2cap init");
    l2cap_init();
    swbt_btstack_production_ipc_pump_schedule();
    return 0;
}

static void swbt_btstack_production_platform_stop(void *context) {
    (void)context;
    swbt_diagnostic_trace("btstack: ipc pump stop");
    swbt_btstack_production_ipc_pump_stop(NULL);
    swbt_diagnostic_trace("btstack: ipc pump stop done");
    swbt_diagnostic_trace("btstack: hci close");
    hci_close();
    swbt_diagnostic_trace("btstack: hci close done");
    swbt_diagnostic_trace("btstack: run loop deinit");
    btstack_run_loop_deinit();
    swbt_diagnostic_trace("btstack: run loop deinit done");
    swbt_btstack_production_hci_dump_stop();
}

static int
swbt_btstack_production_hid_register(void *context, uint8_t *service_buffer,
                                     size_t service_buffer_size,
                                     const swbt_btstack_hid_registration_config_t *config) {
    int result = 0;
    (void)context;
    swbt_diagnostic_trace("btstack: hid register");
    result = swbt_btstack_hid_device_register(swbt_btstack_hid_registration_backend_btstack(), NULL,
                                              service_buffer, service_buffer_size, config);
    swbt_diagnostic_trace(result == SWBT_BTSTACK_HID_REGISTRATION_OK
                              ? "btstack: hid register ok"
                              : "btstack: hid register failed");
    return result == SWBT_BTSTACK_HID_REGISTRATION_OK ? 0 : -1;
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

static int swbt_btstack_production_report_timer_send_neutral_now(
    void *context, swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    const swbt_btstack_input_report_timer_result_t result =
        swbt_btstack_input_report_timer_adapter_send_neutral_now(adapter);
    if (result == SWBT_BTSTACK_INPUT_REPORT_TIMER_OK) {
        return 0;
    }
    if (result == SWBT_BTSTACK_INPUT_REPORT_TIMER_PENDING) {
        return 1;
    }
    return -1;
}

static void
swbt_btstack_production_report_timer_stop(void *context,
                                          swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    swbt_btstack_input_report_timer_adapter_stop(adapter);
}

static int swbt_btstack_production_ssp_confirm_user_confirmation(void *context,
                                                                 const uint8_t address[6]) {
    (void)context;
    if (address == NULL) {
        return -1;
    }
    return gap_ssp_confirmation_response(address);
}

static int swbt_btstack_production_read_controller_address(void *context, uint8_t address[6]) {
    (void)context;
    if (address == NULL) {
        return -1;
    }

    gap_local_bd_addr(address);
    return 0;
}

static uint32_t swbt_btstack_production_time_ms(void *context) {
    (void)context;
    return btstack_run_loop_get_time_ms();
}

static int swbt_btstack_production_power_on(void *context) {
    int result = 0;
    (void)context;
    swbt_diagnostic_trace("btstack: hci power on");
    result = hci_power_control(HCI_POWER_ON);
    swbt_diagnostic_trace(result == 0 ? "btstack: hci power on ok"
                                      : "btstack: hci power on failed");
    return result;
}

static void swbt_btstack_production_power_off(void *context) {
    (void)context;
    swbt_diagnostic_trace("btstack: hci power off");
    (void)hci_power_control(HCI_POWER_OFF);
}

static void swbt_btstack_production_run_loop_execute(void *context) {
    (void)context;
    btstack_run_loop_execute();
}

static void swbt_btstack_production_run_loop_execute_on_main_thread(
    void *context, btstack_context_callback_registration_t *callback_registration) {
    (void)context;
    btstack_run_loop_execute_on_main_thread(callback_registration);
}

static void swbt_btstack_production_run_loop_trigger_exit(void *context) {
    (void)context;
    btstack_run_loop_trigger_exit();
}

const swbt_btstack_production_adapter_t *swbt_btstack_production_adapter(void) {
    static const swbt_btstack_production_adapter_t adapter = {
        .ipc_pump =
            {
                .start = swbt_btstack_production_ipc_pump_start,
                .stop = swbt_btstack_production_ipc_pump_stop,
            },
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
        .report_timer_send_neutral_now = swbt_btstack_production_report_timer_send_neutral_now,
        .report_timer_stop = swbt_btstack_production_report_timer_stop,
        .ssp_confirm_user_confirmation = swbt_btstack_production_ssp_confirm_user_confirmation,
        .read_controller_address = swbt_btstack_production_read_controller_address,
        .time_ms = swbt_btstack_production_time_ms,
        .power_on = swbt_btstack_production_power_on,
        .power_off = swbt_btstack_production_power_off,
        .run_loop_execute = swbt_btstack_production_run_loop_execute,
        .run_loop_execute_on_main_thread = swbt_btstack_production_run_loop_execute_on_main_thread,
        .run_loop_trigger_exit = swbt_btstack_production_run_loop_trigger_exit,
    };
    return &adapter;
}
