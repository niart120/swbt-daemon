#ifndef SWBT_DAEMON_PRODUCTION_BACKEND_OPS_H
#define SWBT_DAEMON_PRODUCTION_BACKEND_OPS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/hid_device_registration.h"
#include "btstack_bridge/input_report_timer_adapter.h"
#include "btstack_bridge/output_report_handler.h"

typedef bool (*swbt_daemon_production_ipc_pump_is_running_t)(void *context);
typedef void (*swbt_daemon_production_ipc_pump_poll_once_at_t)(void *context, uint32_t now_ms);

typedef struct {
    swbt_daemon_production_ipc_pump_is_running_t is_running;
    swbt_daemon_production_ipc_pump_poll_once_at_t poll_once_at;
    void *context;
} swbt_daemon_production_ipc_pump_t;

typedef struct {
    int (*ipc_pump_start)(void *context, const swbt_daemon_production_ipc_pump_t *pump);
    void (*ipc_pump_stop)(void *context);
    int (*platform_start)(void *context);
    void (*platform_stop)(void *context);
    int (*hid_register)(void *context, uint8_t *service_buffer, size_t service_buffer_size,
                        const swbt_btstack_hid_registration_config_t *config);
    void (*hid_stop)(void *context);
    void (*output_handler_start)(void *context, swbt_btstack_output_report_handler_t *handler);
    void (*output_handler_stop)(void *context);
    int (*report_timer_init)(void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
                             const swbt_btstack_input_report_timer_adapter_config_t *config);
    int (*report_timer_start)(void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
                              swbt_btstack_input_report_timer_start_options_t options);
    int (*report_timer_on_can_send_now)(void *context,
                                        swbt_btstack_input_report_timer_adapter_t *adapter);
    int (*report_timer_enqueue_subcommand_reply)(void *context,
                                                 swbt_btstack_input_report_timer_adapter_t *adapter,
                                                 uint16_t hid_cid, const uint8_t *report,
                                                 size_t report_size);
    int (*report_timer_send_neutral_now)(void *context,
                                         swbt_btstack_input_report_timer_adapter_t *adapter);
    void (*report_timer_stop)(void *context, swbt_btstack_input_report_timer_adapter_t *adapter);
    int (*ssp_confirm_user_confirmation)(void *context, const uint8_t address[6]);
    int (*read_controller_address)(void *context, uint8_t address[6]);
    uint32_t (*time_ms)(void *context);
    int (*power_on)(void *context);
    void (*power_off)(void *context);
    void (*run_loop_execute)(void *context);
    void (*run_loop_trigger_exit)(void *context);
} swbt_daemon_production_backend_ops_t;

swbt_btstack_hid_registration_config_t swbt_daemon_production_hid_registration_config(void);

#endif
