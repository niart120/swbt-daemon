#ifndef SWBT_IPC_JSON_H
#define SWBT_IPC_JSON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "support/metrics.h"
#include "ipc/ipc_status.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_rumble.h"

enum {
    SWBT_IPC_JSON_LINE_MAX = 8192,
    SWBT_IPC_JSON_RESPONSE_MAX = 2048,
    SWBT_IPC_JSON_STRING_MAX = 96,
};

typedef enum {
    SWBT_IPC_JSON_OK = 0,
    SWBT_IPC_JSON_ERROR_INVALID_ARGUMENT = -1,
    SWBT_IPC_JSON_ERROR_RESPONSE_TOO_SMALL = -2,
} swbt_ipc_json_result_t;

typedef enum {
    SWBT_IPC_COMMAND_NONE = 0,
    SWBT_IPC_COMMAND_HELLO,
    SWBT_IPC_COMMAND_ACQUIRE,
    SWBT_IPC_COMMAND_RELEASE,
    SWBT_IPC_COMMAND_SET_STATE,
    SWBT_IPC_COMMAND_GET_STATUS,
} swbt_ipc_command_type_t;

typedef enum {
    SWBT_IPC_RESPONSE_NONE = 0,
    SWBT_IPC_RESPONSE_HELLO_OK,
    SWBT_IPC_RESPONSE_ACQUIRED,
    SWBT_IPC_RESPONSE_RELEASED,
    SWBT_IPC_RESPONSE_STATE_ACCEPTED,
    SWBT_IPC_RESPONSE_STATUS,
    SWBT_IPC_RESPONSE_ERROR,
} swbt_ipc_response_type_t;

typedef enum {
    SWBT_IPC_ERROR_CODE_INVALID_JSON = 0,
    SWBT_IPC_ERROR_CODE_INVALID_VERSION,
    SWBT_IPC_ERROR_CODE_UNSUPPORTED_COMMAND,
    SWBT_IPC_ERROR_CODE_OWNER_BUSY,
    SWBT_IPC_ERROR_CODE_NOT_OWNER,
    SWBT_IPC_ERROR_CODE_INVALID_STATE,
    SWBT_IPC_ERROR_CODE_INTERNAL_ERROR,
} swbt_ipc_error_code_t;

typedef struct {
    swbt_ipc_command_type_t type;
    bool has_request_id;
    char request_id[SWBT_IPC_JSON_STRING_MAX];
    bool has_owner_id;
    uint32_t owner_client_id;
    uint64_t sequence;
    swbt_state_t state;
} swbt_ipc_command_t;

typedef struct {
    bool has_owner;
    uint32_t owner_client_id;
    uint64_t last_sequence;
    swbt_state_t state;
    swbt_switch_rumble_state_t rumble;
    swbt_metrics_snapshot_t metrics;
    swbt_ipc_daemon_status_t daemon;
    swbt_ipc_hardware_status_t hardware;
} swbt_ipc_response_status_t;

typedef struct {
    swbt_ipc_response_type_t type;
    bool has_request_id;
    char request_id[SWBT_IPC_JSON_STRING_MAX];
    uint32_t client_id;
    uint32_t owner_client_id;
    uint64_t sequence;
    swbt_ipc_response_status_t status;
    swbt_ipc_error_code_t error_code;
    char error_message[SWBT_IPC_JSON_STRING_MAX];
} swbt_ipc_response_t;

swbt_ipc_json_result_t swbt_ipc_json_decode_command(const char *line,
                                                    swbt_ipc_command_t *out_command,
                                                    swbt_ipc_response_t *out_error_response);

swbt_ipc_json_result_t swbt_ipc_json_encode_response(const swbt_ipc_response_t *typed_response,
                                                     char *response, size_t response_size);

#endif
