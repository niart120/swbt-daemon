#include "daemon/config.h"

#include <cerrno>
#include <cstdio>

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
    return SWBT_DAEMON_CONFIG_FILE_ERROR_PARSE;
}
