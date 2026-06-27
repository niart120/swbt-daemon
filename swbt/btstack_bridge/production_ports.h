#ifndef SWBT_BTSTACK_BRIDGE_PRODUCTION_PORTS_H
#define SWBT_BTSTACK_BRIDGE_PRODUCTION_PORTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/device.h"
#include "btstack_bridge/input_report_timer_adapter.h"
#include "btstack_bridge/output_report_handler.h"

#define SWBT_BTSTACK_PRODUCTION_HID_CONTROL_PSM SWBT_BTSTACK_DEVICE_HID_CONTROL_PSM
#define SWBT_BTSTACK_PRODUCTION_HID_INTERRUPT_PSM SWBT_BTSTACK_DEVICE_HID_INTERRUPT_PSM

typedef bool (*swbt_btstack_production_ipc_pump_is_running_t)(void *context);
typedef void (*swbt_btstack_production_ipc_pump_poll_once_at_t)(void *context, uint32_t now_ms);

typedef struct {
    swbt_btstack_production_ipc_pump_is_running_t is_running;
    swbt_btstack_production_ipc_pump_poll_once_at_t poll_once_at;
    void *context;
} swbt_btstack_production_ipc_pump_t;

typedef struct {
    int (*start)(void *context, const swbt_btstack_production_ipc_pump_t *pump);
    void (*stop)(void *context);
} swbt_btstack_production_ipc_pump_port_t;

typedef struct {
    void (*start)(void *context, swbt_btstack_output_report_handler_t *handler);
    void (*stop)(void *context);
} swbt_btstack_production_output_handler_port_t;

typedef struct {
    int (*init)(void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
                const swbt_btstack_input_report_timer_adapter_config_t *config);
    int (*start)(void *context, swbt_btstack_input_report_timer_adapter_t *adapter,
                 swbt_btstack_input_report_timer_start_options_t options);
    int (*on_can_send_now)(void *context, swbt_btstack_input_report_timer_adapter_t *adapter);
    int (*enqueue_subcommand_reply)(void *context,
                                    swbt_btstack_input_report_timer_adapter_t *adapter,
                                    uint16_t hid_cid, const uint8_t *report, size_t report_size);
    int (*send_neutral_now)(void *context, swbt_btstack_input_report_timer_adapter_t *adapter);
    void (*stop)(void *context, swbt_btstack_input_report_timer_adapter_t *adapter);
} swbt_btstack_production_report_timer_port_t;

typedef struct {
    int (*confirm_ssp_user_confirmation)(void *context, const uint8_t address[6]);
    int (*read_controller_address)(void *context, uint8_t address[6]);
} swbt_btstack_production_controller_port_t;

typedef struct {
    uint32_t (*time_ms)(void *context);
} swbt_btstack_production_clock_port_t;

typedef struct {
    int (*on)(void *context);
    void (*off)(void *context);
} swbt_btstack_production_power_port_t;

typedef struct {
    void (*execute)(void *context);
    void (*execute_on_main_thread)(void *context,
                                   btstack_context_callback_registration_t *callback_registration);
    void (*trigger_exit)(void *context);
} swbt_btstack_production_run_loop_port_t;

typedef struct {
    swbt_btstack_production_ipc_pump_port_t ipc_pump;
    swbt_btstack_device_port_t device;
    swbt_btstack_production_output_handler_port_t output_handler;
    swbt_btstack_production_report_timer_port_t report_timer;
    swbt_btstack_production_controller_port_t controller;
    swbt_btstack_production_clock_port_t clock;
    swbt_btstack_production_power_port_t power;
    swbt_btstack_production_run_loop_port_t run_loop;
} swbt_btstack_production_ports_t;

swbt_btstack_hid_registration_config_t swbt_btstack_production_hid_registration_config(void);

#endif
