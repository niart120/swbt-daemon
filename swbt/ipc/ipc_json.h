#ifndef SWBT_IPC_JSON_H
#define SWBT_IPC_JSON_H

#include <stddef.h>
#include <stdint.h>

#include "ipc/ipc_session.h"

enum {
    SWBT_IPC_JSON_LINE_MAX = 8192,
    SWBT_IPC_JSON_RESPONSE_MAX = 1024,
};

typedef enum {
    SWBT_IPC_JSON_OK = 0,
    SWBT_IPC_JSON_ERROR_INVALID_ARGUMENT = -1,
    SWBT_IPC_JSON_ERROR_RESPONSE_TOO_SMALL = -2,
} swbt_ipc_json_result_t;

swbt_ipc_json_result_t swbt_ipc_json_handle_line(swbt_ipc_session_t *session, uint32_t client_id,
                                                 const char *line, char *response,
                                                 size_t response_size);

#endif
