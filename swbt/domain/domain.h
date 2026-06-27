#ifndef SWBT_DOMAIN_DOMAIN_H
#define SWBT_DOMAIN_DOMAIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "domain/lease.h"
#include "domain/status.h"
#include "support/metrics.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_device_info.h"
#include "switch/switch_report.h"
#include "switch/switch_rumble.h"
#include "switch/switch_subcommand.h"
#include "switch/switch_subcommand_dispatcher.h"

typedef enum {
    SWBT_DOMAIN_OK = 0,
    SWBT_DOMAIN_ERROR_INVALID_ARGUMENT = -1,
    SWBT_DOMAIN_ERROR_OWNER_BUSY = -2,
    SWBT_DOMAIN_ERROR_NOT_OWNER = -3,
    SWBT_DOMAIN_ERROR_STALE_SEQUENCE = -4,
} swbt_domain_result_t;

typedef enum {
    SWBT_DOMAIN_REVOKE_RELEASE = 0,
    SWBT_DOMAIN_REVOKE_DISCONNECT,
    SWBT_DOMAIN_REVOKE_HEARTBEAT_TIMEOUT,
    SWBT_DOMAIN_REVOKE_SHUTDOWN,
} swbt_domain_revoke_reason_t;

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    uint64_t last_sequence;
    swbt_state_t state;
    swbt_switch_rumble_state_t rumble;
    swbt_metrics_snapshot_t metrics;
    swbt_domain_daemon_status_t daemon;
    swbt_domain_hardware_status_t hardware;
} swbt_domain_status_snapshot_t;

typedef struct swbt_domain swbt_domain_t;

typedef struct {
    uint32_t client_id;
    const swbt_state_t *state;
    uint64_t sequence;
} swbt_domain_set_state_options_t;

typedef struct {
    swbt_domain_revoke_reason_t reason;
    uint32_t client_id;
} swbt_domain_revoke_options_t;

swbt_domain_t *swbt_domain_create(void);
void swbt_domain_destroy(swbt_domain_t *app);
swbt_domain_result_t swbt_domain_acquire(swbt_domain_t *app, uint32_t client_id);
swbt_domain_result_t swbt_domain_set_state(swbt_domain_t *app,
                                           swbt_domain_set_state_options_t options);
swbt_domain_result_t swbt_domain_revoke(swbt_domain_t *app, swbt_domain_revoke_options_t options);
swbt_domain_result_t swbt_domain_read_controller_state(const swbt_domain_t *app,
                                                       swbt_state_t *out_state);
swbt_domain_result_t swbt_domain_read_status(const swbt_domain_t *app,
                                             swbt_domain_status_snapshot_t *out_status);

swbt_domain_result_t
swbt_domain_set_daemon_status(swbt_domain_t *app, const swbt_domain_daemon_status_t *daemon_status);
swbt_domain_result_t
swbt_domain_set_daemon_lifecycle(swbt_domain_t *app,
                                 swbt_domain_daemon_lifecycle_state_t lifecycle_state);
swbt_domain_result_t
swbt_domain_set_hardware_approval(swbt_domain_t *app,
                                  swbt_domain_hardware_approval_t hardware_approval);
swbt_domain_result_t
swbt_domain_set_hardware_status(swbt_domain_t *app,
                                const swbt_domain_hardware_status_t *hardware_status);
swbt_domain_result_t swbt_domain_record_report_tick(swbt_domain_t *app, uint64_t now_us,
                                                    swbt_metrics_report_send_result_t send_result);
swbt_domain_result_t swbt_domain_record_state_update_rejected(swbt_domain_t *app);
swbt_domain_result_t swbt_domain_record_rumble(swbt_domain_t *app, const uint8_t *payload,
                                               uint64_t updated_at_ms);
swbt_domain_result_t swbt_domain_handle_output_report(
    swbt_domain_t *app, const swbt_switch_output_report_t *output_report,
    const swbt_switch_report_options_t *report_options,
    const swbt_switch_device_info_t *device_info, uint64_t updated_at_ms,
    swbt_switch_subcommand_dispatcher_response_t *out_response);

#endif
