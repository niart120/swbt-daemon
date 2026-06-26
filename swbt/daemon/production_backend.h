#ifndef SWBT_DAEMON_PRODUCTION_BACKEND_H
#define SWBT_DAEMON_PRODUCTION_BACKEND_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "daemon/config.h"
#include "btstack_bridge/production_adapter.h"
#include "daemon/host.h"
#include "daemon/ipc_runner.h"

#define SWBT_DAEMON_PRODUCTION_HID_SERVICE_BUFFER_SIZE 512u

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

typedef struct {
    const char *run_hardware;
    const char *hardware_approved;
} swbt_daemon_hardware_approval_env_t;

typedef void (*swbt_daemon_shutdown_request_t)(void *context);

typedef struct {
    int (*install)(void *context, swbt_daemon_shutdown_request_t request_shutdown,
                   void *request_context);
    void (*uninstall)(void *context);
} swbt_daemon_shutdown_listener_t;

typedef struct {
    swbt_daemon_config_t config;
    swbt_daemon_config_file_target_t learned_switch_address_target;
    swbt_daemon_ipc_runner_t ipc_runner;
    swbt_btstack_input_report_timer_adapter_t report_timer;
    const swbt_btstack_production_adapter_t *adapter;
    void *adapter_context;
    swbt_daemon_host_t *host;
    uint8_t hid_service_buffer[SWBT_DAEMON_PRODUCTION_HID_SERVICE_BUFFER_SIZE];
    bool initialized;
    bool platform_started;
    bool hid_registered;
    bool report_timer_initialized;
    bool shutdown_neutral_pending;
    bool learned_switch_address_target_configured;
    btstack_context_callback_registration_t shutdown_callback;
    atomic_bool hardware_powered;
    atomic_bool shutdown_requested;
} swbt_daemon_production_backend_t;

swbt_daemon_production_result_t swbt_daemon_production_backend_init(
    swbt_daemon_production_backend_t *backend, const swbt_daemon_config_t *config,
    const swbt_btstack_production_adapter_t *adapter, void *adapter_context);

bool swbt_daemon_production_backend_set_learned_switch_address_target(
    swbt_daemon_production_backend_t *backend, const swbt_daemon_config_file_target_t *target);

const swbt_daemon_host_backend_t *swbt_daemon_production_host_backend(void);

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

swbt_daemon_hardware_approval_t
swbt_daemon_hardware_approval_from_env(const swbt_daemon_hardware_approval_env_t *env);

#endif
