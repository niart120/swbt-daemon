#include "ipc/ipc_json.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    SWBT_IPC_JSON_KEY_MAX = 32,
    SWBT_IPC_JSON_STRING_MAX = 96,
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

        if (*cursor != '"') {
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

static bool swbt_ipc_parse_state(const char *line, swbt_state_t *out_state) {
    int64_t buttons = 0;
    int64_t seq = 0;
    const char *state_value = swbt_json_find_key_value(line, "state");
    swbt_state_t state = swbt_state_neutral();

    if (state_value == NULL || *state_value != '{') {
        return false;
    }
    if (!swbt_ipc_parse_required_i64(line, "buttons", &buttons)) {
        return false;
    }
    if (buttons < 0 || buttons > UINT32_MAX ||
        (((uint32_t)buttons) & ~SWBT_DEFINED_BUTTON_MASK) != 0u) {
        return false;
    }
    state.buttons = (uint32_t)buttons;

    if (!swbt_ipc_parse_stick_field(line, "lx", &state.lx) ||
        !swbt_ipc_parse_stick_field(line, "ly", &state.ly) ||
        !swbt_ipc_parse_stick_field(line, "rx", &state.rx) ||
        !swbt_ipc_parse_stick_field(line, "ry", &state.ry) ||
        !swbt_ipc_parse_i16_field(line, "accel_x", &state.accel_x) ||
        !swbt_ipc_parse_i16_field(line, "accel_y", &state.accel_y) ||
        !swbt_ipc_parse_i16_field(line, "accel_z", &state.accel_z) ||
        !swbt_ipc_parse_i16_field(line, "gyro_x", &state.gyro_x) ||
        !swbt_ipc_parse_i16_field(line, "gyro_y", &state.gyro_y) ||
        !swbt_ipc_parse_i16_field(line, "gyro_z", &state.gyro_z)) {
        return false;
    }

    if (swbt_json_get_i64(line, "seq", &seq) < 0 || seq < 0) {
        return false;
    }
    state.client_seq = (uint64_t)seq;

    *out_state = state;
    return true;
}

static swbt_ipc_json_result_t swbt_ipc_handle_hello(uint32_t client_id, char *response,
                                                    size_t response_size, bool has_request_id,
                                                    const char *request_id) {
    char client_id_text[SWBT_IPC_OWNER_ID_BUFFER_SIZE];
    swbt_ipc_format_owner_id(client_id, client_id_text);

    if (has_request_id) {
        return swbt_json_write(response, response_size,
                               "{\"v\":1,\"type\":\"hello_ok\",\"request_id\":\"%s\","
                               "\"client_id\":\"%s\"}\n",
                               request_id, client_id_text);
    }

    return swbt_json_write(response, response_size,
                           "{\"v\":1,\"type\":\"hello_ok\",\"client_id\":\"%s\"}\n",
                           client_id_text);
}

static swbt_ipc_json_result_t swbt_ipc_handle_acquire(swbt_ipc_session_t *session,
                                                      uint32_t client_id, char *response,
                                                      size_t response_size, bool has_request_id,
                                                      const char *request_id) {
    swbt_ipc_result_t result = swbt_ipc_acquire(session, client_id);
    char owner_id[SWBT_IPC_OWNER_ID_BUFFER_SIZE];

    if (result == SWBT_IPC_ERROR_OWNER_BUSY) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "owner_busy", "another client owns the controller");
    }
    if (result != SWBT_IPC_OK) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "internal_error", "failed to acquire owner");
    }

    swbt_ipc_format_owner_id(client_id, owner_id);
    if (has_request_id) {
        return swbt_json_write(response, response_size,
                               "{\"v\":1,\"type\":\"acquired\",\"request_id\":\"%s\","
                               "\"owner_id\":\"%s\"}\n",
                               request_id, owner_id);
    }

    return swbt_json_write(response, response_size,
                           "{\"v\":1,\"type\":\"acquired\",\"owner_id\":\"%s\"}\n", owner_id);
}

static bool swbt_ipc_json_owner_matches_client(const char *line, uint32_t client_id) {
    char owner_id[SWBT_IPC_JSON_STRING_MAX];
    uint32_t parsed_client_id = 0;

    if (swbt_json_get_string(line, "owner_id", owner_id, sizeof(owner_id)) != 1) {
        return false;
    }
    if (!swbt_ipc_parse_owner_id(owner_id, &parsed_client_id)) {
        return false;
    }

    return parsed_client_id == client_id;
}

static swbt_ipc_json_result_t swbt_ipc_handle_release(swbt_ipc_session_t *session,
                                                      uint32_t client_id, const char *line,
                                                      char *response, size_t response_size,
                                                      bool has_request_id, const char *request_id) {
    swbt_ipc_result_t result = SWBT_IPC_OK;

    if (!swbt_ipc_json_owner_matches_client(line, client_id)) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "not_owner", "client does not own the controller");
    }

    result = swbt_ipc_release(session, client_id);
    if (result == SWBT_IPC_ERROR_NOT_OWNER) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "not_owner", "client does not own the controller");
    }
    if (result != SWBT_IPC_OK) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "internal_error", "failed to release owner");
    }

    if (has_request_id) {
        return swbt_json_write(response, response_size,
                               "{\"v\":1,\"type\":\"released\",\"request_id\":\"%s\"}\n",
                               request_id);
    }

    return swbt_json_write(response, response_size, "{\"v\":1,\"type\":\"released\"}\n");
}

static swbt_ipc_json_result_t swbt_ipc_handle_set_state(swbt_ipc_session_t *session,
                                                        uint32_t client_id, const char *line,
                                                        char *response, size_t response_size,
                                                        bool has_request_id,
                                                        const char *request_id) {
    swbt_state_t state;
    swbt_ipc_result_t result = SWBT_IPC_OK;

    if (!swbt_ipc_json_owner_matches_client(line, client_id)) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "not_owner", "client does not own the controller");
    }
    if (!swbt_ipc_parse_state(line, &state)) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "invalid_state", "state field is invalid");
    }

    result = swbt_ipc_set_state(session, client_id, &state);
    if (result == SWBT_IPC_ERROR_NOT_OWNER) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "not_owner", "client does not own the controller");
    }
    if (result != SWBT_IPC_OK) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "internal_error", "failed to set state");
    }

    if (!has_request_id) {
        response[0] = '\0';
        return SWBT_IPC_JSON_OK;
    }

    return swbt_json_write(response, response_size,
                           "{\"v\":1,\"type\":\"state_accepted\",\"request_id\":\"%s\","
                           "\"seq\":%llu}\n",
                           request_id, (unsigned long long)state.client_seq);
}

static swbt_ipc_json_result_t swbt_ipc_handle_get_status(swbt_ipc_session_t *session,
                                                         char *response, size_t response_size,
                                                         bool has_request_id,
                                                         const char *request_id) {
    swbt_ipc_status_t status;
    char owner_id[SWBT_IPC_OWNER_ID_BUFFER_SIZE] = "00000000";
    char rumble_raw[SWBT_IPC_RUMBLE_HEX_BUFFER_SIZE];

    if (swbt_ipc_get_status(session, &status) != SWBT_IPC_OK) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "internal_error", "failed to get status");
    }

    if (status.has_owner) {
        swbt_ipc_format_owner_id(status.owner_client_id, owner_id);
    }
    swbt_ipc_format_rumble_raw(status.rumble.raw, rumble_raw);

    if (has_request_id) {
        return swbt_json_write(
            response, response_size,
            "{\"v\":1,\"type\":\"status\",\"request_id\":\"%s\","
            "\"owner\":{\"present\":%s,\"owner_id\":\"%s\",\"last_seq\":%llu},"
            "\"state\":{\"buttons\":%u,\"lx\":%u,\"ly\":%u,\"rx\":%u,\"ry\":%u,"
            "\"accel_x\":%d,\"accel_y\":%d,\"accel_z\":%d,"
            "\"gyro_x\":%d,\"gyro_y\":%d,\"gyro_z\":%d},"
            "\"rumble\":{\"updated\":%s,\"last_update_ms\":%llu,\"raw\":\"%s\"}}\n",
            request_id, status.has_owner ? "true" : "false", owner_id,
            (unsigned long long)status.state.client_seq, (unsigned int)status.state.buttons,
            (unsigned int)status.state.lx, (unsigned int)status.state.ly,
            (unsigned int)status.state.rx, (unsigned int)status.state.ry, (int)status.state.accel_x,
            (int)status.state.accel_y, (int)status.state.accel_z, (int)status.state.gyro_x,
            (int)status.state.gyro_y, (int)status.state.gyro_z,
            status.rumble.updated ? "true" : "false",
            (unsigned long long)status.rumble.updated_at_ms, rumble_raw);
    }

    return swbt_json_write(
        response, response_size,
        "{\"v\":1,\"type\":\"status\","
        "\"owner\":{\"present\":%s,\"owner_id\":\"%s\",\"last_seq\":%llu},"
        "\"state\":{\"buttons\":%u,\"lx\":%u,\"ly\":%u,\"rx\":%u,\"ry\":%u,"
        "\"accel_x\":%d,\"accel_y\":%d,\"accel_z\":%d,"
        "\"gyro_x\":%d,\"gyro_y\":%d,\"gyro_z\":%d},"
        "\"rumble\":{\"updated\":%s,\"last_update_ms\":%llu,\"raw\":\"%s\"}}\n",
        status.has_owner ? "true" : "false", owner_id, (unsigned long long)status.state.client_seq,
        (unsigned int)status.state.buttons, (unsigned int)status.state.lx,
        (unsigned int)status.state.ly, (unsigned int)status.state.rx, (unsigned int)status.state.ry,
        (int)status.state.accel_x, (int)status.state.accel_y, (int)status.state.accel_z,
        (int)status.state.gyro_x, (int)status.state.gyro_y, (int)status.state.gyro_z,
        status.rumble.updated ? "true" : "false", (unsigned long long)status.rumble.updated_at_ms,
        rumble_raw);
}

swbt_ipc_json_result_t swbt_ipc_json_handle_line(swbt_ipc_session_t *session, uint32_t client_id,
                                                 const char *line, char *response,
                                                 size_t response_size) {
    char request_id[SWBT_IPC_JSON_STRING_MAX];
    char type[SWBT_IPC_JSON_STRING_MAX];
    int64_t version = 0;
    bool has_request_id = false;
    int type_result = 0;

    if (session == NULL || line == NULL || response == NULL || response_size == 0) {
        return SWBT_IPC_JSON_ERROR_INVALID_ARGUMENT;
    }

    response[0] = '\0';

    if (!swbt_json_is_complete_object(line)) {
        return swbt_ipc_write_error(response, response_size, false, "", "invalid_json",
                                    "message is not a complete JSON object");
    }
    if (!swbt_ipc_parse_request_id(line, request_id, &has_request_id)) {
        return swbt_ipc_write_error(response, response_size, false, "", "invalid_json",
                                    "request_id is invalid");
    }
    if (swbt_json_get_i64(line, "v", &version) != 1 || version != 1) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "invalid_version", "unsupported protocol version");
    }

    type_result = swbt_json_get_string(line, "type", type, sizeof(type));
    if (type_result < 0) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "invalid_json", "type is invalid");
    }
    if (type_result == 0) {
        return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                    "unsupported_command", "missing command type");
    }

    if (strcmp(type, "hello") == 0) {
        return swbt_ipc_handle_hello(client_id, response, response_size, has_request_id,
                                     request_id);
    }
    if (strcmp(type, "acquire") == 0) {
        return swbt_ipc_handle_acquire(session, client_id, response, response_size, has_request_id,
                                       request_id);
    }
    if (strcmp(type, "release") == 0) {
        return swbt_ipc_handle_release(session, client_id, line, response, response_size,
                                       has_request_id, request_id);
    }
    if (strcmp(type, "set_state") == 0) {
        return swbt_ipc_handle_set_state(session, client_id, line, response, response_size,
                                         has_request_id, request_id);
    }
    if (strcmp(type, "get_status") == 0) {
        return swbt_ipc_handle_get_status(session, response, response_size, has_request_id,
                                          request_id);
    }

    return swbt_ipc_write_error(response, response_size, has_request_id, request_id,
                                "unsupported_command", "unsupported command type");
}
