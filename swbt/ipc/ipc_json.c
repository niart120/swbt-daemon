#include "ipc/ipc_json.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/swbt_version.h"

enum {
    SWBT_IPC_JSON_KEY_MAX = 32,
    SWBT_IPC_PROTOCOL_VERSION = 1,
    SWBT_IPC_OWNER_ID_HEX_SIZE = 8,
    SWBT_IPC_OWNER_ID_BUFFER_SIZE = SWBT_IPC_OWNER_ID_HEX_SIZE + 1,
    SWBT_IPC_RUMBLE_HEX_SIZE = SWBT_SWITCH_RUMBLE_DATA_SIZE * 2,
    SWBT_IPC_RUMBLE_HEX_BUFFER_SIZE = SWBT_IPC_RUMBLE_HEX_SIZE + 1,
    SWBT_IPC_STICK_MAX = 4095,
};

static const uint32_t SWBT_DEFINED_BUTTON_MASK =
    SWBT_BUTTON_Y | SWBT_BUTTON_X | SWBT_BUTTON_B | SWBT_BUTTON_A | SWBT_BUTTON_SR_R |
    SWBT_BUTTON_SL_R | SWBT_BUTTON_R | SWBT_BUTTON_ZR | SWBT_BUTTON_MINUS | SWBT_BUTTON_PLUS |
    SWBT_BUTTON_R_STICK | SWBT_BUTTON_L_STICK | SWBT_BUTTON_HOME | SWBT_BUTTON_CAPTURE |
    SWBT_BUTTON_DOWN | SWBT_BUTTON_UP | SWBT_BUTTON_RIGHT | SWBT_BUTTON_LEFT | SWBT_BUTTON_SR_L |
    SWBT_BUTTON_SL_L | SWBT_BUTTON_L | SWBT_BUTTON_ZL;

static const char *swbt_json_skip_ws(const char *cursor) {
    while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
        ++cursor;
    }
    return cursor;
}

static bool swbt_json_is_complete_object(const char *line) {
    size_t length = 0;
    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    bool saw_object_end = false;
    const char *cursor = swbt_json_skip_ws(line);

    if (*cursor != '{') {
        return false;
    }

    for (; *cursor != '\0'; ++cursor) {
        if (length >= SWBT_IPC_JSON_LINE_MAX) {
            return false;
        }
        ++length;

        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (*cursor == '\\') {
                escaped = true;
            } else if (*cursor == '"') {
                in_string = false;
            }
            continue;
        }

        if (*cursor == '"') {
            in_string = true;
        } else if (*cursor == '{') {
            ++depth;
        } else if (*cursor == '}') {
            --depth;
            if (depth < 0) {
                return false;
            }
            if (depth == 0) {
                saw_object_end = true;
                ++cursor;
                break;
            }
        }
    }

    if (!saw_object_end || in_string || depth != 0) {
        return false;
    }

    cursor = swbt_json_skip_ws(cursor);
    return *cursor == '\0';
}

static bool swbt_json_key_matches(const char *start, size_t length, const char *key) {
    return strlen(key) == length && strncmp(start, key, length) == 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static const char *swbt_json_find_key_value(const char *json, const char *key) {
    bool in_string = false;
    bool escaped = false;
    bool saw_root_object = false;
    int depth = 0;

    for (const char *cursor = json; *cursor != '\0'; ++cursor) {
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (*cursor == '\\') {
                escaped = true;
            } else if (*cursor == '"') {
                in_string = false;
            }
            continue;
        }

        if (*cursor == '{') {
            saw_root_object = true;
            ++depth;
            continue;
        }
        if (*cursor == '}') {
            if (depth > 0) {
                --depth;
            }
            if (saw_root_object && depth == 0) {
                break;
            }
            continue;
        }
        if (*cursor != '"') {
            continue;
        }
        if (depth != 1) {
            in_string = true;
            continue;
        }

        const char *key_start = cursor + 1;
        const char *key_end = key_start;
        while (*key_end != '\0' && *key_end != '"' && *key_end != '\\') {
            ++key_end;
        }
        if (*key_end != '"') {
            return NULL;
        }

        const char *after_key = swbt_json_skip_ws(key_end + 1);
        if (*after_key == ':' &&
            swbt_json_key_matches(key_start, (size_t)(key_end - key_start), key)) {
            return swbt_json_skip_ws(after_key + 1);
        }

        cursor = key_end;
    }

    return NULL;
}

static int swbt_json_read_string(const char *value, char *out, size_t out_size) {
    size_t length = 0;

    if (value == NULL) {
        return 0;
    }
    if (*value != '"') {
        return -1;
    }
    ++value;

    while (*value != '\0' && *value != '"') {
        if (*value == '\\' || (unsigned char)*value < 0x20u) {
            return -1;
        }
        if (length + 1 >= out_size) {
            return -1;
        }
        out[length] = *value;
        ++length;
        ++value;
    }

    if (*value != '"') {
        return -1;
    }

    out[length] = '\0';
    return 1;
}

static int swbt_json_read_i64(const char *value, int64_t *out) {
    char *end = NULL;
    const char *after_value = NULL;
    long long parsed = 0;

    if (value == NULL) {
        return 0;
    }

    errno = 0;
    parsed = strtoll(value, &end, 10);
    if (value == end || errno != 0) {
        return -1;
    }
    after_value = swbt_json_skip_ws(end);
    if (*after_value != ',' && *after_value != '}' && *after_value != ']' && *after_value != '\0') {
        return -1;
    }

    *out = (int64_t)parsed;
    return 1;
}

static int swbt_json_get_string(const char *json, const char *key, char *out, size_t out_size) {
    return swbt_json_read_string(swbt_json_find_key_value(json, key), out, out_size);
}

static int swbt_json_get_i64(const char *json, const char *key, int64_t *out) {
    return swbt_json_read_i64(swbt_json_find_key_value(json, key), out);
}

static swbt_ipc_json_result_t swbt_json_write(char *response, size_t response_size,
                                              const char *format, ...) {
    va_list args;
    int written = 0;

    va_start(args, format);
    // C11 Annex K formatting functions are not consistently available in the target toolchains.
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
    written = vsnprintf(response, response_size, format, args);
    va_end(args);

    if (written < 0 || (size_t)written >= response_size) {
        if (response_size > 0) {
            response[0] = '\0';
        }
        return SWBT_IPC_JSON_ERROR_RESPONSE_TOO_SMALL;
    }

    return SWBT_IPC_JSON_OK;
}

static void swbt_ipc_format_owner_id(uint32_t client_id, char out[SWBT_IPC_OWNER_ID_BUFFER_SIZE]) {
    static const char hex[] = "0123456789abcdef";

    for (size_t index = 0; index < SWBT_IPC_OWNER_ID_HEX_SIZE; ++index) {
        const unsigned int shift = (unsigned int)((SWBT_IPC_OWNER_ID_HEX_SIZE - 1u - index) * 4u);
        out[index] = hex[(client_id >> shift) & 0x0Fu];
    }
    out[SWBT_IPC_OWNER_ID_HEX_SIZE] = '\0';
}

static void swbt_ipc_format_rumble_raw(const uint8_t raw[SWBT_SWITCH_RUMBLE_DATA_SIZE],
                                       char out[SWBT_IPC_RUMBLE_HEX_BUFFER_SIZE]) {
    static const char hex[] = "0123456789abcdef";

    for (size_t index = 0; index < SWBT_SWITCH_RUMBLE_DATA_SIZE; ++index) {
        out[index * 2u] = hex[(raw[index] >> 4u) & 0x0Fu];
        out[(index * 2u) + 1u] = hex[raw[index] & 0x0Fu];
    }
    out[SWBT_IPC_RUMBLE_HEX_SIZE] = '\0';
}

static bool swbt_ipc_parse_owner_id(const char *owner_id, uint32_t *out_client_id) {
    char *end = NULL;
    unsigned long parsed = 0;

    if (owner_id == NULL || strlen(owner_id) != SWBT_IPC_OWNER_ID_HEX_SIZE) {
        return false;
    }

    errno = 0;
    parsed = strtoul(owner_id, &end, 16);
    if (errno != 0 || *end != '\0' || parsed > UINT32_MAX) {
        return false;
    }

    *out_client_id = (uint32_t)parsed;
    return true;
}

static swbt_ipc_json_result_t swbt_ipc_write_error(char *response, size_t response_size,
                                                   bool has_request_id, const char *request_id,
                                                   const char *code, const char *message) {
    if (has_request_id) {
        return swbt_json_write(response, response_size,
                               "{\"v\":1,\"type\":\"error\",\"request_id\":\"%s\","
                               "\"code\":\"%s\",\"message\":\"%s\"}\n",
                               request_id, code, message);
    }

    return swbt_json_write(response, response_size,
                           "{\"v\":1,\"type\":\"error\",\"code\":\"%s\",\"message\":\"%s\"}\n",
                           code, message);
}

static bool swbt_ipc_parse_request_id(const char *line, char request_id[SWBT_IPC_JSON_STRING_MAX],
                                      bool *has_request_id) {
    int result = swbt_json_get_string(line, "request_id", request_id, SWBT_IPC_JSON_STRING_MAX);
    if (result < 0) {
        return false;
    }
    *has_request_id = result > 0;
    if (!*has_request_id) {
        request_id[0] = '\0';
    }
    return true;
}

static bool swbt_ipc_parse_required_i64(const char *line, const char *key, int64_t *out) {
    return swbt_json_get_i64(line, key, out) == 1;
}

static bool swbt_ipc_parse_i16_field(const char *line, const char *key, int16_t *out) {
    int64_t value = 0;
    if (!swbt_ipc_parse_required_i64(line, key, &value)) {
        return false;
    }
    if (value < INT16_MIN || value > INT16_MAX) {
        return false;
    }
    *out = (int16_t)value;
    return true;
}

static bool swbt_ipc_parse_stick_field(const char *line, const char *key, uint16_t *out) {
    int64_t value = 0;
    if (!swbt_ipc_parse_required_i64(line, key, &value)) {
        return false;
    }
    if (value < 0 || value > SWBT_IPC_STICK_MAX) {
        return false;
    }
    *out = (uint16_t)value;
    return true;
}

static bool swbt_ipc_parse_state(const char *line, swbt_state_t *out_state,
                                 uint64_t *out_sequence) {
    int64_t buttons = 0;
    int64_t seq = 0;
    const char *state_value = swbt_json_find_key_value(line, "state");
    const char *state_json = state_value;
    swbt_state_t state = swbt_state_neutral();

    if (state_value == NULL || *state_value != '{') {
        return false;
    }
    if (!swbt_ipc_parse_required_i64(state_json, "buttons", &buttons)) {
        return false;
    }
    if (buttons < 0 || buttons > UINT32_MAX ||
        (((uint32_t)buttons) & ~SWBT_DEFINED_BUTTON_MASK) != 0u) {
        return false;
    }
    state.buttons = (uint32_t)buttons;

    if (!swbt_ipc_parse_stick_field(state_json, "lx", &state.lx) ||
        !swbt_ipc_parse_stick_field(state_json, "ly", &state.ly) ||
        !swbt_ipc_parse_stick_field(state_json, "rx", &state.rx) ||
        !swbt_ipc_parse_stick_field(state_json, "ry", &state.ry) ||
        !swbt_ipc_parse_i16_field(state_json, "accel_x", &state.accel_x) ||
        !swbt_ipc_parse_i16_field(state_json, "accel_y", &state.accel_y) ||
        !swbt_ipc_parse_i16_field(state_json, "accel_z", &state.accel_z) ||
        !swbt_ipc_parse_i16_field(state_json, "gyro_x", &state.gyro_x) ||
        !swbt_ipc_parse_i16_field(state_json, "gyro_y", &state.gyro_y) ||
        !swbt_ipc_parse_i16_field(state_json, "gyro_z", &state.gyro_z)) {
        return false;
    }

    if (swbt_json_get_i64(line, "seq", &seq) < 0 || seq < 0) {
        return false;
    }

    *out_state = state;
    *out_sequence = (uint64_t)seq;
    return true;
}

static void swbt_ipc_json_init_command(swbt_ipc_command_t *command) {
    command->type = SWBT_IPC_COMMAND_NONE;
    command->has_request_id = false;
    command->request_id[0] = '\0';
    command->has_owner_id = false;
    command->owner_client_id = 0;
    command->sequence = 0;
    command->state = swbt_state_neutral();
}

static void swbt_ipc_json_init_response(swbt_ipc_response_t *response) {
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
    daemon.lifecycle_state = SWBT_IPC_DAEMON_LIFECYCLE_UNAVAILABLE;
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

static void swbt_ipc_json_copy_text(char *out, size_t out_size, const char *text) {
    size_t index = 0;

    if (out_size == 0) {
        return;
    }
    for (; index + 1u < out_size && text[index] != '\0'; ++index) {
        out[index] = text[index];
    }
    out[index] = '\0';
}

static void swbt_ipc_json_set_error(swbt_ipc_response_t *response, bool has_request_id,
                                    const char *request_id, swbt_ipc_error_code_t error_code,
                                    const char *message) {
    response->type = SWBT_IPC_RESPONSE_ERROR;
    response->has_request_id = has_request_id;
    if (has_request_id) {
        swbt_ipc_json_copy_text(response->request_id, sizeof(response->request_id), request_id);
    } else {
        response->request_id[0] = '\0';
    }
    response->error_code = error_code;
    swbt_ipc_json_copy_text(response->error_message, sizeof(response->error_message), message);
}

static void swbt_ipc_json_copy_request_id(swbt_ipc_command_t *command, bool has_request_id,
                                          const char *request_id) {
    command->has_request_id = has_request_id;
    if (has_request_id) {
        swbt_ipc_json_copy_text(command->request_id, sizeof(command->request_id), request_id);
    } else {
        command->request_id[0] = '\0';
    }
}

swbt_ipc_json_result_t swbt_ipc_json_decode_command(const char *line,
                                                    swbt_ipc_command_t *out_command,
                                                    swbt_ipc_response_t *out_error_response) {
    char request_id[SWBT_IPC_JSON_STRING_MAX];
    char type[SWBT_IPC_JSON_STRING_MAX];
    char owner_id[SWBT_IPC_JSON_STRING_MAX];
    int64_t version = 0;
    bool has_request_id = false;
    int type_result = 0;

    if (line == NULL || out_command == NULL || out_error_response == NULL) {
        return SWBT_IPC_JSON_ERROR_INVALID_ARGUMENT;
    }

    swbt_ipc_json_init_command(out_command);
    swbt_ipc_json_init_response(out_error_response);

    if (!swbt_json_is_complete_object(line)) {
        swbt_ipc_json_set_error(out_error_response, false, "", SWBT_IPC_ERROR_CODE_INVALID_JSON,
                                "message is not a complete JSON object");
        return SWBT_IPC_JSON_OK;
    }
    if (!swbt_ipc_parse_request_id(line, request_id, &has_request_id)) {
        swbt_ipc_json_set_error(out_error_response, false, "", SWBT_IPC_ERROR_CODE_INVALID_JSON,
                                "request_id is invalid");
        return SWBT_IPC_JSON_OK;
    }
    if (swbt_json_get_i64(line, "v", &version) != 1 || version != 1) {
        swbt_ipc_json_set_error(out_error_response, has_request_id, request_id,
                                SWBT_IPC_ERROR_CODE_INVALID_VERSION,
                                "unsupported protocol version");
        return SWBT_IPC_JSON_OK;
    }

    type_result = swbt_json_get_string(line, "type", type, sizeof(type));
    if (type_result < 0) {
        swbt_ipc_json_set_error(out_error_response, has_request_id, request_id,
                                SWBT_IPC_ERROR_CODE_INVALID_JSON, "type is invalid");
        return SWBT_IPC_JSON_OK;
    }
    if (type_result == 0) {
        swbt_ipc_json_set_error(out_error_response, has_request_id, request_id,
                                SWBT_IPC_ERROR_CODE_UNSUPPORTED_COMMAND, "missing command type");
        return SWBT_IPC_JSON_OK;
    }

    swbt_ipc_json_copy_request_id(out_command, has_request_id, request_id);

    if (strcmp(type, "hello") == 0) {
        out_command->type = SWBT_IPC_COMMAND_HELLO;
        return SWBT_IPC_JSON_OK;
    }
    if (strcmp(type, "acquire") == 0) {
        out_command->type = SWBT_IPC_COMMAND_ACQUIRE;
        return SWBT_IPC_JSON_OK;
    }
    if (strcmp(type, "release") == 0) {
        out_command->type = SWBT_IPC_COMMAND_RELEASE;
        if (swbt_json_get_string(line, "owner_id", owner_id, sizeof(owner_id)) == 1) {
            out_command->has_owner_id =
                swbt_ipc_parse_owner_id(owner_id, &out_command->owner_client_id);
        }
        return SWBT_IPC_JSON_OK;
    }
    if (strcmp(type, "set_state") == 0) {
        out_command->type = SWBT_IPC_COMMAND_SET_STATE;
        if (swbt_json_get_string(line, "owner_id", owner_id, sizeof(owner_id)) == 1) {
            out_command->has_owner_id =
                swbt_ipc_parse_owner_id(owner_id, &out_command->owner_client_id);
        }
        if (!swbt_ipc_parse_state(line, &out_command->state, &out_command->sequence)) {
            swbt_ipc_json_init_command(out_command);
            swbt_ipc_json_set_error(out_error_response, has_request_id, request_id,
                                    SWBT_IPC_ERROR_CODE_INVALID_STATE, "state field is invalid");
        }
        return SWBT_IPC_JSON_OK;
    }
    if (strcmp(type, "get_status") == 0) {
        out_command->type = SWBT_IPC_COMMAND_GET_STATUS;
        return SWBT_IPC_JSON_OK;
    }

    swbt_ipc_json_set_error(out_error_response, has_request_id, request_id,
                            SWBT_IPC_ERROR_CODE_UNSUPPORTED_COMMAND, "unsupported command type");
    return SWBT_IPC_JSON_OK;
}

static const char *swbt_ipc_json_error_code_text(swbt_ipc_error_code_t error_code) {
    switch (error_code) {
    case SWBT_IPC_ERROR_CODE_INVALID_JSON:
        return "invalid_json";
    case SWBT_IPC_ERROR_CODE_INVALID_VERSION:
        return "invalid_version";
    case SWBT_IPC_ERROR_CODE_UNSUPPORTED_COMMAND:
        return "unsupported_command";
    case SWBT_IPC_ERROR_CODE_OWNER_BUSY:
        return "owner_busy";
    case SWBT_IPC_ERROR_CODE_NOT_OWNER:
        return "not_owner";
    case SWBT_IPC_ERROR_CODE_INVALID_STATE:
        return "invalid_state";
    case SWBT_IPC_ERROR_CODE_INTERNAL_ERROR:
        return "internal_error";
    }
    return "internal_error";
}

static const char *
swbt_ipc_json_hardware_status_text(swbt_metrics_hardware_status_t hardware_status) {
    switch (hardware_status) {
    case SWBT_METRICS_HARDWARE_UNAVAILABLE:
        return "unavailable";
    case SWBT_METRICS_HARDWARE_OBSERVED:
        return "observed";
    }
    return "unavailable";
}

static const char *swbt_ipc_json_daemon_backend_text(swbt_ipc_daemon_backend_t daemon_backend) {
    switch (daemon_backend) {
    case SWBT_IPC_DAEMON_BACKEND_UNKNOWN:
        return "unknown";
    case SWBT_IPC_DAEMON_BACKEND_NOOP:
        return "noop";
    case SWBT_IPC_DAEMON_BACKEND_PRODUCTION:
        return "production";
    }
    return "unknown";
}

static const char *
swbt_ipc_json_daemon_lifecycle_text(swbt_ipc_daemon_lifecycle_state_t lifecycle_state) {
    switch (lifecycle_state) {
    case SWBT_IPC_DAEMON_LIFECYCLE_UNAVAILABLE:
        return "unavailable";
    case SWBT_IPC_DAEMON_LIFECYCLE_STOPPED:
        return "stopped";
    case SWBT_IPC_DAEMON_LIFECYCLE_RUNNING:
        return "running";
    }
    return "unavailable";
}

static const char *
swbt_ipc_json_hardware_approval_text(swbt_ipc_hardware_approval_t hardware_approval) {
    switch (hardware_approval) {
    case SWBT_IPC_HARDWARE_APPROVAL_UNAVAILABLE:
        return "unavailable";
    case SWBT_IPC_HARDWARE_APPROVAL_APPROVED:
        return "approved";
    }
    return "unavailable";
}

static const char *
swbt_ipc_json_hardware_channel_text(swbt_ipc_hardware_channel_state_t hardware_channel_state) {
    switch (hardware_channel_state) {
    case SWBT_IPC_HARDWARE_CHANNEL_UNAVAILABLE:
        return "unavailable";
    }
    return "unavailable";
}

swbt_ipc_json_result_t swbt_ipc_json_encode_response(const swbt_ipc_response_t *typed_response,
                                                     char *response, size_t response_size) {
    char client_id[SWBT_IPC_OWNER_ID_BUFFER_SIZE];
    char owner_id[SWBT_IPC_OWNER_ID_BUFFER_SIZE];
    char rumble_raw[SWBT_IPC_RUMBLE_HEX_BUFFER_SIZE];

    if (typed_response == NULL || response == NULL || response_size == 0) {
        return SWBT_IPC_JSON_ERROR_INVALID_ARGUMENT;
    }

    response[0] = '\0';

    switch (typed_response->type) {
    case SWBT_IPC_RESPONSE_NONE:
        return SWBT_IPC_JSON_OK;
    case SWBT_IPC_RESPONSE_HELLO_OK:
        swbt_ipc_format_owner_id(typed_response->client_id, client_id);
        if (typed_response->has_request_id) {
            return swbt_json_write(response, response_size,
                                   "{\"v\":1,\"type\":\"hello_ok\",\"request_id\":\"%s\","
                                   "\"client_id\":\"%s\"}\n",
                                   typed_response->request_id, client_id);
        }
        return swbt_json_write(response, response_size,
                               "{\"v\":1,\"type\":\"hello_ok\",\"client_id\":\"%s\"}\n", client_id);
    case SWBT_IPC_RESPONSE_ACQUIRED:
        swbt_ipc_format_owner_id(typed_response->owner_client_id, owner_id);
        if (typed_response->has_request_id) {
            return swbt_json_write(response, response_size,
                                   "{\"v\":1,\"type\":\"acquired\",\"request_id\":\"%s\","
                                   "\"owner_id\":\"%s\"}\n",
                                   typed_response->request_id, owner_id);
        }
        return swbt_json_write(response, response_size,
                               "{\"v\":1,\"type\":\"acquired\",\"owner_id\":\"%s\"}\n", owner_id);
    case SWBT_IPC_RESPONSE_RELEASED:
        if (typed_response->has_request_id) {
            return swbt_json_write(response, response_size,
                                   "{\"v\":1,\"type\":\"released\",\"request_id\":\"%s\"}\n",
                                   typed_response->request_id);
        }
        return swbt_json_write(response, response_size, "{\"v\":1,\"type\":\"released\"}\n");
    case SWBT_IPC_RESPONSE_STATE_ACCEPTED:
        if (typed_response->has_request_id) {
            return swbt_json_write(response, response_size,
                                   "{\"v\":1,\"type\":\"state_accepted\","
                                   "\"request_id\":\"%s\",\"seq\":%llu}\n",
                                   typed_response->request_id,
                                   (unsigned long long)typed_response->sequence);
        }
        return SWBT_IPC_JSON_OK;
    case SWBT_IPC_RESPONSE_STATUS:
        swbt_ipc_format_owner_id(
            typed_response->status.has_owner ? typed_response->status.owner_client_id : 0u,
            owner_id);
        swbt_ipc_format_rumble_raw(typed_response->status.rumble.raw, rumble_raw);
        if (typed_response->has_request_id) {
            return swbt_json_write(
                response, response_size,
                "{\"v\":1,\"type\":\"status\",\"request_id\":\"%s\","
                "\"daemon\":{\"protocol_version\":%u,\"daemon_version\":\"%s\","
                "\"backend\":\"%s\",\"lifecycle_state\":\"%s\","
                "\"hardware_approval\":\"%s\"},"
                "\"metrics\":{\"hardware_status\":\"%s\","
                "\"report_ticks_total\":%llu,\"reports_sent_total\":%llu,"
                "\"send_failures_total\":%llu,\"report_interval_average_us\":%llu,"
                "\"report_interval_max_us\":%llu,\"ipc_state_accepted_total\":%llu,"
                "\"ipc_state_rejected_total\":%llu,\"ipc_state_coalesced_total\":%llu,"
                "\"actual_report_rate_hz\":%u,\"jitter_max_us\":%llu},"
                "\"hardware\":{\"adapter_state\":\"%s\","
                "\"switch_connection_state\":\"%s\",\"hid_channel_state\":\"%s\"},"
                "\"owner\":{\"present\":%s,\"owner_id\":\"%s\",\"last_seq\":%llu},"
                "\"state\":{\"buttons\":%u,\"lx\":%u,\"ly\":%u,\"rx\":%u,\"ry\":%u,"
                "\"accel_x\":%d,\"accel_y\":%d,\"accel_z\":%d,"
                "\"gyro_x\":%d,\"gyro_y\":%d,\"gyro_z\":%d},"
                "\"rumble\":{\"updated\":%s,\"last_update_ms\":%llu,\"raw\":\"%s\"}}\n",
                typed_response->request_id, (unsigned int)SWBT_IPC_PROTOCOL_VERSION,
                swbt_get_version_string(),
                swbt_ipc_json_daemon_backend_text(typed_response->status.daemon.backend),
                swbt_ipc_json_daemon_lifecycle_text(typed_response->status.daemon.lifecycle_state),
                swbt_ipc_json_hardware_approval_text(
                    typed_response->status.daemon.hardware_approval),
                swbt_ipc_json_hardware_status_text(typed_response->status.metrics.hardware_status),
                (unsigned long long)typed_response->status.metrics.report_ticks,
                (unsigned long long)typed_response->status.metrics.report_send_ok,
                (unsigned long long)typed_response->status.metrics.report_send_failed,
                (unsigned long long)typed_response->status.metrics.report_interval_average_us,
                (unsigned long long)typed_response->status.metrics.report_interval_max_us,
                (unsigned long long)typed_response->status.metrics.ipc_state_accepted,
                (unsigned long long)typed_response->status.metrics.ipc_state_rejected,
                (unsigned long long)typed_response->status.metrics.ipc_state_coalesced,
                (unsigned int)typed_response->status.metrics.actual_report_rate_hz,
                (unsigned long long)typed_response->status.metrics.jitter_max_us,
                swbt_ipc_json_hardware_channel_text(typed_response->status.hardware.adapter_state),
                swbt_ipc_json_hardware_channel_text(
                    typed_response->status.hardware.switch_connection_state),
                swbt_ipc_json_hardware_channel_text(
                    typed_response->status.hardware.hid_channel_state),
                typed_response->status.has_owner ? "true" : "false", owner_id,
                (unsigned long long)typed_response->status.last_sequence,
                (unsigned int)typed_response->status.state.buttons,
                (unsigned int)typed_response->status.state.lx,
                (unsigned int)typed_response->status.state.ly,
                (unsigned int)typed_response->status.state.rx,
                (unsigned int)typed_response->status.state.ry,
                (int)typed_response->status.state.accel_x,
                (int)typed_response->status.state.accel_y,
                (int)typed_response->status.state.accel_z, (int)typed_response->status.state.gyro_x,
                (int)typed_response->status.state.gyro_y, (int)typed_response->status.state.gyro_z,
                typed_response->status.rumble.updated ? "true" : "false",
                (unsigned long long)typed_response->status.rumble.updated_at_ms, rumble_raw);
        }
        return swbt_json_write(
            response, response_size,
            "{\"v\":1,\"type\":\"status\","
            "\"daemon\":{\"protocol_version\":%u,\"daemon_version\":\"%s\","
            "\"backend\":\"%s\",\"lifecycle_state\":\"%s\","
            "\"hardware_approval\":\"%s\"},"
            "\"metrics\":{\"hardware_status\":\"%s\","
            "\"report_ticks_total\":%llu,\"reports_sent_total\":%llu,"
            "\"send_failures_total\":%llu,\"report_interval_average_us\":%llu,"
            "\"report_interval_max_us\":%llu,\"ipc_state_accepted_total\":%llu,"
            "\"ipc_state_rejected_total\":%llu,\"ipc_state_coalesced_total\":%llu,"
            "\"actual_report_rate_hz\":%u,\"jitter_max_us\":%llu},"
            "\"hardware\":{\"adapter_state\":\"%s\","
            "\"switch_connection_state\":\"%s\",\"hid_channel_state\":\"%s\"},"
            "\"owner\":{\"present\":%s,\"owner_id\":\"%s\",\"last_seq\":%llu},"
            "\"state\":{\"buttons\":%u,\"lx\":%u,\"ly\":%u,\"rx\":%u,\"ry\":%u,"
            "\"accel_x\":%d,\"accel_y\":%d,\"accel_z\":%d,"
            "\"gyro_x\":%d,\"gyro_y\":%d,\"gyro_z\":%d},"
            "\"rumble\":{\"updated\":%s,\"last_update_ms\":%llu,\"raw\":\"%s\"}}\n",
            (unsigned int)SWBT_IPC_PROTOCOL_VERSION, swbt_get_version_string(),
            swbt_ipc_json_daemon_backend_text(typed_response->status.daemon.backend),
            swbt_ipc_json_daemon_lifecycle_text(typed_response->status.daemon.lifecycle_state),
            swbt_ipc_json_hardware_approval_text(typed_response->status.daemon.hardware_approval),
            swbt_ipc_json_hardware_status_text(typed_response->status.metrics.hardware_status),
            (unsigned long long)typed_response->status.metrics.report_ticks,
            (unsigned long long)typed_response->status.metrics.report_send_ok,
            (unsigned long long)typed_response->status.metrics.report_send_failed,
            (unsigned long long)typed_response->status.metrics.report_interval_average_us,
            (unsigned long long)typed_response->status.metrics.report_interval_max_us,
            (unsigned long long)typed_response->status.metrics.ipc_state_accepted,
            (unsigned long long)typed_response->status.metrics.ipc_state_rejected,
            (unsigned long long)typed_response->status.metrics.ipc_state_coalesced,
            (unsigned int)typed_response->status.metrics.actual_report_rate_hz,
            (unsigned long long)typed_response->status.metrics.jitter_max_us,
            swbt_ipc_json_hardware_channel_text(typed_response->status.hardware.adapter_state),
            swbt_ipc_json_hardware_channel_text(
                typed_response->status.hardware.switch_connection_state),
            swbt_ipc_json_hardware_channel_text(typed_response->status.hardware.hid_channel_state),
            typed_response->status.has_owner ? "true" : "false", owner_id,
            (unsigned long long)typed_response->status.last_sequence,
            (unsigned int)typed_response->status.state.buttons,
            (unsigned int)typed_response->status.state.lx,
            (unsigned int)typed_response->status.state.ly,
            (unsigned int)typed_response->status.state.rx,
            (unsigned int)typed_response->status.state.ry,
            (int)typed_response->status.state.accel_x, (int)typed_response->status.state.accel_y,
            (int)typed_response->status.state.accel_z, (int)typed_response->status.state.gyro_x,
            (int)typed_response->status.state.gyro_y, (int)typed_response->status.state.gyro_z,
            typed_response->status.rumble.updated ? "true" : "false",
            (unsigned long long)typed_response->status.rumble.updated_at_ms, rumble_raw);
    case SWBT_IPC_RESPONSE_ERROR:
        return swbt_ipc_write_error(response, response_size, typed_response->has_request_id,
                                    typed_response->request_id,
                                    swbt_ipc_json_error_code_text(typed_response->error_code),
                                    typed_response->error_message);
    }

    return SWBT_IPC_JSON_ERROR_INVALID_ARGUMENT;
}
