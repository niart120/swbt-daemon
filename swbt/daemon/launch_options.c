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

static bool swbt_daemon_launch_options_parse_backend(swbt_daemon_launch_backend_t *out_backend,
                                                     const char *value) {
    if (out_backend == NULL || value == NULL) {
        return false;
    }
    if (strcmp(value, "production") == 0) {
        *out_backend = SWBT_DAEMON_LAUNCH_BACKEND_PRODUCTION;
        return true;
    }
    if (strcmp(value, "noop") == 0) {
        *out_backend = SWBT_DAEMON_LAUNCH_BACKEND_NOOP;
        return true;
    }
    return false;
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
        const char *link_key_db_value = NULL;
        const char *trace_value = NULL;
        const char *hci_dump_value = NULL;
        const char *crash_dump_value = NULL;
        const char *backend_value = NULL;

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

        if (strcmp(argument, "--link-key-db") == 0) {
            if (index + 1 >= argc || argv[index + 1] == NULL || argv[index + 1][0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->link_key_db_path = argv[index + 1];
            ++index;
            continue;
        }

        link_key_db_value =
            swbt_daemon_launch_options_value_after_equals(argument, "--link-key-db=");
        if (link_key_db_value != NULL) {
            if (link_key_db_value[0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->link_key_db_path = link_key_db_value;
            continue;
        }

        if (strcmp(argument, "--trace-path") == 0) {
            if (index + 1 >= argc || argv[index + 1] == NULL || argv[index + 1][0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->trace_path = argv[index + 1];
            ++index;
            continue;
        }

        trace_value = swbt_daemon_launch_options_value_after_equals(argument, "--trace-path=");
        if (trace_value != NULL) {
            if (trace_value[0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->trace_path = trace_value;
            continue;
        }

        if (strcmp(argument, "--hci-dump-path") == 0) {
            if (index + 1 >= argc || argv[index + 1] == NULL || argv[index + 1][0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->hci_dump_path = argv[index + 1];
            ++index;
            continue;
        }

        hci_dump_value =
            swbt_daemon_launch_options_value_after_equals(argument, "--hci-dump-path=");
        if (hci_dump_value != NULL) {
            if (hci_dump_value[0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->hci_dump_path = hci_dump_value;
            continue;
        }

        if (strcmp(argument, "--crash-dump-path") == 0) {
            if (index + 1 >= argc || argv[index + 1] == NULL || argv[index + 1][0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->crash_dump_path = argv[index + 1];
            ++index;
            continue;
        }

        crash_dump_value =
            swbt_daemon_launch_options_value_after_equals(argument, "--crash-dump-path=");
        if (crash_dump_value != NULL) {
            if (crash_dump_value[0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            options->crash_dump_path = crash_dump_value;
            continue;
        }

        if (strcmp(argument, "--backend") == 0) {
            if (index + 1 >= argc || argv[index + 1] == NULL || argv[index + 1][0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            if (!swbt_daemon_launch_options_parse_backend(&options->backend, argv[index + 1])) {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_INVALID_ARGUMENT;
            }
            ++index;
            continue;
        }

        backend_value = swbt_daemon_launch_options_value_after_equals(argument, "--backend=");
        if (backend_value != NULL) {
            if (backend_value[0] == '\0') {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_MISSING_VALUE;
            }
            if (!swbt_daemon_launch_options_parse_backend(&options->backend, backend_value)) {
                return SWBT_DAEMON_LAUNCH_OPTIONS_ERROR_INVALID_ARGUMENT;
            }
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

    if (options->link_key_db_path != NULL) {
        launch_config->link_key_db_path = options->link_key_db_path;
        launch_config->link_key_db_configured = true;
    }

    launch_config->hci_dump_path = options->hci_dump_path;

    if (!swbt_daemon_config_apply_env(&launch_config->config, env)) {
        *launch_config = (swbt_daemon_launch_config_t){0};
        return false;
    }

    return true;
}
