#ifndef SWBT_DAEMON_LAUNCH_OPTIONS_H
#define SWBT_DAEMON_LAUNCH_OPTIONS_H

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

swbt_daemon_launch_options_result_t
swbt_daemon_launch_options_parse(swbt_daemon_launch_options_t *options, int argc,
                                 const char *const argv[]);

#ifdef __cplusplus
}
#endif

#endif
