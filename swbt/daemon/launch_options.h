#ifndef SWBT_DAEMON_LAUNCH_OPTIONS_H
#define SWBT_DAEMON_LAUNCH_OPTIONS_H

#include <stdbool.h>

#include "daemon/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SWBT_DAEMON_LAUNCH_OPTIONS_OK = 0,
    SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_INVALID_ARGUMENT = -1,
    SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE = -2,
    SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_UNKNOWN_OPTION = -3,
} swbt_daemon_launch_options_result_t;

typedef struct {
    const char *config_path;
} swbt_daemon_launch_options_t;

typedef struct {
    swbt_daemon_config_t config;
    swbt_daemon_config_file_target_t learned_switch_address_target;
    bool learned_switch_address_target_configured;
} swbt_daemon_launch_config_t;

swbt_daemon_launch_options_result_t
swbt_daemon_launch_options_parse(swbt_daemon_launch_options_t *options, int argc,
                                 const char *const argv[]);

bool swbt_daemon_launch_config_prepare(swbt_daemon_launch_config_t *launch_config,
                                       const swbt_daemon_launch_options_t *options,
                                       const swbt_daemon_config_env_t *env);

#ifdef __cplusplus
}
#endif

#endif
