#ifndef SWBT_IPC_ADAPTER_H
#define SWBT_IPC_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

#include "control/control.h"
#include "ipc/ipc_json.h"
#include "ipc/ipc_status.h"

typedef enum {
    SWBT_IPC_OK = 0,
    SWBT_IPC_ERROR_INVALID_ARGUMENT = -1,
    SWBT_IPC_ERROR_OWNER_BUSY = -2,
    SWBT_IPC_ERROR_NOT_OWNER = -3,
} swbt_ipc_result_t;

swbt_ipc_json_result_t swbt_ipc_adapter_handle_line(swbt_control_t *control, uint32_t client_id,
                                                    const char *line, char *response,
                                                    size_t response_size);

swbt_ipc_result_t swbt_ipc_adapter_handle_disconnect(swbt_control_t *control, uint32_t client_id);

swbt_ipc_result_t swbt_ipc_adapter_handle_heartbeat_timeout(swbt_control_t *control,
                                                            uint32_t client_id);

swbt_ipc_result_t swbt_ipc_adapter_handle_shutdown(swbt_control_t *control);

swbt_ipc_result_t swbt_ipc_adapter_get_status(const swbt_control_t *control,
                                              swbt_ipc_status_t *out_status);

#endif
