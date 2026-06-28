#include "btstack_bridge/production_ports.h"

static int expect_true(int actual, const char *label) {
    (void)label;
    return actual != 0 ? 0 : 1;
}

static int fake_ipc_start(void *context, const swbt_btstack_production_ipc_pump_t *pump) {
    (void)context;
    (void)pump;
    return 0;
}

static void fake_ipc_stop(void *context) {
    (void)context;
}

static int fake_platform_start(void *context) {
    (void)context;
    return 0;
}

static void fake_platform_stop(void *context) {
    (void)context;
}

static int fake_hid_register(void *context, uint8_t *service_buffer, size_t service_buffer_size,
                             const swbt_btstack_hid_registration_config_t *config) {
    (void)context;
    (void)service_buffer;
    (void)service_buffer_size;
    (void)config;
    return 0;
}

static void fake_hid_stop(void *context) {
    (void)context;
}

static int fake_connect(void *context, const swbt_btstack_device_connect_request_t *request,
                        uint16_t *out_hid_cid) {
    (void)context;
    (void)request;
    if (out_hid_cid != NULL) {
        *out_hid_cid = 0x0042u;
    }
    return 0;
}

static int fake_disconnect(void *context, uint16_t hid_cid) {
    (void)context;
    (void)hid_cid;
    return 0;
}

static int fake_send(void *context, uint16_t hid_cid, const uint8_t *message, size_t message_size) {
    (void)context;
    (void)hid_cid;
    (void)message;
    (void)message_size;
    return 0;
}

static void fake_output_start(void *context, swbt_btstack_output_report_handler_t *handler) {
    (void)context;
    (void)handler;
}

static void fake_output_stop(void *context) {
    (void)context;
}

static int fake_report_timer_init(void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
                                  const swbt_btstack_input_report_timer_adapter_config_t *config) {
    (void)context;
    (void)adapter;
    (void)config;
    return 0;
}

static int fake_report_timer_start(void *context,
                                   swbt_btstack_input_report_timer_adapter_t *adapter,
                                   swbt_btstack_input_report_timer_start_options_t options) {
    (void)context;
    (void)adapter;
    (void)options;
    return 0;
}

static int fake_report_timer_on_can_send_now(void *context,
                                             swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    (void)adapter;
    return 0;
}

static int fake_report_timer_enqueue_subcommand_reply(
    void *context, swbt_btstack_input_report_timer_adapter_t *adapter, uint16_t hid_cid,
    const uint8_t *report, size_t report_size) {
    (void)context;
    (void)adapter;
    (void)hid_cid;
    (void)report;
    (void)report_size;
    return 0;
}

static int fake_report_timer_send_neutral_now(void *context,
                                              swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    (void)adapter;
    return 0;
}

static void fake_report_timer_stop(void *context,
                                   swbt_btstack_input_report_timer_adapter_t *adapter) {
    (void)context;
    (void)adapter;
}

static int fake_confirm_ssp_user_confirmation(void *context, const uint8_t address[6]) {
    (void)context;
    (void)address;
    return 0;
}

static int fake_read_controller_address(void *context, uint8_t address[6]) {
    (void)context;
    if (address != NULL) {
        address[0] = 0x01u;
    }
    return 0;
}

static uint32_t fake_time_ms(void *context) {
    (void)context;
    return 0u;
}

static int fake_power_on(void *context) {
    (void)context;
    return 0;
}

static void fake_power_off(void *context) {
    (void)context;
}

static void fake_run_loop_execute(void *context) {
    (void)context;
}

static void fake_run_loop_execute_on_main_thread(
    void *context, btstack_context_callback_registration_t *callback_registration) {
    (void)context;
    (void)callback_registration;
}

static void fake_run_loop_set_timer_handler(void *context, btstack_timer_source_t *timer,
                                            void (*process)(btstack_timer_source_t *timer)) {
    (void)context;
    (void)timer;
    (void)process;
}

static void fake_run_loop_set_timer_context(void *context, btstack_timer_source_t *timer,
                                            void *timer_context) {
    (void)context;
    (void)timer;
    (void)timer_context;
}

static void fake_run_loop_set_timer(void *context, btstack_timer_source_t *timer,
                                    uint32_t timeout_ms) {
    (void)context;
    (void)timer;
    (void)timeout_ms;
}

static void fake_run_loop_add_timer(void *context, btstack_timer_source_t *timer) {
    (void)context;
    (void)timer;
}

static int fake_run_loop_remove_timer(void *context, btstack_timer_source_t *timer) {
    (void)context;
    (void)timer;
    return 0;
}

static void fake_run_loop_trigger_exit(void *context) {
    (void)context;
}

static swbt_btstack_production_ports_t ipc_pump_only_ports(void) {
    return (swbt_btstack_production_ports_t){
        .ipc_pump =
            {
                .start = fake_ipc_start,
                .stop = fake_ipc_stop,
            },
    };
}

static swbt_btstack_production_ports_t full_ports(void) {
    return (swbt_btstack_production_ports_t){
        .ipc_pump =
            {
                .start = fake_ipc_start,
                .stop = fake_ipc_stop,
            },
        .device =
            {
                .platform_start = fake_platform_start,
                .platform_stop = fake_platform_stop,
                .hid_register = fake_hid_register,
                .hid_stop = fake_hid_stop,
                .connect = fake_connect,
                .disconnect = fake_disconnect,
                .send = fake_send,
            },
        .output_handler =
            {
                .start = fake_output_start,
                .stop = fake_output_stop,
            },
        .report_timer =
            {
                .init = fake_report_timer_init,
                .start = fake_report_timer_start,
                .on_can_send_now = fake_report_timer_on_can_send_now,
                .enqueue_subcommand_reply = fake_report_timer_enqueue_subcommand_reply,
                .send_neutral_now = fake_report_timer_send_neutral_now,
                .stop = fake_report_timer_stop,
            },
        .controller =
            {
                .confirm_ssp_user_confirmation = fake_confirm_ssp_user_confirmation,
                .read_controller_address = fake_read_controller_address,
            },
        .clock =
            {
                .time_ms = fake_time_ms,
            },
        .power =
            {
                .on = fake_power_on,
                .off = fake_power_off,
            },
        .run_loop =
            {
                .execute = fake_run_loop_execute,
                .execute_on_main_thread = fake_run_loop_execute_on_main_thread,
                .set_timer_handler = fake_run_loop_set_timer_handler,
                .set_timer_context = fake_run_loop_set_timer_context,
                .set_timer = fake_run_loop_set_timer,
                .add_timer = fake_run_loop_add_timer,
                .remove_timer = fake_run_loop_remove_timer,
                .trigger_exit = fake_run_loop_trigger_exit,
            },
    };
}

static int ipc_pump_only_ports_pass_initialization_validation(void) {
    const swbt_btstack_production_ports_t ports = ipc_pump_only_ports();
    return expect_true(swbt_btstack_production_ports_has_ipc_pump(&ports), "ipc pump validation");
}

static int ipc_pump_only_ports_fail_startup_validation(void) {
    const swbt_btstack_production_ports_t ports = ipc_pump_only_ports();
    return expect_true(!swbt_btstack_production_ports_is_valid(&ports),
                       "startup validation rejects IPC-only ports");
}

static int full_ports_pass_startup_validation(void) {
    const swbt_btstack_production_ports_t ports = full_ports();
    return expect_true(swbt_btstack_production_ports_is_valid(&ports), "full startup validation");
}

int main(void) {
    int failed = 0;
    failed += ipc_pump_only_ports_pass_initialization_validation();
    failed += ipc_pump_only_ports_fail_startup_validation();
    failed += full_ports_pass_startup_validation();
    return failed == 0 ? 0 : 1;
}
