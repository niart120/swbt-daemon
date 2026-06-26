#include "daemon/launch_options.h"

#include <stdbool.h>
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
        const char *experimental_link_key_db_value = NULL;

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

        if (strcmp(argument, "--experimental-link-key-db") == 0) {
            if (index + 1 >= argc || argv[index + 1] == NULL || argv[index + 1][0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->experimental_link_key_db_path = argv[index + 1];
            ++index;
            continue;
        }

        experimental_link_key_db_value =
            swbt_daemon_launch_options_value_after_equals(argument, "--experimental-link-key-db=");
        if (experimental_link_key_db_value != NULL) {
            if (experimental_link_key_db_value[0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->experimental_link_key_db_path = experimental_link_key_db_value;
            continue;
        }

        return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_UNKNOWN_OPTION;
    }

    return SWBT_DAEMON_LAUNCH_OPTIONS_OK;
}

bool swbt_daemon_launch_config_prepare(swbt_daemon_launch_config_t *launch_config,
                                       const swbt_daemon_launch_options_t *options,
                                       const swbt_daemon_config_env_t *env) {
    swbt_daemon_config_file_result_t file_result;

    if (launch_config == NULL || options == NULL || env == NULL) {
        return false;
    }

    *launch_config = (swbt_daemon_launch_config_t){
        .config = swbt_daemon_config_default(),
    };

    if (options->config_path != NULL) {
        const swbt_daemon_config_file_source_t source = {
            .path = options->config_path,
            .required = true,
        };
        file_result = swbt_daemon_config_apply_file(&launch_config->config, &source);
        if (file_result != SWBT_DAEMON_CONFIG_FILE_OK) {
            *launch_config = (swbt_daemon_launch_config_t){0};
            return false;
        }
        launch_config->learned_switch_address_target =
            (swbt_daemon_config_file_target_t){.path = options->config_path};
        launch_config->learned_switch_address_target_configured = true;
    }

    if (options->experimental_link_key_db_path != NULL) {
        launch_config->experimental_link_key_db_path = options->experimental_link_key_db_path;
        launch_config->experimental_link_key_db_configured = true;
    }

    if (!swbt_daemon_config_apply_env(&launch_config->config, env)) {
        *launch_config = (swbt_daemon_launch_config_t){0};
        return false;
    }

    return true;
}
