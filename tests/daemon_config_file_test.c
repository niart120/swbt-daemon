#include "daemon/config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int expect_eq_int(int actual, int expected, const char *label) {
    if (actual == expected) {
        return 0;
    }
    fprintf(stderr, "%s: expected %d, got %d\n", label, expected, actual);
    return 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected, const char *label) {
    if (actual == expected) {
        return 0;
    }
    fprintf(stderr, "%s: expected %u, got %u\n", label, (unsigned)expected, (unsigned)actual);
    return 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected, const char *label) {
    if (actual == expected) {
        return 0;
    }
    fprintf(stderr, "%s: expected %u, got %u\n", label, (unsigned)expected, (unsigned)actual);
    return 1;
}

static int expect_str_eq(const char *actual, const char *expected, const char *label) {
    if (actual != NULL && expected != NULL && strcmp(actual, expected) == 0) {
        return 0;
    }
    fprintf(stderr, "%s: expected %s, got %s\n", label, expected == NULL ? "(null)" : expected,
            actual == NULL ? "(null)" : actual);
    return 1;
}

static int expect_default_runtime_config(const swbt_daemon_config_t *config) {
    int failed = 0;
    failed += expect_eq_u32(config->report_period_us, SWBT_DAEMON_DEFAULT_REPORT_PERIOD_US,
                            "report period");
    failed += expect_str_eq(config->ipc_host, SWBT_DAEMON_DEFAULT_IPC_HOST, "ipc host");
    failed += expect_eq_u16(config->ipc_port, SWBT_DAEMON_DEFAULT_IPC_PORT, "ipc port");
    failed += expect_eq_int(config->ipc_backlog, SWBT_DAEMON_DEFAULT_IPC_BACKLOG, "ipc backlog");
    failed += expect_eq_u32(config->ipc_heartbeat_timeout_ms,
                            SWBT_DAEMON_DEFAULT_IPC_HEARTBEAT_TIMEOUT_MS, "heartbeat timeout");
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

static int write_ipc_host_config(const char *path, const char *host) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    if (fprintf(file, "[ipc]\nhost = \"%s\"\n", host) < 0 || fclose(file) != 0) {
        (void)remove(path);
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
    if (write_ipc_host_config(first_path, "0.0.0.0") != 0 ||
        write_ipc_host_config(second_path, "127.0.0.1") != 0) {
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
    failed += expect_str_eq(config.ipc_host, "0.0.0.0", "first ipc host");

    const swbt_daemon_config_t copied = config;
    failed += expect_eq_int(swbt_daemon_config_apply_file(&config, &second_source),
                            SWBT_DAEMON_CONFIG_FILE_OK, "apply second file");
    failed += expect_str_eq(config.ipc_host, "127.0.0.1", "second ipc host");
    failed += expect_str_eq(copied.ipc_host, "0.0.0.0", "copied ipc host");

    failed += remove(first_path) == 0 ? 0 : 1;
    failed += remove(second_path) == 0 ? 0 : 1;
    return failed;
}

int main(void) {
    int failed = 0;
    failed += missing_optional_config_file_keeps_defaults();
    failed += empty_toml_config_file_keeps_defaults();
    failed += valid_toml_config_file_applies_runtime_values();
    failed += ipc_host_from_toml_config_is_owned_by_config_value();
    return failed == 0 ? 0 : 1;
}
