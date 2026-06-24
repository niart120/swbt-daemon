#include "ipc/ipc_adapter.h"

static swbt_ipc_result_t swbt_ipc_map_app_result(swbt_app_result_t result) {
    switch (result) {
    case SWBT_APP_OK:
    case SWBT_APP_ERROR_STALE_SEQUENCE:
        return SWBT_IPC_OK;
    case SWBT_APP_ERROR_OWNER_BUSY:
        return SWBT_IPC_ERROR_OWNER_BUSY;
    case SWBT_APP_ERROR_NOT_OWNER:
        return SWBT_IPC_ERROR_NOT_OWNER;
    case SWBT_APP_ERROR_INVALID_ARGUMENT:
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    return SWBT_IPC_ERROR_INVALID_ARGUMENT;
}

static void swbt_ipc_adapter_init_response(swbt_ipc_response_t *response) {
    swbt_switch_rumble_state_t rumble = {0};
    swbt_metrics_snapshot_t metrics = {0};
    swbt_ipc_daemon_status_t daemon = {0};
    swbt_ipc_hardware_status_t hardware = {0};

    response->type = SWBT_IPC_RESPONSE_NONE;
    response->has_request_id = false;
    response->request_id[0] = '\0';
    response->client_id = 0;
    response->owner_client_id = 0;
    response->sequence = 0;
    metrics.hardware_status = SWBT_METRICS_HARDWARE_UNAVAILABLE;
    daemon.hardware_approval = SWBT_IPC_HARDWARE_APPROVAL_UNAVAILABLE;
    response->status = (swbt_ipc_response_status_t){
        .has_owner = false,
        .owner_client_id = 0,
        .last_sequence = 0,
        .state = swbt_state_neutral(),
        .rumble = rumble,
        .metrics = metrics,
        .daemon = daemon,
        .hardware = hardware,
    };
    response->error_code = SWBT_IPC_ERROR_CODE_INVALID_JSON;
    response->error_message[0] = '\0';
}

static void swbt_ipc_adapter_copy_text(char *out, size_t out_size, const char *text) {
    size_t index = 0;

    if (out_size == 0) {
        return;
    }
    for (; index + 1u < out_size && text[index] != '\0'; ++index) {
        out[index] = text[index];
    }
    out[index] = '\0';
}

static void swbt_ipc_adapter_set_error(swbt_ipc_response_t *response, bool has_request_id,
                                       const char *request_id, swbt_ipc_error_code_t error_code,
                                       const char *message) {
    response->type = SWBT_IPC_RESPONSE_ERROR;
    response->has_request_id = has_request_id;
    if (has_request_id) {
        swbt_ipc_adapter_copy_text(response->request_id, sizeof(response->request_id), request_id);
    } else {
        response->request_id[0] = '\0';
    }
    response->error_code = error_code;
    swbt_ipc_adapter_copy_text(response->error_message, sizeof(response->error_message), message);
}

static void swbt_ipc_adapter_copy_command_request(const swbt_ipc_command_t *command,
                                                  swbt_ipc_response_t *response) {
    response->has_request_id = command->has_request_id;
    if (command->has_request_id) {
        swbt_ipc_adapter_copy_text(response->request_id, sizeof(response->request_id),
                                   command->request_id);
    } else {
        response->request_id[0] = '\0';
    }
}

static bool swbt_ipc_adapter_command_owner_matches_client(const swbt_ipc_command_t *command,
                                                          uint32_t client_id) {
    return command->has_owner_id && command->owner_client_id == client_id;
}

static void swbt_ipc_adapter_copy_status(const swbt_ipc_status_t *status,
                                         swbt_ipc_response_status_t *out_status) {
    out_status->has_owner = status->has_owner;
    out_status->owner_client_id = status->owner_client_id;
    out_status->last_sequence = status->last_seq;
    out_status->state = status->state;
    out_status->rumble = status->rumble;
    out_status->metrics = status->metrics;
    out_status->daemon = status->daemon;
    out_status->hardware = status->hardware;
}

static void swbt_ipc_adapter_copy_snapshot(const swbt_app_snapshot_t *snapshot,
                                           swbt_ipc_status_t *out_status) {
    out_status->has_owner = snapshot->has_owner;
    out_status->owner_client_id = snapshot->owner_client_id;
    out_status->last_seq = snapshot->last_sequence;
    out_status->state = snapshot->state;
    out_status->rumble = snapshot->rumble;
    out_status->metrics = snapshot->metrics;
    out_status->daemon = snapshot->daemon;
    out_status->hardware = snapshot->hardware;
}

static void swbt_ipc_adapter_error_from_command(const swbt_ipc_command_t *command,
                                                swbt_ipc_response_t *response,
                                                swbt_ipc_error_code_t error_code,
                                                const char *message) {
    swbt_ipc_adapter_set_error(response, command->has_request_id, command->request_id, error_code,
                               message);
}

static swbt_ipc_json_result_t swbt_ipc_adapter_execute_command(swbt_app_t *app, uint32_t client_id,
                                                               const swbt_ipc_command_t *command,
                                                               swbt_ipc_response_t *out_response) {
    swbt_ipc_status_t status;
    swbt_ipc_result_t result = SWBT_IPC_OK;

    swbt_ipc_adapter_init_response(out_response);
    swbt_ipc_adapter_copy_command_request(command, out_response);

    switch (command->type) {
    case SWBT_IPC_COMMAND_HELLO:
        out_response->type = SWBT_IPC_RESPONSE_HELLO_OK;
        out_response->client_id = client_id;
        return SWBT_IPC_JSON_OK;
    case SWBT_IPC_COMMAND_ACQUIRE:
        result = swbt_ipc_map_app_result(swbt_app_acquire(app, client_id));
        if (result == SWBT_IPC_ERROR_OWNER_BUSY) {
            swbt_ipc_adapter_error_from_command(command, out_response,
                                                SWBT_IPC_ERROR_CODE_OWNER_BUSY,
                                                "another client owns the controller");
            return SWBT_IPC_JSON_OK;
        }
        if (result != SWBT_IPC_OK) {
            swbt_ipc_adapter_error_from_command(command, out_response,
                                                SWBT_IPC_ERROR_CODE_INTERNAL_ERROR,
                                                "failed to acquire owner");
            return SWBT_IPC_JSON_OK;
        }
        out_response->type = SWBT_IPC_RESPONSE_ACQUIRED;
        out_response->owner_client_id = client_id;
        return SWBT_IPC_JSON_OK;
    case SWBT_IPC_COMMAND_RELEASE:
        if (!swbt_ipc_adapter_command_owner_matches_client(command, client_id)) {
            swbt_ipc_adapter_error_from_command(command, out_response,
                                                SWBT_IPC_ERROR_CODE_NOT_OWNER,
                                                "client does not own the controller");
            return SWBT_IPC_JSON_OK;
        }
        result = swbt_ipc_map_app_result(swbt_app_revoke(app, (swbt_app_revoke_options_t){
                                                                  .reason = SWBT_APP_REVOKE_RELEASE,
                                                                  .client_id = client_id,
                                                              }));
        if (result == SWBT_IPC_ERROR_NOT_OWNER) {
            swbt_ipc_adapter_error_from_command(command, out_response,
                                                SWBT_IPC_ERROR_CODE_NOT_OWNER,
                                                "client does not own the controller");
            return SWBT_IPC_JSON_OK;
        }
        if (result != SWBT_IPC_OK) {
            swbt_ipc_adapter_error_from_command(command, out_response,
                                                SWBT_IPC_ERROR_CODE_INTERNAL_ERROR,
                                                "failed to release owner");
            return SWBT_IPC_JSON_OK;
        }
        out_response->type = SWBT_IPC_RESPONSE_RELEASED;
        return SWBT_IPC_JSON_OK;
    case SWBT_IPC_COMMAND_SET_STATE:
        if (!swbt_ipc_adapter_command_owner_matches_client(command, client_id)) {
            (void)swbt_app_record_state_update_rejected(app);
            swbt_ipc_adapter_error_from_command(command, out_response,
                                                SWBT_IPC_ERROR_CODE_NOT_OWNER,
                                                "client does not own the controller");
            return SWBT_IPC_JSON_OK;
        }
        result = swbt_ipc_map_app_result(swbt_app_set_state(app, (swbt_app_set_state_options_t){
                                                                     .client_id = client_id,
                                                                     .state = &command->state,
                                                                     .sequence = command->sequence,
                                                                 }));
        if (result == SWBT_IPC_ERROR_NOT_OWNER) {
            swbt_ipc_adapter_error_from_command(command, out_response,
                                                SWBT_IPC_ERROR_CODE_NOT_OWNER,
                                                "client does not own the controller");
            return SWBT_IPC_JSON_OK;
        }
        if (result != SWBT_IPC_OK) {
            swbt_ipc_adapter_error_from_command(
                command, out_response, SWBT_IPC_ERROR_CODE_INTERNAL_ERROR, "failed to set state");
            return SWBT_IPC_JSON_OK;
        }
        out_response->type = SWBT_IPC_RESPONSE_STATE_ACCEPTED;
        out_response->sequence = command->sequence;
        return SWBT_IPC_JSON_OK;
    case SWBT_IPC_COMMAND_GET_STATUS:
        if (swbt_ipc_adapter_get_status(app, &status) != SWBT_IPC_OK) {
            swbt_ipc_adapter_error_from_command(
                command, out_response, SWBT_IPC_ERROR_CODE_INTERNAL_ERROR, "failed to get status");
            return SWBT_IPC_JSON_OK;
        }
        out_response->type = SWBT_IPC_RESPONSE_STATUS;
        swbt_ipc_adapter_copy_status(&status, &out_response->status);
        return SWBT_IPC_JSON_OK;
    case SWBT_IPC_COMMAND_NONE:
        break;
    }

    swbt_ipc_adapter_error_from_command(command, out_response, SWBT_IPC_ERROR_CODE_INTERNAL_ERROR,
                                        "failed to decode command");
    return SWBT_IPC_JSON_OK;
}

swbt_ipc_json_result_t swbt_ipc_adapter_handle_line(swbt_app_t *app, uint32_t client_id,
                                                    const char *line, char *response,
                                                    size_t response_size) {
    swbt_ipc_command_t command;
    swbt_ipc_response_t typed_response;
    swbt_ipc_json_result_t result = SWBT_IPC_JSON_OK;

    if (app == NULL || line == NULL || response == NULL || response_size == 0) {
        return SWBT_IPC_JSON_ERROR_INVALID_ARGUMENT;
    }

    result = swbt_ipc_json_decode_command(line, &command, &typed_response);
    if (result != SWBT_IPC_JSON_OK) {
        return result;
    }
    if (typed_response.type == SWBT_IPC_RESPONSE_NONE) {
        result = swbt_ipc_adapter_execute_command(app, client_id, &command, &typed_response);
        if (result != SWBT_IPC_JSON_OK) {
            return result;
        }
    }

    return swbt_ipc_json_encode_response(&typed_response, response, response_size);
}

swbt_ipc_result_t swbt_ipc_adapter_handle_disconnect(swbt_app_t *app, uint32_t client_id) {
    return swbt_ipc_map_app_result(swbt_app_revoke(app, (swbt_app_revoke_options_t){
                                                            .reason = SWBT_APP_REVOKE_DISCONNECT,
                                                            .client_id = client_id,
                                                        }));
}

swbt_ipc_result_t swbt_ipc_adapter_handle_heartbeat_timeout(swbt_app_t *app, uint32_t client_id) {
    return swbt_ipc_map_app_result(
        swbt_app_revoke(app, (swbt_app_revoke_options_t){
                                 .reason = SWBT_APP_REVOKE_HEARTBEAT_TIMEOUT,
                                 .client_id = client_id,
                             }));
}

swbt_ipc_result_t swbt_ipc_adapter_handle_shutdown(swbt_app_t *app) {
    return swbt_ipc_map_app_result(swbt_app_revoke(app, (swbt_app_revoke_options_t){
                                                            .reason = SWBT_APP_REVOKE_SHUTDOWN,
                                                            .client_id = 0u,
                                                        }));
}

swbt_ipc_result_t swbt_ipc_adapter_get_status(const swbt_app_t *app,
                                              swbt_ipc_status_t *out_status) {
    swbt_app_snapshot_t snapshot;

    if (app == NULL || out_status == NULL) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    if (swbt_app_snapshot(app, &snapshot) != SWBT_APP_OK) {
        return SWBT_IPC_ERROR_INVALID_ARGUMENT;
    }
    swbt_ipc_adapter_copy_snapshot(&snapshot, out_status);
    return SWBT_IPC_OK;
}
