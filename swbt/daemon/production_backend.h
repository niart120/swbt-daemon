#ifndef SWBT_DAEMON_PRODUCTION_BACKEND_H
#define SWBT_DAEMON_PRODUCTION_BACKEND_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "btstack_bridge/hid_device_registration.h"
#include "btstack_bridge/input_report_timer_adapter.h"
#include "btstack_bridge/output_report_handler.h"
#include "daemon/config.h"
#include "daemon/ipc_runner.h"
#include "daemon/runtime.h"

#define SWBT_DAEMON_PRODUCTION_HID_SERVICE_BUFFER_SIZE 300u

typedef enum {
    SWBT_DAEMON_PRODUCTION_OK = 0,
    SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT = -1,
    SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME = -2,
    SWBT_DAEMON_PRODUCTION_ERROR_HARDWARE_APPROVAL_REQUIRED = -3,
    SWBT_DAEMON_PRODUCTION_ERROR_HARDWARE = -4,
} swbt_daemon_production_result_t;

typedef struct {
    bool run_hardware;
    bool hardware_approved;
} swbt_daemon_hardware_approval_t;

typedef void (*swbt_daemon_shutdown_request_t)(void *context);

typedef struct {
    int (*install)(void *context, swbt_daemon_shutdown_request_t request_shutdown,
                   void *request_context);
    void (*uninstall)(void *context);
} swbt_daemon_shutdown_listener_t;

typedef struct {
    int (*ipc_start)(void *context, swbt_daemon_ipc_runner_t *runner, swbt_ipc_session_t *session,
                     const swbt_daemon_ipc_runner_config_t *config);
    void (*ipc_stop)(void *context, swbt_daemon_ipc_runner_t *runner);
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
    void (*report_timer_stop)(void *context, swbt_btstack_input_report_timer_adapter_t *adapter);
    uint32_t (*time_ms)(void *context);
    int (*power_on)(void *context);
    void (*power_off)(void *context);
    void (*run_loop_execute)(void *context);
    void (*run_loop_trigger_exit)(void *context);
} swbt_daemon_production_backend_ops_t;

typedef struct {
    swbt_daemon_config_t config;
    swbt_daemon_ipc_runner_t ipc_runner;
    swbt_btstack_input_report_timer_adapter_t report_timer;
    const swbt_daemon_production_backend_ops_t *ops;
    void *ops_context;
    uint8_t hid_service_buffer[SWBT_DAEMON_PRODUCTION_HID_SERVICE_BUFFER_SIZE];
    bool initialized;
    bool platform_started;
    bool hid_registered;
    bool report_timer_initialized;
    atomic_bool hardware_powered;
    atomic_bool shutdown_requested;
} swbt_daemon_production_backend_t;

swbt_daemon_production_result_t swbt_daemon_production_backend_init(
    swbt_daemon_production_backend_t *backend, const swbt_daemon_config_t *config,
    const swbt_daemon_production_backend_ops_t *ops, void *ops_context);

const swbt_daemon_runtime_backend_t *swbt_daemon_production_runtime_backend(void);

swbt_daemon_production_result_t
swbt_daemon_production_main_with_backend(swbt_daemon_production_backend_t *backend,
                                         const swbt_daemon_hardware_approval_t *approval);

swbt_daemon_production_result_t swbt_daemon_production_main_with_backend_and_shutdown(
    swbt_daemon_production_backend_t *backend, const swbt_daemon_hardware_approval_t *approval,
    const swbt_daemon_shutdown_listener_t *shutdown_listener, void *shutdown_context);

uint32_t
swbt_daemon_production_backend_report_period_us(const swbt_daemon_production_backend_t *backend);

swbt_daemon_ipc_runner_config_t
swbt_daemon_production_backend_ipc_config(const swbt_daemon_production_backend_t *backend);

bool swbt_daemon_hardware_approval_is_granted(const swbt_daemon_hardware_approval_t *approval);

#endif
