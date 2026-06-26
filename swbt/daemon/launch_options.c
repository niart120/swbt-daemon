#include "daemon/launch_options.h"

#include <string.h>

static const char *swbt_daemon_launch_options_value_after_equals(const char *argument,
                                                                 const char *prefix) {
    const size_t prefix_length = strlen(prefix);
    if (strncmp(argument, prefix, prefix_length) != 0) {
        return NULL;
    }
    return argument + prefix_length;
}

swbt_daemon_launch_options_result_t
swbt_daemon_launch_options_parse(swbt_daemon_launch_options_t *options, int argc,
                                 const char *const argv[]) {
    if (options == NULL || argc < 0 || (argc > 0 && argv == NULL)) {
        return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_INVALID_ARGUMENT;
    }

    *options = (swbt_daemon_launch_options_t){0};

    for (int index = 1; index < argc; ++index) {
        const char *argument = argv[index];
        const char *config_value = NULL;

        if (argument == NULL) {
            return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_INVALID_ARGUMENT;
        }
        if (strcmp(argument, "--config") == 0) {
            if (index + 1 >= argc || argv[index + 1] == NULL || argv[index + 1][0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->config_path = argv[index + 1];
            ++index;
            continue;
        }

        config_value = swbt_daemon_launch_options_value_after_equals(argument, "--config=");
        if (config_value != NULL) {
            if (config_value[0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->config_path = config_value;
            continue;
        }

        return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_UNKNOWN_OPTION;
    }

    return SWBT_DAEMON_LAUNCH_OPTIONS_OK;
}
