#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ipc/ipc_adapter.h"
#include "ipc/ipc_json.h"
#include "switch/switch_controller_state.h"
#include "switch/switch_rumble.h"

static int expect_contains(const char *text, const char *needle) {
    return strstr(text, needle) != NULL ? 0 : 1;
}

static int expect_eq_u32(uint32_t actual, uint32_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u16(uint16_t actual, uint16_t expected) {
    return actual == expected ? 0 : 1;
}

static int expect_eq_u64(uint64_t actual, uint64_t expected) {
    return actual == expected ? 0 : 1;
}

static int handle(swbt_app_t *app, uint32_t client_id, const char *line, char *response,
                  size_t response_size) {
    for (size_t index = 0; index < response_size; ++index) {
        response[index] = '\0';
    }
    return swbt_ipc_adapter_handle_line(app, client_id, line, response, response_size);
}

static int large_status_response_fits_response_buffer(void) {
    swbt_ipc_response_t typed_response = {
        .type = SWBT_IPC_RESPONSE_STATUS,
        .has_request_id = true,
        .status =
            {
                .has_owner = true,
                .owner_client_id = UINT32_MAX,
                .last_sequence = UINT64_MAX,
                .state =
                    {
                        .buttons = UINT32_MAX,
                        .lx = 4095u,
                        .ly = 4095u,
                        .rx = 4095u,
                        .ry = 4095u,
                        .accel_x = INT16_MIN,
                        .accel_y = INT16_MIN,
                        .accel_z = INT16_MIN,
                        .gyro_x = INT16_MIN,
                        .gyro_y = INT16_MIN,
                        .gyro_z = INT16_MIN,
                    },
                .rumble =
                    {
                        .raw = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
                        .updated_at_ms = UINT64_MAX,
                        .updated = true,
                    },
                .metrics =
                    {
                        .report_ticks = UINT64_MAX,
                        .report_send_ok = UINT64_MAX,
                        .report_send_failed = UINT64_MAX,
                        .report_interval_average_us = UINT64_MAX,
                        .report_interval_max_us = UINT64_MAX,
                        .ipc_state_accepted = UINT64_MAX,
                        .ipc_state_rejected = UINT64_MAX,
                        .ipc_state_coalesced = UINT64_MAX,
                        .hardware_status = SWBT_METRICS_HARDWARE_UNAVAILABLE,
                        .actual_report_rate_hz = UINT32_MAX,
                        .jitter_max_us = UINT64_MAX,
                    },
                .daemon =
                    {
                        .backend = SWBT_IPC_DAEMON_BACKEND_PRODUCTION,
                        .lifecycle_state = SWBT_IPC_DAEMON_LIFECYCLE_RUNNING,
                        .hardware_approval = SWBT_IPC_HARDWARE_APPROVAL_APPROVED,
                    },
                .hardware =
                    {
                        .adapter_state = SWBT_IPC_HARDWARE_CHANNEL_UNAVAILABLE,
                        .switch_connection_state = SWBT_IPC_HARDWARE_CHANNEL_UNAVAILABLE,
                        .hid_channel_state = SWBT_IPC_HARDWARE_CHANNEL_UNAVAILABLE,
                    },
            },
    };
    char response[SWBT_IPC_JSON_RESPONSE_MAX];

    for (size_t index = 0; index < SWBT_IPC_JSON_STRING_MAX - 1u; ++index) {
        typed_response.request_id[index] = 'r';
    }
    typed_response.request_id[SWBT_IPC_JSON_STRING_MAX - 1u] = '\0';
    return swbt_ipc_json_encode_response(&typed_response, response, sizeof(response)) ==
                       SWBT_IPC_JSON_OK &&
                   expect_contains(response, "\"type\":\"status\"") == 0 &&
                   strlen(response) < sizeof(response)
               ? 0
               : 1;
}

int main(void) {
    swbt_ipc_command_t command;
    swbt_ipc_response_t codec_error;
    swbt_app_t *app;
    swbt_ipc_status_t status;
    swbt_ipc_status_t status_after_decode;
    char response[SWBT_IPC_JSON_RESPONSE_MAX];
    const uint8_t active_rumble[SWBT_SWITCH_RUMBLE_DATA_SIZE] = {
        0x04, 0x01, 0x80, 0x41, 0x08, 0x01, 0x80, 0x42,
    };

    app = swbt_app_create();
    if (app == NULL) {
        return 1;
    }

    if (swbt_ipc_json_decode_command(
            "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000003e9\",\"seq\":77,"
            "\"request_id\":\"decode-only\",\"state\":{\"buttons\":8,\"lx\":1234,\"ly\":2345,"
            "\"rx\":2048,\"ry\":2048,\"accel_x\":1,\"accel_y\":2,\"accel_z\":3,"
            "\"gyro_x\":4,\"gyro_y\":5,\"gyro_z\":6}}\n",
            &command, &codec_error) != SWBT_IPC_JSON_OK) {
        return 37;
    }
    if (codec_error.type != SWBT_IPC_RESPONSE_NONE || command.type != SWBT_IPC_COMMAND_SET_STATE ||
        !command.has_request_id || strcmp(command.request_id, "decode-only") != 0 ||
        !command.has_owner_id || command.owner_client_id != 1001u || command.sequence != 77u ||
        expect_eq_u32(command.state.buttons, SWBT_BUTTON_A) ||
        expect_eq_u16(command.state.lx, 1234) || expect_eq_u16(command.state.ly, 2345)) {
        return 38;
    }
    if (swbt_ipc_adapter_get_status(app, &status_after_decode) != SWBT_IPC_OK) {
        return 39;
    }
    if (status_after_decode.has_owner || expect_eq_u32(status_after_decode.state.buttons, 0) ||
        expect_eq_u16(status_after_decode.state.lx, 2048) || status_after_decode.last_seq != 0u) {
        return 40;
    }

    if (swbt_ipc_json_decode_command("{\"v\":1,\"type\":\"set_state\"", &command, &codec_error) !=
        SWBT_IPC_JSON_OK) {
        return 41;
    }
    if (command.type != SWBT_IPC_COMMAND_NONE || codec_error.type != SWBT_IPC_RESPONSE_ERROR ||
        codec_error.has_request_id || codec_error.error_code != SWBT_IPC_ERROR_CODE_INVALID_JSON) {
        return 42;
    }
    if (swbt_ipc_json_encode_response(&codec_error, response, sizeof(response)) !=
        SWBT_IPC_JSON_OK) {
        return 43;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"invalid_json\"") ||
        expect_contains(response, "\"message\":\"message is not a complete JSON object\"")) {
        return 44;
    }
    if (swbt_ipc_adapter_get_status(app, &status_after_decode) != SWBT_IPC_OK) {
        return 45;
    }
    if (status_after_decode.has_owner || expect_eq_u32(status_after_decode.state.buttons, 0) ||
        expect_eq_u16(status_after_decode.state.lx, 2048) || status_after_decode.last_seq != 0u) {
        return 46;
    }

    if (swbt_ipc_json_decode_command(
            "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000003e9\",\"seq\":80,"
            "\"request_id\":\"top-level-state\",\"state\":{},\"buttons\":8,\"lx\":1234,"
            "\"ly\":2345,\"rx\":2048,\"ry\":2048,\"accel_x\":1,\"accel_y\":2,"
            "\"accel_z\":3,\"gyro_x\":4,\"gyro_y\":5,\"gyro_z\":6}\n",
            &command, &codec_error) != SWBT_IPC_JSON_OK) {
        return 47;
    }
    if (command.type != SWBT_IPC_COMMAND_NONE || codec_error.type != SWBT_IPC_RESPONSE_ERROR ||
        !codec_error.has_request_id || strcmp(codec_error.request_id, "top-level-state") != 0 ||
        codec_error.error_code != SWBT_IPC_ERROR_CODE_INVALID_STATE) {
        return 48;
    }
    if (swbt_ipc_adapter_get_status(app, &status_after_decode) != SWBT_IPC_OK) {
        return 49;
    }
    if (status_after_decode.has_owner || expect_eq_u32(status_after_decode.state.buttons, 0) ||
        expect_eq_u16(status_after_decode.state.lx, 2048) || status_after_decode.last_seq != 0u) {
        return 50;
    }

    if (handle(app, 1001,
               "{\"v\":1,\"type\":\"hello\",\"client_name\":\"unit\",\"request_id\":\"h1\"}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 2;
    }
    if (expect_contains(response, "\"type\":\"hello_ok\"") ||
        expect_contains(response, "\"request_id\":\"h1\"") ||
        expect_contains(response, "\"client_id\":\"000003e9\"")) {
        return 3;
    }

    if (handle(app, 1001,
               "{\"v\":1,\"type\":\"acquire\",\"mode\":\"exclusive\",\"request_id\":\"a1\"}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 4;
    }
    if (expect_contains(response, "\"type\":\"acquired\"") ||
        expect_contains(response, "\"request_id\":\"a1\"") ||
        expect_contains(response, "\"owner_id\":\"000003e9\"")) {
        return 5;
    }

    if (handle(app, 2002,
               "{\"v\":1,\"type\":\"acquire\",\"mode\":\"exclusive\",\"request_id\":\"a2\"}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 6;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"owner_busy\"") ||
        expect_contains(response, "\"request_id\":\"a2\"")) {
        return 7;
    }

    if (handle(app, 2002,
               "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000007d2\",\"seq\":77,"
               "\"request_id\":\"s2\",\"state\":{\"buttons\":8,\"lx\":1234,\"ly\":2345,"
               "\"rx\":2048,\"ry\":2048,\"accel_x\":1,\"accel_y\":2,\"accel_z\":3,"
               "\"gyro_x\":4,\"gyro_y\":5,\"gyro_z\":6}}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 8;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"not_owner\"") ||
        expect_contains(response, "\"request_id\":\"s2\"")) {
        return 9;
    }
    if (swbt_ipc_adapter_get_status(app, &status) != SWBT_IPC_OK) {
        return 52;
    }
    if (expect_eq_u64(status.metrics.ipc_state_rejected, 1u) ||
        expect_eq_u32(status.state.buttons, 0u) || expect_eq_u16(status.state.lx, 2048u) ||
        status.last_seq != 0u) {
        return 53;
    }

    if (handle(app, 1001,
               "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000003e9\",\"seq\":77,"
               "\"request_id\":\"s1\",\"state\":{\"buttons\":8,\"lx\":1234,\"ly\":2345,"
               "\"rx\":2048,\"ry\":2048,\"accel_x\":1,\"accel_y\":2,\"accel_z\":3,"
               "\"gyro_x\":4,\"gyro_y\":5,\"gyro_z\":6}}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 10;
    }
    if (expect_contains(response, "\"type\":\"state_accepted\"") ||
        expect_contains(response, "\"request_id\":\"s1\"") ||
        expect_contains(response, "\"seq\":77")) {
        return 11;
    }
    if (swbt_ipc_adapter_get_status(app, &status) != SWBT_IPC_OK) {
        return 12;
    }
    if (expect_eq_u32(status.state.buttons, SWBT_BUTTON_A) ||
        expect_eq_u16(status.state.lx, 1234) || expect_eq_u16(status.state.ly, 2345) ||
        status.last_seq != 77) {
        return 13;
    }
    if (swbt_app_record_rumble(app, active_rumble, 4242u) != SWBT_APP_OK) {
        return 32;
    }

    if (handle(app, 2002, "{\"v\":1,\"type\":\"get_status\",\"request_id\":\"g1\"}\n", response,
               sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 14;
    }
    if (expect_contains(response, "\"type\":\"status\"") ||
        expect_contains(response, "\"request_id\":\"g1\"") ||
        expect_contains(response,
                        "\"daemon\":{\"protocol_version\":1,\"daemon_version\":\"0.1.0-dev\","
                        "\"backend\":\"unknown\",\"lifecycle_state\":\"stopped\","
                        "\"hardware_approval\":\"unavailable\"}") ||
        expect_contains(response, "\"metrics\":{\"hardware_status\":\"unavailable\","
                                  "\"report_ticks_total\":0,\"reports_sent_total\":0,"
                                  "\"send_failures_total\":0,\"report_interval_average_us\":0,"
                                  "\"report_interval_max_us\":0,\"ipc_state_accepted_total\":1,"
                                  "\"ipc_state_rejected_total\":1,\"ipc_state_coalesced_total\":0,"
                                  "\"actual_report_rate_hz\":0,\"jitter_max_us\":0}") ||
        expect_contains(response, "\"hardware\":{\"adapter_state\":\"unavailable\","
                                  "\"switch_connection_state\":\"unavailable\","
                                  "\"hid_channel_state\":\"unavailable\"}") ||
        expect_contains(response, "\"present\":true") ||
        expect_contains(response, "\"owner_id\":\"000003e9\"") ||
        expect_contains(response, "\"last_seq\":77") ||
        expect_contains(response, "\"buttons\":8") || expect_contains(response, "\"lx\":1234") ||
        expect_contains(response, "\"rumble\":{\"updated\":true,\"last_update_ms\":4242,"
                                  "\"raw\":\"0401804108018042\"}")) {
        return 15;
    }

    if (handle(app, 1001,
               "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000003e9\",\"seq\":78,"
               "\"request_id\":\"bad-state\",\"state\":{\"buttons\":8,\"lx\":5000,\"ly\":2048,"
               "\"rx\":2048,\"ry\":2048,\"accel_x\":0,\"accel_y\":0,\"accel_z\":0,"
               "\"gyro_x\":0,\"gyro_y\":0,\"gyro_z\":0}}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 16;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"invalid_state\"") ||
        expect_contains(response, "\"request_id\":\"bad-state\"")) {
        return 17;
    }
    if (swbt_ipc_adapter_get_status(app, &status) != SWBT_IPC_OK) {
        return 18;
    }
    if (expect_eq_u16(status.state.lx, 1234) || status.last_seq != 77) {
        return 19;
    }

    if (handle(app, 1001,
               "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000003e9\",\"seq\":79,"
               "\"request_id\":\"bad-state-object\",\"state\":0,\"buttons\":0,\"lx\":2048,"
               "\"ly\":2048,\"rx\":2048,\"ry\":2048,\"accel_x\":0,\"accel_y\":0,"
               "\"accel_z\":0,\"gyro_x\":0,\"gyro_y\":0,\"gyro_z\":0}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 20;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"invalid_state\"") ||
        expect_contains(response, "\"request_id\":\"bad-state-object\"")) {
        return 21;
    }
    if (swbt_ipc_adapter_get_status(app, &status) != SWBT_IPC_OK) {
        return 22;
    }
    if (expect_eq_u16(status.state.lx, 1234) || status.last_seq != 77) {
        return 23;
    }

    if (handle(app, 1001,
               "{\"v\":1,\"type\":\"set_state\",\"owner_id\":\"000003e9\",\"seq\":76,"
               "\"request_id\":\"stale\",\"state\":{\"buttons\":2,\"lx\":3456,\"ly\":2048,"
               "\"rx\":2048,\"ry\":2048,\"accel_x\":0,\"accel_y\":0,\"accel_z\":0,"
               "\"gyro_x\":0,\"gyro_y\":0,\"gyro_z\":0}}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 33;
    }
    if (expect_contains(response, "\"type\":\"state_accepted\"") ||
        expect_contains(response, "\"request_id\":\"stale\"") ||
        expect_contains(response, "\"seq\":76")) {
        return 34;
    }
    if (swbt_ipc_adapter_get_status(app, &status) != SWBT_IPC_OK) {
        return 35;
    }
    if (expect_eq_u32(status.state.buttons, SWBT_BUTTON_A) ||
        expect_eq_u16(status.state.lx, 1234) || status.last_seq != 77) {
        return 36;
    }
    if (expect_eq_u64(status.metrics.ipc_state_rejected, 2u)) {
        return 54;
    }

    if (handle(app, 1001,
               "{\"v\":1,\"type\":\"release\",\"owner_id\":\"000003e9\",\"request_id\":\"r1\"}\n",
               response, sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 24;
    }
    if (expect_contains(response, "\"type\":\"released\"") ||
        expect_contains(response, "\"request_id\":\"r1\"")) {
        return 25;
    }
    if (swbt_ipc_adapter_get_status(app, &status) != SWBT_IPC_OK) {
        return 26;
    }
    if (status.has_owner || expect_eq_u32(status.state.buttons, 0) ||
        expect_eq_u16(status.state.lx, 2048)) {
        return 27;
    }

    if (handle(app, 1001, "{\"v\":2,\"type\":\"hello\",\"request_id\":\"bad-v\"}\n", response,
               sizeof(response)) != SWBT_IPC_JSON_OK) {
        return 28;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"invalid_version\"") ||
        expect_contains(response, "\"request_id\":\"bad-v\"")) {
        return 29;
    }

    if (handle(app, 1001, "{\"v\":1,\"type\":\"hello\"", response, sizeof(response)) !=
        SWBT_IPC_JSON_OK) {
        return 30;
    }
    if (expect_contains(response, "\"type\":\"error\"") ||
        expect_contains(response, "\"code\":\"invalid_json\"")) {
        return 31;
    }

    if (large_status_response_fits_response_buffer() != 0) {
        return 51;
    }

    swbt_app_destroy(app);
    return 0;
}
