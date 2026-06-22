#ifndef SWBT_IPC_STATUS_H
#define SWBT_IPC_STATUS_H

typedef enum {
    SWBT_IPC_DAEMON_BACKEND_UNKNOWN = 0,
    SWBT_IPC_DAEMON_BACKEND_NOOP = 1,
    SWBT_IPC_DAEMON_BACKEND_PRODUCTION = 2,
} swbt_ipc_daemon_backend_t;

typedef enum {
    SWBT_IPC_DAEMON_LIFECYCLE_UNAVAILABLE = 0,
    SWBT_IPC_DAEMON_LIFECYCLE_STOPPED = 1,
    SWBT_IPC_DAEMON_LIFECYCLE_RUNNING = 2,
} swbt_ipc_daemon_lifecycle_state_t;

typedef enum {
    SWBT_IPC_HARDWARE_APPROVAL_UNAVAILABLE = 0,
    SWBT_IPC_HARDWARE_APPROVAL_APPROVED = 1,
} swbt_ipc_hardware_approval_t;

typedef enum {
    SWBT_IPC_HARDWARE_CHANNEL_UNAVAILABLE = 0,
} swbt_ipc_hardware_channel_state_t;

typedef struct {
    swbt_ipc_daemon_backend_t backend;
    swbt_ipc_daemon_lifecycle_state_t lifecycle_state;
    swbt_ipc_hardware_approval_t hardware_approval;
} swbt_ipc_daemon_status_t;

typedef struct {
    swbt_ipc_hardware_channel_state_t adapter_state;
    swbt_ipc_hardware_channel_state_t switch_connection_state;
    swbt_ipc_hardware_channel_state_t hid_channel_state;
} swbt_ipc_hardware_status_t;

#endif
