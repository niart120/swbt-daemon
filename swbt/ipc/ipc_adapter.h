#ifndef SWBT_IPC_ADAPTER_H
#define SWBT_IPC_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

#include "ipc/ipc_json.h"
#include "ipc/ipc_session.h"

swbt_ipc_json_result_t swbt_ipc_adapter_handle_line(swbt_ipc_session_t *session, uint32_t client_id,
                                                    const char *line, char *response,
                                                    size_t response_size);

swbt_ipc_result_t swbt_ipc_adapter_handle_disconnect(swbt_ipc_session_t *session,
                                                     uint32_t client_id);

swbt_ipc_result_t swbt_ipc_adapter_handle_heartbeat_timeout(swbt_ipc_session_t *session,
                                                            uint32_t client_id);

swbt_ipc_result_t swbt_ipc_adapter_handle_shutdown(swbt_ipc_session_t *session);

#endif
