#include "daemon/config.h"

#include <stdint.h>
#include <string.h>

static int expect_eq_int(int actual, int expected, const char *label) {
    (void)label;
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected, const char *label) {
    (void)label;
    return actual == expected ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected, const char *label) {
    (void)label;
    return actual == expected ? 0 : 1;
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
    failed += expect_eq_u32(config.report_period_us, SWBT_DAEMON_DEFAULT_REPORT_PERIOD_US,
                            "report period");
    failed += config.ipc_host != NULL && strcmp(config.ipc_host, SWBT_DAEMON_DEFAULT_IPC_HOST) == 0
                  ? 0
                  : 1;
    failed += expect_eq_u16(config.ipc_port, SWBT_DAEMON_DEFAULT_IPC_PORT, "ipc port");
    failed += expect_eq_int(config.ipc_backlog, SWBT_DAEMON_DEFAULT_IPC_BACKLOG, "ipc backlog");
    failed += expect_eq_u32(config.ipc_heartbeat_timeout_ms,
                            SWBT_DAEMON_DEFAULT_IPC_HEARTBEAT_TIMEOUT_MS, "heartbeat timeout");
    return failed;
}

int main(void) {
    int failed = 0;
    failed += missing_optional_config_file_keeps_defaults();
    return failed == 0 ? 0 : 1;
}
