#ifndef SWBT_IPC_STATUS_H
#define SWBT_IPC_STATUS_H

#include <stdbool.h>
#include <stdint.h>

#include "application/status.h"
#include "core/metrics.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_rumble.h"

typedef swbt_app_daemon_backend_t swbt_ipc_daemon_backend_t;
typedef swbt_app_daemon_lifecycle_state_t swbt_ipc_daemon_lifecycle_state_t;
typedef swbt_app_hardware_approval_t swbt_ipc_hardware_approval_t;
typedef swbt_app_hardware_channel_state_t swbt_ipc_hardware_channel_state_t;
typedef swbt_app_daemon_status_t swbt_ipc_daemon_status_t;
typedef swbt_app_hardware_status_t swbt_ipc_hardware_status_t;

#define SWBT_IPC_DAEMON_BACKEND_UNKNOWN SWBT_APP_DAEMON_BACKEND_UNKNOWN
#define SWBT_IPC_DAEMON_BACKEND_NOOP SWBT_APP_DAEMON_BACKEND_NOOP
#define SWBT_IPC_DAEMON_BACKEND_PRODUCTION SWBT_APP_DAEMON_BACKEND_PRODUCTION

#define SWBT_IPC_DAEMON_LIFECYCLE_UNAVAILABLE SWBT_APP_DAEMON_LIFECYCLE_UNAVAILABLE
#define SWBT_IPC_DAEMON_LIFECYCLE_STOPPED SWBT_APP_DAEMON_LIFECYCLE_STOPPED
#define SWBT_IPC_DAEMON_LIFECYCLE_RUNNING SWBT_APP_DAEMON_LIFECYCLE_RUNNING

#define SWBT_IPC_HARDWARE_APPROVAL_UNAVAILABLE SWBT_APP_HARDWARE_APPROVAL_UNAVAILABLE
#define SWBT_IPC_HARDWARE_APPROVAL_APPROVED SWBT_APP_HARDWARE_APPROVAL_APPROVED

#define SWBT_IPC_HARDWARE_CHANNEL_UNAVAILABLE SWBT_APP_HARDWARE_CHANNEL_UNAVAILABLE

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    uint64_t last_seq;
    swbt_state_t state;
    swbt_switch_rumble_state_t rumble;
    swbt_metrics_snapshot_t metrics;
    swbt_ipc_daemon_status_t daemon;
    swbt_ipc_hardware_status_t hardware;
} swbt_ipc_status_t;

#endif
