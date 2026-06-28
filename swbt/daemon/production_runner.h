#ifndef SWBT_DAEMON_PRODUCTION_RUNNER_H
#define SWBT_DAEMON_PRODUCTION_RUNNER_H

#include <stdbool.h>

#include "daemon/config.h"
#include "daemon/shutdown_listener.h"

typedef struct swbt_btstack_production_ports swbt_btstack_production_ports_t;
typedef struct swbt_daemon_production_runner swbt_daemon_production_runner_t;

typedef enum {
    SWBT_DAEMON_PRODUCTION_OK = 0,
    SWBT_DAEMON_PRODUCTION_ERROR_INVALID_ARGUMENT = -1,
    SWBT_DAEMON_PRODUCTION_ERROR_RUNTIME = -2,
    SWBT_DAEMON_PRODUCTION_ERROR_HARDWARE_APPROVAL_REQUIRED = -3,
    SWBT_DAEMON_PRODUCTION_ERROR_HARDWARE = -4,
    SWBT_DAEMON_PRODUCTION_ERROR_ADAPTER_LOCATION_REQUIRED = -5,
} swbt_daemon_production_result_t;

typedef struct {
    const swbt_daemon_config_t *config;
    const swbt_btstack_production_ports_t *ports;
    void *ports_context;
    bool adapter_location_configured;
    bool learned_switch_address_target_configured;
    swbt_daemon_config_file_target_t learned_switch_address_target;
    const swbt_daemon_shutdown_listener_t *shutdown_listener;
    void *shutdown_context;
} swbt_daemon_production_run_config_t;

swbt_daemon_production_result_t swbt_daemon_production_runner_init(
    swbt_daemon_production_runner_t *backend, const swbt_daemon_config_t *config,
    const swbt_btstack_production_ports_t *ports, void *ports_context);

bool swbt_daemon_production_runner_set_learned_switch_address_target(
    swbt_daemon_production_runner_t *backend, const swbt_daemon_config_file_target_t *target);

bool swbt_daemon_production_runner_set_adapter_location_configured(
    swbt_daemon_production_runner_t *backend);

swbt_daemon_production_result_t
swbt_daemon_production_main_with_runner(swbt_daemon_production_runner_t *backend);

swbt_daemon_production_result_t swbt_daemon_production_main_with_runner_and_shutdown(
    swbt_daemon_production_runner_t *backend,
    const swbt_daemon_shutdown_listener_t *shutdown_listener, void *shutdown_context);

swbt_daemon_production_result_t
swbt_daemon_production_run(const swbt_daemon_production_run_config_t *run_config);

#endif
