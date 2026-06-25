#include "daemon/config.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <limits>
#include <string>

#include <toml.hpp>

static bool swbt_toml_find_table(const toml::value &root, const char *key,
                                 const toml::value **out_table) {
    if (out_table == nullptr) {
        return false;
    }
    *out_table = nullptr;
    if (!root.contains(key)) {
        return true;
    }

    const toml::value &table = root.at(key);
    if (!table.is_table()) {
        return false;
    }
    *out_table = &table;
    return true;
}

static bool swbt_toml_key_is_allowed(const toml::value::key_type &key,
                                     const char *const *allowed_keys, std::size_t allowed_count) {
    for (std::size_t index = 0; index < allowed_count; ++index) {
        if (key == allowed_keys[index]) {
            return true;
        }
    }
    return false;
}

static bool swbt_toml_table_contains_only_keys(const toml::value *table,
                                               const char *const *allowed_keys,
                                               std::size_t allowed_count) {
    if (table == nullptr) {
        return true;
    }
    for (const auto &entry : table->as_table()) {
        if (!swbt_toml_key_is_allowed(entry.first, allowed_keys, allowed_count)) {
            return false;
        }
    }
    return true;
}

static bool swbt_toml_root_contains_only_known_sections(const toml::value &root) {
    const char *const allowed_sections[] = {"report", "ipc", "device"};
    for (const auto &entry : root.as_table()) {
        if (!swbt_toml_key_is_allowed(entry.first, allowed_sections,
                                      sizeof(allowed_sections) / sizeof(allowed_sections[0]))) {
            return false;
        }
    }
    return true;
}

static bool swbt_toml_tables_contain_only_known_keys(const toml::value *report,
                                                     const toml::value *ipc,
                                                     const toml::value *device) {
    const char *const report_keys[] = {"period_us"};
    const char *const ipc_keys[] = {"host", "port", "backlog", "heartbeat_timeout_ms"};
    const char *const device_keys[] = {"profile"};

    return swbt_toml_table_contains_only_keys(report, report_keys,
                                              sizeof(report_keys) / sizeof(report_keys[0])) &&
           swbt_toml_table_contains_only_keys(ipc, ipc_keys,
                                              sizeof(ipc_keys) / sizeof(ipc_keys[0])) &&
           swbt_toml_table_contains_only_keys(device, device_keys,
                                              sizeof(device_keys) / sizeof(device_keys[0]));
}

static bool swbt_toml_apply_u32(const toml::value *table, const char *key, uint32_t *out_value) {
    if (table == nullptr || !table->contains(key)) {
        return true;
    }
    const toml::value &value = table->at(key);
    if (!value.is_integer()) {
        return false;
    }
    const toml::value::integer_type parsed = value.as_integer();
    if (parsed < 0 ||
        parsed > static_cast<toml::value::integer_type>(std::numeric_limits<uint32_t>::max())) {
        return false;
    }
    *out_value = static_cast<uint32_t>(parsed);
    return true;
}

static bool swbt_toml_apply_u16(const toml::value *table, const char *key, uint16_t *out_value) {
    uint32_t parsed = 0;
    if (table == nullptr || !table->contains(key)) {
        return true;
    }
    if (!swbt_toml_apply_u32(table, key, &parsed)) {
        return false;
    }
    if (parsed > std::numeric_limits<uint16_t>::max()) {
        return false;
    }
    *out_value = static_cast<uint16_t>(parsed);
    return true;
}

static bool swbt_toml_apply_int(const toml::value *table, const char *key, int *out_value) {
    uint32_t parsed = 0;
    if (table == nullptr || !table->contains(key)) {
        return true;
    }
    if (!swbt_toml_apply_u32(table, key, &parsed)) {
        return false;
    }
    if (parsed > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
        return false;
    }
    *out_value = static_cast<int>(parsed);
    return true;
}

static bool swbt_toml_apply_device_profile(const toml::value *table, swbt_daemon_config_t *config) {
    if (table == nullptr || !table->contains("profile")) {
        return true;
    }
    const toml::value &profile = table->at("profile");
    if (!profile.is_string()) {
        return false;
    }
    const std::string &profile_name = profile.as_string();
    return swbt_daemon_config_apply_device_info_profile(config, profile_name.c_str());
}

static bool swbt_toml_apply_ipc_host(const toml::value *table, swbt_daemon_config_t *config) {
    if (table == nullptr || !table->contains("host")) {
        return true;
    }
    const toml::value &host = table->at("host");
    if (!host.is_string()) {
        return false;
    }
    const std::string &host_name = host.as_string();
    return swbt_daemon_config_set_ipc_host(config, host_name.c_str());
}

static swbt_daemon_config_file_result_t swbt_daemon_config_apply_toml(swbt_daemon_config_t *config,
                                                                      const toml::value &parsed) {
    const toml::value *report = nullptr;
    const toml::value *ipc = nullptr;
    const toml::value *device = nullptr;
    swbt_daemon_config_t next = *config;

    if (!swbt_toml_find_table(parsed, "report", &report) ||
        !swbt_toml_find_table(parsed, "ipc", &ipc) ||
        !swbt_toml_find_table(parsed, "device", &device)) {
        return SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE;
    }
    if (!swbt_toml_root_contains_only_known_sections(parsed) ||
        !swbt_toml_tables_contain_only_known_keys(report, ipc, device)) {
        return SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE;
    }

    if (!swbt_toml_apply_u32(report, "period_us", &next.report_period_us)) {
        return SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE;
    }
    if (!swbt_toml_apply_u16(ipc, "port", &next.ipc_port)) {
        return SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE;
    }
    if (!swbt_toml_apply_ipc_host(ipc, &next)) {
        return SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE;
    }
    if (!swbt_toml_apply_int(ipc, "backlog", &next.ipc_backlog)) {
        return SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE;
    }
    if (!swbt_toml_apply_u32(ipc, "heartbeat_timeout_ms", &next.ipc_heartbeat_timeout_ms)) {
        return SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE;
    }
    if (!swbt_toml_apply_device_profile(device, &next)) {
        return SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE;
    }
    if (next.report_period_us == 0u || next.ipc_backlog <= 0) {
        return SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_VALUE;
    }

    *config = next;
    return SWBT_DAEMON_CONFIG_FILE_OK;
}

extern "C" swbt_daemon_config_file_result_t
swbt_daemon_config_apply_file(swbt_daemon_config_t *config,
                              const swbt_daemon_config_file_source_t *source) {
    FILE *file = nullptr;

    if (config == nullptr || source == nullptr) {
        return SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_ARGUMENT;
    }
    if (source->path == nullptr || source->path[0] == '\0') {
        return source->required ? SWBT_DAEMON_CONFIG_FILE_ERROR_INVALID_ARGUMENT
                                : SWBT_DAEMON_CONFIG_FILE_OK;
    }

    errno = 0;
    file = std::fopen(source->path, "rb");
    if (file == nullptr) {
        if (!source->required && errno == ENOENT) {
            return SWBT_DAEMON_CONFIG_FILE_OK;
        }
        return SWBT_DAEMON_CONFIG_FILE_ERROR_IO;
    }
    (void)std::fclose(file);

    try {
        const auto parsed = toml::parse(source->path, toml::spec::v(1, 0, 0));
        return swbt_daemon_config_apply_toml(config, parsed);
    } catch (const std::exception &) {
        return SWBT_DAEMON_CONFIG_FILE_ERROR_PARSE;
    }
}
