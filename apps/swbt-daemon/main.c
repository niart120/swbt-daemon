#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "btstack_bridge/production_btstack.h"
#include "daemon/config.h"
#include "daemon/production_backend.h"
#include "daemon/runtime.h"

static bool swbt_daemon_env_is_enabled(const char *value) {
    return value != NULL && strcmp(value, "1") == 0;
}

static bool swbt_daemon_parse_u32(const char *value, uint32_t *out_value) {
    char *end = NULL;
    unsigned long parsed;

    if (value == NULL || value[0] == '\0' || out_value == NULL) {
        return false;
    }
    errno = 0;
    parsed = strtoul(value, &end, 10);
    if (errno == ERANGE || end == value || *end != '\0' || parsed > UINT32_MAX) {
        return false;
    }
    *out_value = (uint32_t)parsed;
    return true;
}

static bool swbt_daemon_parse_u16(const char *value, uint16_t *out_value) {
    uint32_t parsed;
    if (!swbt_daemon_parse_u32(value, &parsed) || parsed > UINT16_MAX) {
        return false;
    }
    *out_value = (uint16_t)parsed;
    return true;
}

static bool swbt_daemon_parse_int(const char *value, int *out_value) {
    uint32_t parsed;
    if (!swbt_daemon_parse_u32(value, &parsed) || parsed > (uint32_t)INT32_MAX) {
        return false;
    }
    *out_value = (int)parsed;
    return true;
}

static bool swbt_daemon_apply_env_config(swbt_daemon_config_t *config) {
    const char *report_period = getenv("SWBT_REPORT_PERIOD_US");
    const char *ipc_host = getenv("SWBT_IPC_HOST");
    const char *ipc_port = getenv("SWBT_IPC_PORT");
    const char *ipc_backlog = getenv("SWBT_IPC_BACKLOG");

    if (config == NULL) {
        return false;
    }
    if (report_period != NULL && !swbt_daemon_parse_u32(report_period, &config->report_period_us)) {
        return false;
    }
    if (ipc_host != NULL && ipc_host[0] != '\0') {
        config->ipc_host = ipc_host;
    }
    if (ipc_port != NULL && !swbt_daemon_parse_u16(ipc_port, &config->ipc_port)) {
        return false;
    }
    if (ipc_backlog != NULL && !swbt_daemon_parse_int(ipc_backlog, &config->ipc_backlog)) {
        return false;
    }
    return config->report_period_us != 0u && config->ipc_backlog > 0;
}

static int swbt_daemon_run_production(const swbt_daemon_config_t *config) {
    swbt_daemon_production_backend_t backend;
    const swbt_daemon_hardware_approval_t approval = {
        .run_hardware = swbt_daemon_env_is_enabled(getenv("SWBT_RUN_HARDWARE")),
        .hardware_approved = swbt_daemon_env_is_enabled(getenv("SWBT_HARDWARE_APPROVED")),
    };

    if (swbt_daemon_production_backend_init(&backend, config, swbt_btstack_production_backend_ops(),
                                            NULL) != SWBT_DAEMON_PRODUCTION_OK) {
        return 1;
    }
    return swbt_daemon_production_main_with_backend(&backend, &approval) ==
                   SWBT_DAEMON_PRODUCTION_OK
               ? 0
               : 1;
}

int main(void) {
    swbt_daemon_config_t config = swbt_daemon_config_default();
    const char *backend = getenv("SWBT_DAEMON_BACKEND");

    if (!swbt_daemon_apply_env_config(&config)) {
        return 1;
    }
    if (backend != NULL && strcmp(backend, "production") == 0) {
        return swbt_daemon_run_production(&config);
    }
    return swbt_daemon_main_with_backend(&config, swbt_daemon_runtime_noop_backend(), NULL);
}
