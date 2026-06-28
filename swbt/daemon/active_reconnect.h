#ifndef SWBT_DAEMON_ACTIVE_RECONNECT_H
#define SWBT_DAEMON_ACTIVE_RECONNECT_H

#include "btstack_bridge/device.h"
#include "btstack_bridge/production_ports.h"
#include "daemon/config.h"
#include "domain/domain.h"

typedef enum {
    SWBT_DAEMON_ACTIVE_RECONNECT_REQUEST_INVALID = -1,
    SWBT_DAEMON_ACTIVE_RECONNECT_REQUEST_READY = 0,
    SWBT_DAEMON_ACTIVE_RECONNECT_REQUEST_NONE = 1,
} swbt_daemon_active_reconnect_request_result_t;

typedef enum {
    SWBT_DAEMON_ACTIVE_RECONNECT_ACTIVE_FAILED = -1,
    SWBT_DAEMON_ACTIVE_RECONNECT_ACTIVE_NONE = 0,
    SWBT_DAEMON_ACTIVE_RECONNECT_ACTIVE_REQUESTED = 1,
} swbt_daemon_active_reconnect_active_result_t;

typedef struct {
    const swbt_daemon_config_t *config;
    swbt_btstack_device_t *device;
    swbt_domain_t *app;
} swbt_daemon_active_reconnect_t;

swbt_daemon_active_reconnect_request_result_t
swbt_daemon_active_reconnect_build_request(const swbt_daemon_config_t *config,
                                           swbt_btstack_device_connect_request_t *out_request);

void swbt_daemon_active_reconnect_report_failed(swbt_domain_t *app);

swbt_daemon_active_reconnect_active_result_t
swbt_daemon_active_reconnect_request_active(const swbt_daemon_active_reconnect_t *reconnect);

void swbt_daemon_active_reconnect_save_learned_address(
    swbt_daemon_config_t *config, const swbt_daemon_config_file_target_t *target,
    const uint8_t address[6]);

#endif
