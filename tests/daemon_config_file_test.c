#include "daemon/config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int expect_eq_int(int actual, int expected, const char *label) {
    (void)label;
    if (actual == expected) {
        return 0;
    }
    return 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected, const char *label) {
    (void)label;
    if (actual == expected) {
        return 0;
    }
    return 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected, const char *label) {
    (void)label;
    if (actual == expected) {
        return 0;
    }
    return 1;
}

static int expect_str_eq(const char *actual, const char *expected) {
    if (actual != NULL && expected != NULL && strcmp(actual, expected) == 0) {
        return 0;
    }
    return 1;
}

static int expect_null(const void *actual) {
    return actual == NULL ? 0 : 1;
}

static int expect_default_runtime_config(const swbt_daemon_config_t *config) {
    int failed = 0;
    failed += expect_eq_u32(config->report_period_us, SWBT_DAEMON_DEFAULT_REPORT_PERIOD_US,
                            "report period");
    failed += expect_str_eq(config->ipc_host, SWBT_DAEMON_DEFAULT_IPC_HOST);
    failed += expect_eq_u16(config->ipc_port, SWBT_DAEMON_DEFAULT_IPC_PORT, "ipc port");
    failed += expect_eq_int(config->ipc_backlog, SWBT_DAEMON_DEFAULT_IPC_BACKLOG, "ipc backlog");
    failed += expect_eq_u32(config->ipc_heartbeat_timeout_ms,
                            SWBT_DAEMON_DEFAULT_IPC_HEARTBEAT_TIMEOUT_MS, "heartbeat timeout");
    failed += expect_str_eq(config->active_reconnect_explicit_switch_address, "");
    failed += expect_str_eq(config->active_reconnect_learned_switch_address, "");
    failed += expect_null(swbt_daemon_config_effective_reconnect_switch_address(config));
    return failed;
}

static int expect_runtime_config_eq(const swbt_daemon_config_t *actual,
                                    const swbt_daemon_config_t *expected) {
    int failed = 0;
    failed += expect_eq_u32(actual->report_period_us, expected->report_period_us, "report period");
    failed += expect_str_eq(actual->ipc_host, expected->ipc_host);
    failed += expect_eq_u16(actual->ipc_port, expected->ipc_port, "ipc port");
    failed += expect_eq_int(actual->ipc_backlog, expected->ipc_backlog, "ipc backlog");
    failed += expect_eq_u32(actual->ipc_heartbeat_timeout_ms, expected->ipc_heartbeat_timeout_ms,
                            "heartbeat timeout");
    return failed;
}

static int missing_optional_config_file_keeps_defaults(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = "tmp/test-data/does-not-exist/swbt-daemon.toml",
        .required = false,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "apply file");
    failed += expect_default_runtime_config(&config);
    return failed;
}

static int empty_toml_config_file_keeps_defaults(void) {
    const char *path = "daemon-config-empty.toml";
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "apply file");
    failed += expect_default_runtime_config(&config);
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

typedef struct {
    const char *path;
    const char *host;
} ipc_host_config_file_t;

static int write_ipc_host_config(ipc_host_config_file_t config) {
    FILE *file = fopen(config.path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs("[ipc]\nhost = \"", file) < 0 || fputs(config.host, file) < 0 ||
        fputs("\"\n", file) < 0 || fclose(file) != 0) {
        (void)remove(config.path);
        return 1;
    }
    return 0;
}

static int valid_toml_config_file_applies_runtime_values(void) {
    const char *path = "daemon-config-valid-runtime.toml";
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs("[report]\n"
              "period_us = 16667\n"
              "\n"
              "[ipc]\n"
              "port = 37637\n"
              "backlog = 4\n"
              "heartbeat_timeout_ms = 2500\n"
              "\n"
              "[device]\n"
              "profile = \"swbt-pro\"\n",
              file) < 0 ||
        fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "apply file");
    failed += expect_eq_u32(config.report_period_us, 16667u, "report period");
    failed += expect_eq_u16(config.ipc_port, 37637u, "ipc port");
    failed += expect_eq_int(config.ipc_backlog, 4, "ipc backlog");
    failed += expect_eq_u32(config.ipc_heartbeat_timeout_ms, 2500u, "heartbeat timeout");
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int ipc_host_from_toml_config_is_owned_by_config_value(void) {
    const char *first_path = "daemon-config-ipc-host-first.toml";
    const char *second_path = "daemon-config-ipc-host-second.toml";
    if (write_ipc_host_config((ipc_host_config_file_t){.path = first_path, .host = "0.0.0.0"}) !=
            0 ||
        write_ipc_host_config((ipc_host_config_file_t){.path = second_path, .host = "127.0.0.1"}) !=
            0) {
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t first_source = {
        .path = first_path,
        .required = true,
    };
    const swbt_daemon_config_file_source_t second_source = {
        .path = second_path,
        .required = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &first_source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "apply first file");
    failed += expect_str_eq(config.ipc_host, "0.0.0.0");

    const swbt_daemon_config_t copied = config;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &second_source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "apply second file");
    failed += expect_str_eq(config.ipc_host, "127.0.0.1");
    failed += expect_str_eq(copied.ipc_host, "0.0.0.0");

    failed += remove(first_path) == 0 ? 0 : 1;
    failed += remove(second_path) == 0 ? 0 : 1;
    return failed;
}

static int active_reconnect_addresses_from_toml_are_stored_separately(void) {
    const char *path = "daemon-config-active-reconnect-address.toml";
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs("[active_reconnect]\n"
              "switch_address = \"01:23:45:67:89:ab\"\n"
              "\n"
              "[active_reconnect.learned]\n"
              "switch_address = \"fe:dc:ba:98:76:54\"\n",
              file) < 0 ||
        fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "apply file");
    failed += expect_str_eq(config.active_reconnect_explicit_switch_address, "01:23:45:67:89:AB");
    failed += expect_str_eq(config.active_reconnect_learned_switch_address, "FE:DC:BA:98:76:54");
    failed += expect_str_eq(swbt_daemon_config_effective_reconnect_switch_address(&config),
                            "01:23:45:67:89:AB");
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int active_reconnect_learned_address_is_effective_without_explicit(void) {
    const char *path = "daemon-config-active-reconnect-learned-address.toml";
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs("[active_reconnect.learned]\n"
              "switch_address = \"fe:dc:ba:98:76:54\"\n",
              file) < 0 ||
        fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "apply file");
    failed += expect_str_eq(config.active_reconnect_explicit_switch_address, "");
    failed += expect_str_eq(config.active_reconnect_learned_switch_address, "FE:DC:BA:98:76:54");
    failed += expect_str_eq(swbt_daemon_config_effective_reconnect_switch_address(&config),
                            "FE:DC:BA:98:76:54");
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int learned_active_reconnect_address_is_written_without_overwriting_explicit(void) {
    const char *path = "daemon-config-save-active-reconnect-learned.toml";
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs("[active_reconnect]\n"
              "switch_address = \"01:23:45:67:89:ab\"\n",
              file) < 0 ||
        fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };
    const swbt_daemon_config_file_target_t target = {
        .path = path,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "apply file");
    failed += expect_eq_int(swbt_daemon_config_save_active_reconnect_learned_switch_address(
                                &config, &target, "fe:dc:ba:98:76:54"),
                            SWBT_DAEMON_CONFIG_FILE_OK, "save learned address");
    failed += expect_str_eq(config.active_reconnect_explicit_switch_address, "01:23:45:67:89:AB");
    failed += expect_str_eq(config.active_reconnect_learned_switch_address, "FE:DC:BA:98:76:54");
    failed += expect_str_eq(swbt_daemon_config_effective_reconnect_switch_address(&config),
                            "01:23:45:67:89:AB");

    swbt_daemon_config_t reloaded = swbt_daemon_config_default();
    failed += expect_eq_int(swbt_daemon_config_apply_file(&reloaded, &source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "reload file");
    failed += expect_str_eq(reloaded.active_reconnect_explicit_switch_address, "01:23:45:67:89:AB");
    failed += expect_str_eq(reloaded.active_reconnect_learned_switch_address, "FE:DC:BA:98:76:54");
    failed += expect_str_eq(swbt_daemon_config_effective_reconnect_switch_address(&reloaded),
                            "01:23:45:67:89:AB");
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int learned_active_reconnect_address_save_creates_config_file(void) {
    const char *path = "daemon-config-create-active-reconnect-learned.toml";
    (void)remove(path);

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_target_t target = {
        .path = path,
    };
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_save_active_reconnect_learned_switch_address(
                                &config, &target, "fe:dc:ba:98:76:54"),
                            SWBT_DAEMON_CONFIG_FILE_OK, "save learned address");
    failed += expect_str_eq(config.active_reconnect_explicit_switch_address, "");
    failed += expect_str_eq(config.active_reconnect_learned_switch_address, "FE:DC:BA:98:76:54");

    swbt_daemon_config_t reloaded = swbt_daemon_config_default();
    failed += expect_eq_int(swbt_daemon_config_apply_file(&reloaded, &source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "reload file");
    failed += expect_str_eq(reloaded.active_reconnect_explicit_switch_address, "");
    failed += expect_str_eq(reloaded.active_reconnect_learned_switch_address, "FE:DC:BA:98:76:54");
    failed += expect_str_eq(swbt_daemon_config_effective_reconnect_switch_address(&reloaded),
                            "FE:DC:BA:98:76:54");
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int invalid_active_reconnect_address_rejects_and_preserves_config(void) {
    const char *path = "daemon-config-invalid-active-reconnect-address.toml";
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs("[report]\n"
              "period_us = 16667\n"
              "\n"
              "[active_reconnect]\n"
              "switch_address = \"not-an-address\"\n",
              file) < 0 ||
        fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE, "apply file");
    failed += expect_default_runtime_config(&config);
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int invalid_active_reconnect_learned_address_rejects_and_preserves_config(void) {
    const char *path = "daemon-config-invalid-active-reconnect-learned-address.toml";
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs("[report]\n"
              "period_us = 16667\n"
              "\n"
              "[active_reconnect.learned]\n"
              "switch_address = \"not-an-address\"\n",
              file) < 0 ||
        fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE, "apply file");
    failed += expect_default_runtime_config(&config);
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int unknown_toml_config_key_rejects_and_preserves_config(void) {
    const char *path = "daemon-config-unknown-key.toml";
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs("[report]\n"
              "period_us = 16667\n"
              "unexpected = true\n",
              file) < 0 ||
        fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE, "apply file");
    failed += expect_default_runtime_config(&config);
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int env_override_wins_over_config_file_value(void) {
    const char *path = "daemon-config-env-override.toml";
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs("[report]\n"
              "period_us = 16667\n"
              "\n"
              "[ipc]\n"
              "host = \"0.0.0.0\"\n"
              "port = 37637\n"
              "backlog = 4\n"
              "heartbeat_timeout_ms = 2500\n",
              file) < 0 ||
        fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };
    const swbt_daemon_config_env_t env = {
        .report_period_us = "3333",
        .ipc_host = "127.0.0.1",
        .ipc_port = "44123",
        .ipc_backlog = "8",
        .ipc_heartbeat_timeout_ms = "1250",
        .device_info_profile = NULL,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "apply file");
    failed += expect_eq_int(swbt_daemon_config_apply_env(&config, &env), true, "apply env");
    failed += expect_eq_u32(config.report_period_us, 3333u, "report period");
    failed += expect_str_eq(config.ipc_host, "127.0.0.1");
    failed += expect_eq_u16(config.ipc_port, 44123u, "ipc port");
    failed += expect_eq_int(config.ipc_backlog, 8, "ipc backlog");
    failed += expect_eq_u32(config.ipc_heartbeat_timeout_ms, 1250u, "heartbeat timeout");
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int invalid_toml_config_value_rejects_and_preserves_config(void) {
    const char *path = "daemon-config-invalid-value.toml";
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs("[report]\n"
              "period_us = 16667\n"
              "\n"
              "[ipc]\n"
              "host = \"0.0.0.0\"\n"
              "port = 37637\n"
              "backlog = 4\n"
              "heartbeat_timeout_ms = 2500\n"
              "\n"
              "[device]\n"
              "profile = \"unknown\"\n",
              file) < 0 ||
        fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE, "apply file");
    failed += expect_default_runtime_config(&config);
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

static int invalid_env_override_after_config_file_rejects_and_preserves_config(void) {
    const char *path = "daemon-config-invalid-env-after-file.toml";
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fputs("[report]\n"
              "period_us = 16667\n"
              "\n"
              "[ipc]\n"
              "host = \"0.0.0.0\"\n"
              "port = 37637\n"
              "backlog = 4\n"
              "heartbeat_timeout_ms = 2500\n",
              file) < 0 ||
        fclose(file) != 0) {
        (void)remove(path);
        return 1;
    }

    swbt_daemon_config_t config = swbt_daemon_config_default();
    const swbt_daemon_config_file_source_t source = {
        .path = path,
        .required = true,
    };
    const swbt_daemon_config_env_t env = {
        .report_period_us = "0",
        .ipc_host = "127.0.0.1",
        .ipc_port = "44123",
        .ipc_backlog = "8",
        .ipc_heartbeat_timeout_ms = "1250",
        .device_info_profile = NULL,
    };

    int failed = 0;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "apply file");
    const swbt_daemon_config_t expected = config;
    failed += expect_eq_int(swbt_daemon_config_apply_env(&config, &env), false, "apply env");
    failed += expect_runtime_config_eq(&config, &expected);
    failed += remove(path) == 0 ? 0 : 1;
    return failed;
}

int main(void) {
    int failed = 0;
    failed += missing_optional_config_file_keeps_defaults();
    failed += empty_toml_config_file_keeps_defaults();
    failed += valid_toml_config_file_applies_runtime_values();
    failed += ipc_host_from_toml_config_is_owned_by_config_value();
    failed += active_reconnect_addresses_from_toml_are_stored_separately();
    failed += active_reconnect_learned_address_is_effective_without_explicit();
    failed += learned_active_reconnect_address_is_written_without_overwriting_explicit();
    failed += learned_active_reconnect_address_save_creates_config_file();
    failed += invalid_active_reconnect_address_rejects_and_preserves_config();
    failed += invalid_active_reconnect_learned_address_rejects_and_preserves_config();
    failed += unknown_toml_config_key_rejects_and_preserves_config();
    failed += env_override_wins_over_config_file_value();
    failed += invalid_toml_config_value_rejects_and_preserves_config();
    failed += invalid_env_override_after_config_file_rejects_and_preserves_config();
    return failed == 0 ? 0 : 1;
}
