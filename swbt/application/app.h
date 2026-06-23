#ifndef SWBT_APPLICATION_APP_H
#define SWBT_APPLICATION_APP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "application/control_lease.h"
#include "application/status.h"
#include "core/metrics.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_device_info.h"
#include "switch/switch_report.h"
#include "switch/switch_rumble.h"
#include "switch/switch_subcommand.h"
#include "switch/switch_subcommand_dispatcher.h"

typedef enum {
    SWBT_APP_OK = 0,
    SWBT_APP_ERROR_INVALID_ARGUMENT = -1,
    SWBT_APP_ERROR_OWNER_BUSY = -2,
    SWBT_APP_ERROR_NOT_OWNER = -3,
    SWBT_APP_ERROR_STALE_SEQUENCE = -4,
} swbt_app_result_t;

typedef enum {
    SWBT_APP_REVOKE_RELEASE = 0,
    SWBT_APP_REVOKE_DISCONNECT,
    SWBT_APP_REVOKE_HEARTBEAT_TIMEOUT,
    SWBT_APP_REVOKE_SHUTDOWN,
} swbt_app_revoke_reason_t;

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    uint64_t last_sequence;
    swbt_state_t state;
    swbt_switch_rumble_state_t rumble;
    swbt_metrics_snapshot_t metrics;
    swbt_app_daemon_status_t daemon;
    swbt_app_hardware_status_t hardware;
} swbt_app_snapshot_t;

typedef struct swbt_app swbt_app_t;

swbt_app_t *swbt_app_create(void);
void swbt_app_destroy(swbt_app_t *app);
swbt_app_result_t swbt_app_acquire(swbt_app_t *app, uint32_t client_id);
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_app_result_t swbt_app_set_state(swbt_app_t *app, uint32_t client_id, const swbt_state_t *state,
                                     uint64_t sequence);
// NOLINTEND(bugprone-easily-swappable-parameters)
// NOLINTBEGIN(bugprone-easily-swappable-parameters)
swbt_app_result_t swbt_app_revoke(swbt_app_t *app, swbt_app_revoke_reason_t reason,
                                  uint32_t client_id);
// NOLINTEND(bugprone-easily-swappable-parameters)
swbt_app_result_t swbt_app_snapshot(const swbt_app_t *app, swbt_app_snapshot_t *out_snapshot);

swbt_app_result_t swbt_app_set_daemon_status(swbt_app_t *app,
                                             const swbt_app_daemon_status_t *daemon_status);
swbt_app_result_t swbt_app_set_daemon_lifecycle(swbt_app_t *app,
                                                swbt_app_daemon_lifecycle_state_t lifecycle_state);
swbt_app_result_t swbt_app_set_hardware_approval(swbt_app_t *app,
                                                 swbt_app_hardware_approval_t hardware_approval);
swbt_app_result_t swbt_app_record_report_tick(swbt_app_t *app, uint64_t now_us,
                                              swbt_metrics_report_send_result_t send_result);
swbt_app_result_t swbt_app_record_state_update_rejected(swbt_app_t *app);
swbt_app_result_t swbt_app_record_rumble(swbt_app_t *app, const uint8_t *payload,
                                         uint64_t updated_at_ms);
swbt_app_result_t
swbt_app_handle_output_report(swbt_app_t *app, const swbt_switch_output_report_t *output_report,
                              const swbt_switch_report_options_t *report_options,
                              const swbt_switch_device_info_t *device_info, uint64_t updated_at_ms,
                              swbt_switch_subcommand_dispatcher_response_t *out_response);

#endif
