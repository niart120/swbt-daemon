#ifndef SWBT_DAEMON_CONFIG_H
#define SWBT_DAEMON_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#include "daemon/switch_address.h"
#include "switch/switch_device_info.h"
#include "switch/switch_report.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SWBT_DAEMON_DEFAULT_IPC_HOST "127.0.0.1"
#define SWBT_DAEMON_DEFAULT_IPC_PORT 0u
#define SWBT_DAEMON_DEFAULT_IPC_BACKLOG 1
#define SWBT_DAEMON_DEFAULT_IPC_HEARTBEAT_TIMEOUT_MS 0u
#define SWBT_DAEMON_DEFAULT_REPORT_PERIOD_US 8000u
#define SWBT_DAEMON_CONFIG_IPC_HOST_SIZE 16u
#define SWBT_DAEMON_CONFIG_SWITCH_ADDRESS_SIZE SWBT_DAEMON_SWITCH_ADDRESS_TEXT_SIZE

typedef struct {
    uint32_t report_period_us;
    char ipc_host[SWBT_DAEMON_CONFIG_IPC_HOST_SIZE];
    uint16_t ipc_port;
    int ipc_backlog;
    uint32_t ipc_heartbeat_timeout_ms;
    char active_reconnect_explicit_switch_address[SWBT_DAEMON_CONFIG_SWITCH_ADDRESS_SIZE];
    char active_reconnect_learned_switch_address[SWBT_DAEMON_CONFIG_SWITCH_ADDRESS_SIZE];
    swbt_switch_report_options_t report_options;
    swbt_switch_device_info_t device_info;
} swbt_daemon_config_t;

typedef struct {
    const char *report_period_us;
    const char *ipc_host;
    const char *ipc_port;
    const char *ipc_backlog;
    const char *ipc_heartbeat_timeout_ms;
    const char *device_info_profile;
} swbt_daemon_config_env_t;

typedef enum {
    SWBT_DAEMON_CONFIG_FILE_OK = 0,
    SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_ARGUMENT = -1,
    SWBT_DAEMON_CONFIG_FILE_ERROR_IO = -2,
    SWBT_DAEMON_CONFIG_FILE_ERROR_PARSE = -3,
    SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE = -4,
} swbt_daemon_config_file_result_t;

typedef struct {
    const char *path;
    bool required;
} swbt_daemon_config_file_source_t;

typedef struct {
    const char *path;
} swbt_daemon_config_file_target_t;

swbt_daemon_config_t swbt_daemon_config_default(void);

bool swbt_daemon_config_set_ipc_host(swbt_daemon_config_t *config, const char *host);

bool swbt_daemon_config_set_active_reconnect_explicit_switch_address(swbt_daemon_config_t *config,
                                                                     const char *address);

bool swbt_daemon_config_set_active_reconnect_learned_switch_address(swbt_daemon_config_t *config,
                                                                    const char *address);

const char *
swbt_daemon_config_effective_reconnect_switch_address(const swbt_daemon_config_t *config);

swbt_daemon_config_file_result_t
swbt_daemon_config_apply_file(swbt_daemon_config_t *config,
                              const swbt_daemon_config_file_source_t *source);

swbt_daemon_config_file_result_t swbt_daemon_config_save_active_reconnect_learned_switch_address(
    swbt_daemon_config_t *config, const swbt_daemon_config_file_target_t *target,
    const char *address);

bool swbt_daemon_config_apply_env(swbt_daemon_config_t *config,
                                  const swbt_daemon_config_env_t *env);

bool swbt_daemon_config_apply_device_info_profile(swbt_daemon_config_t *config,
                                                  const char *profile);

#ifdef __cplusplus
}
#endif

#endif
