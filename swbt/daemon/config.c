#include "daemon/config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

bool swbt_daemon_config_set_ipc_host(swbt_daemon_config_t *config, const char *host) {
    size_t index = 0;

    if (config == NULL || host == NULL || host[0] == '\0') {
        return false;
    }
    for (; index + 1u < SWBT_DAEMON_CONFIG_IPC_HOST_SIZE && host[index] != '\0'; ++index) {
        config->ipc_host[index] = host[index];
    }
    config->ipc_host[index] = '\0';
    return host[index] == '\0';
}

static bool swbt_daemon_config_hex_digit_to_upper(char value, char *out_value) {
    if (out_value == NULL) {
        return false;
    }
    if (value >= '0' && value <= '9') {
        *out_value = value;
        return true;
    }
    if (value >= 'a' && value <= 'f') {
        *out_value = (char)(value - 'a' + 'A');
        return true;
    }
    if (value >= 'A' && value <= 'F') {
        *out_value = value;
        return true;
    }
    return false;
}

static bool swbt_daemon_config_set_switch_address(char dest[SWBT_DAEMON_CONFIG_SWITCH_ADDRESS_SIZE],
                                                  const char *address) {
    if (dest == NULL || address == NULL) {
        return false;
    }
    for (size_t index = 0u; index + 1u < SWBT_DAEMON_CONFIG_SWITCH_ADDRESS_SIZE; ++index) {
        if (address[index] == '\0') {
            return false;
        }
        if ((index + 1u) % 3u == 0u) {
            if (address[index] != ':') {
                return false;
            }
            dest[index] = ':';
        } else if (!swbt_daemon_config_hex_digit_to_upper(address[index], &dest[index])) {
            return false;
        }
    }
    if (address[SWBT_DAEMON_CONFIG_SWITCH_ADDRESS_SIZE - 1u] != '\0') {
        return false;
    }
    dest[SWBT_DAEMON_CONFIG_SWITCH_ADDRESS_SIZE - 1u] = '\0';
    return true;
}

bool swbt_daemon_config_set_active_reconnect_explicit_switch_address(swbt_daemon_config_t *config,
                                                                     const char *address) {
    if (config == NULL) {
        return false;
    }
    return swbt_daemon_config_set_switch_address(config->active_reconnect_explicit_switch_address,
                                                 address);
}

bool swbt_daemon_config_set_active_reconnect_learned_switch_address(swbt_daemon_config_t *config,
                                                                    const char *address) {
    if (config == NULL) {
        return false;
    }
    return swbt_daemon_config_set_switch_address(config->active_reconnect_learned_switch_address,
                                                 address);
}

const char *
swbt_daemon_config_effective_reconnect_switch_address(const swbt_daemon_config_t *config) {
    if (config == NULL) {
        return NULL;
    }
    if (config->active_reconnect_explicit_switch_address[0] != '\0') {
        return config->active_reconnect_explicit_switch_address;
    }
    if (config->active_reconnect_learned_switch_address[0] != '\0') {
        return config->active_reconnect_learned_switch_address;
    }
    return NULL;
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

swbt_daemon_config_t swbt_daemon_config_default(void) {
    swbt_daemon_config_t config = {
        .report_period_us = SWBT_DAEMON_DEFAULT_REPORT_PERIOD_US,
        .ipc_host = SWBT_DAEMON_DEFAULT_IPC_HOST,
        .ipc_port = SWBT_DAEMON_DEFAULT_IPC_PORT,
        .ipc_backlog = SWBT_DAEMON_DEFAULT_IPC_BACKLOG,
        .ipc_heartbeat_timeout_ms = SWBT_DAEMON_DEFAULT_IPC_HEARTBEAT_TIMEOUT_MS,
        .report_options =
            {
                .battery_connection = 0x8Eu,
                .vibrator_report = 0x80u,
            },
    };
    config.device_info = swbt_switch_device_info_default();
    return config;
}

bool swbt_daemon_config_apply_env(swbt_daemon_config_t *config,
                                  const swbt_daemon_config_env_t *env) {
    swbt_daemon_config_t next;

    if (config == NULL || env == NULL) {
        return false;
    }
    next = *config;
    if (env->report_period_us != NULL &&
        !swbt_daemon_parse_u32(env->report_period_us, &next.report_period_us)) {
        return false;
    }
    if (env->ipc_host != NULL && env->ipc_host[0] != '\0' &&
        !swbt_daemon_config_set_ipc_host(&next, env->ipc_host)) {
        return false;
    }
    if (env->ipc_port != NULL && !swbt_daemon_parse_u16(env->ipc_port, &next.ipc_port)) {
        return false;
    }
    if (env->ipc_backlog != NULL && !swbt_daemon_parse_int(env->ipc_backlog, &next.ipc_backlog)) {
        return false;
    }
    if (env->ipc_heartbeat_timeout_ms != NULL &&
        !swbt_daemon_parse_u32(env->ipc_heartbeat_timeout_ms, &next.ipc_heartbeat_timeout_ms)) {
        return false;
    }
    if (!swbt_daemon_config_apply_device_info_profile(&next, env->device_info_profile)) {
        return false;
    }
    if (next.report_period_us == 0u || next.ipc_backlog <= 0) {
        return false;
    }
    *config = next;
    return true;
}

bool swbt_daemon_config_apply_device_info_profile(swbt_daemon_config_t *config,
                                                  const char *profile) {
    if (config == NULL) {
        return false;
    }
    if (profile == NULL || profile[0] == '\0') {
        return true;
    }
    if (strcmp(profile, "swbt-pro") == 0 || strcmp(profile, "default") == 0) {
        config->device_info = swbt_switch_device_info_swbt_pro();
        return true;
    }
    return false;
}
